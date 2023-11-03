// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VideoBackend.h"
#include "ScanlineBuffer.h"
#include "VgaMode.h"
#include "basic_math.h"
#include "scanvideo.pio.h"
#include "scanvideo_options.h"
#include "timing.pio.h"
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <pico/sync.h>
#include <stdio.h>
#include <string.h>

namespace kio ::Video
{

// clang-format off
#define pio0_hw ((pio_hw_t *)PIO0_BASE)				   // assert definition hasn't changed
#undef pio0_hw										   // undef c-style definition
#define pio0_hw reinterpret_cast<pio_hw_t*>(PIO0_BASE) // replace with c++-style definition

#define dma_hw ((dma_hw_t *)DMA_BASE)				 // assert definition hasn't changed
#undef dma_hw										 // undef c-style definition
#define dma_hw reinterpret_cast<dma_hw_t*>(DMA_BASE) // replace with c++-style definition
// clang-format on

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.VB" XWRAP(__LINE__))))	   // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.VB" XWRAP(__LINE__)))) // general ram


#define video_pio pio0

constexpr uint SYNC_PIN_BASE	= VIDEO_SYNC_PIN_BASE;
constexpr bool ENABLE_CLOCK_PIN = VIDEO_ENABLE_CLOCKS;
constexpr bool ENABLE_DEN_PIN	= VIDEO_ENABLE_CLOCKS;
constexpr uint CLOCK_POLARITY	= VIDEO_CLOCK_POLARITY;
constexpr uint DEN_POLARITY		= VIDEO_DEN_POLARITY;
constexpr uint TIMING_DMA_IRQ	= DMA_IRQ_1;

static uint TIMING_SM;
static uint SCANLINE_SM;
static uint TIMING_DMA_CHANNEL;
static uint SCANLINE_DMA_CTRL_CHANNEL;
static uint SCANLINE_DMA_DATA_CHANNEL;


// =========================================================

VgaMode vga_mode;

uint32		  px_per_scanline; // pixel per scanline
uint32		  px_per_frame;	   // pixel per frame
uint32		  cc_per_scanline; //
uint32		  cc_per_frame;	   //
uint		  cc_per_px;	   // cpu clock cycles per pixel
uint		  cc_per_us;	   // cpu clock cycles per microsecond
volatile bool in_vblank;
volatile int  line_at_frame_start;
uint32		  time_us_at_frame_start; // timestamp of start of active screen lines (set ~2 scanlines early)

static uint32 us_per_frame;
static uint	  cc_per_frame_fract;
static uint	  cc_per_frame_rest;


alignas(16) static uint32 prog_active[4];
alignas(16) static uint32 prog_vblank[4];
alignas(16) static uint32 prog_vpulse[4];

static struct
{
	uint32* program;
	uint32	count; // scanlines
} program[4];

enum State {
	generate_v_active,
	generate_v_frontporch,
	generate_v_pulse,
	generate_v_backporch,
	video_off,
};

static State state = video_off;
static uint8 scanline_program_load_offset;
static uint8 video_htiming_load_offset;


// =========================================================

static inline void RAM abort_all_dma_channels() noexcept
{
	// for irq handler only
	dma_hw->abort = (1u << SCANLINE_DMA_DATA_CHANNEL) | (1u << SCANLINE_DMA_CTRL_CHANNEL);

	while (dma_channel_is_busy(SCANLINE_DMA_DATA_CHANNEL)) tight_loop_contents();

	// we don't want any pending completion IRQ which may have happened in the interim
	dma_hw->ints0 = (1u << SCANLINE_DMA_DATA_CHANNEL) |
					(1u << SCANLINE_DMA_CTRL_CHANNEL); // this is probably not required because we don't use this IRQ
}

__unused static inline void RAM abort_all_scanline_sms() noexcept
{
	// there's a lot to do:
	// abort dma
	// drain pio tx fifo
	// drain pio tx register
	// set pio sm to position of PIO_WAIT_IRQ4

	//	abort_all_dma_channels();

	//constexpr uint32 CLR_IRQ_4 = 0xc044;
	//pio_sm_exec(video_pio, SM1, CLR_IRQ_4);	// clear IRQ4 -> never needed. why?

	const uint jmp = pio_encode_jmp(scanline_program_load_offset + video_scanline_wrap_target); // WAIT_IRQ4 position
	//const uint drain = pio_encode_out(pio_null, 32);	// drain the OSR (rp2040 §3.4.7.2)

	pio_sm_clear_fifos(video_pio, SCANLINE_SM); // drain the TX fifo
	pio_sm_exec(video_pio, SCANLINE_SM, jmp);	// goto WAIT_IRQ4 position
	//pio_sm_exec(video_pio, SM1, drain);		// drain the OSR -> blocks & eats one word if OSR is empty (as it should be)
}

static void RAM timing_isr() noexcept
{
	// DMA complete
	// interrupt for for timing pio
	// triggered when DMA completed and needs refill & restart
	// can be interrupted by scanline interrupt

	if (dma_irqn_get_channel_status(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL))
	{
		dma_irqn_acknowledge_channel(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL);

		state	   = State((state + 1) & 3);
		auto& prog = program[state];
		dma_channel_transfer_from_buffer_now(TIMING_DMA_CHANNEL, prog.program, prog.count);

		if (state == generate_v_frontporch)
		{
			in_vblank = true;
			__sev();
		}

		else if (state == generate_v_active)
		{
			in_vblank			= false;
			line_at_frame_start = line_at_frame_start + vga_mode.height;

			time_us_at_frame_start += cc_per_frame / cc_per_us;
			if ((cc_per_frame_rest += cc_per_frame_fract) >= cc_per_us)
			{
				cc_per_frame_rest -= cc_per_us;
				time_us_at_frame_start += 1;
			}

			__sev();
		}
	}
}


// =========================================================

static void setup_scanline_sm(const VgaMode& vga_mode) throws
{
	assert(scanline_buffer.is_valid());

	uint32 system_clock = get_system_clock();
	uint32 pixel_clock	= vga_mode.pixel_clock;
	uint   cc_per_pixel = system_clock / pixel_clock;

	if (cc_per_pixel < 2) throw "System clock is too low for the requested pixel clock";
	if (cc_per_pixel * pixel_clock != system_clock)
		throw "System clock must be an integer multiple of the requested pixel clock";

	// install program:

	const uint PIO_WAIT_IRQ4 = pio_encode_wait_irq(1, false, 4);
	assert(video_scanline_program.instructions[video_scanline_wrap_target] == PIO_WAIT_IRQ4);
	scanline_program_load_offset = uint8(pio_add_program(video_pio, &video_scanline_program));

	// setup scanline SMs:

	pio_sm_config config = video_scanline_program_get_default_config(scanline_program_load_offset);

	pio_sm_set_consecutive_pindirs(video_pio, SCANLINE_SM, VIDEO_COLOR_PIN_BASE, VIDEO_COLOR_PIN_COUNT, true);
	sm_config_set_out_pins(&config, VIDEO_COLOR_PIN_BASE, VIDEO_COLOR_PIN_COUNT);
	sm_config_set_out_shift(&config, true, true, 32); // autopull
	sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

	sm_config_set_clkdiv_int_frac(&config, uint16(cc_per_pixel / 2), uint8((cc_per_pixel & 1u) << 7u));
	pio_sm_init(video_pio, SCANLINE_SM, scanline_program_load_offset, &config); // sm paused
}

static void setup_timing_sm(uint32 pixel_clock_frequency)
{
	// get the program, modify it as needed and install it:

	uint16 instructions[32];
	memcpy(instructions, video_htiming_program_instructions, sizeof(video_htiming_program_instructions));
	pio_program_t program = video_htiming_program;
	program.instructions  = instructions;

	if constexpr (ENABLE_CLOCK_PIN && CLOCK_POLARITY != 0)
	{
		constexpr uint32 clock_pin_side_set_bitmask = 0x1000;
		for (uint i = 0; i < program.length; i++) { instructions[i] ^= clock_pin_side_set_bitmask; }
	}

	video_htiming_load_offset = uint8(pio_add_program(video_pio, &program));

	// configure state machine:

	pio_sm_config config = video_htiming_program_get_default_config(video_htiming_load_offset);

	uint system_clock		   = clock_get_hz(clk_sys);
	uint clock_divider_times_2 = system_clock / pixel_clock_frequency; // assuming 2 clocks / pixel
	sm_config_set_clkdiv_int_frac(
		&config, uint16(clock_divider_times_2 / 2), uint8((clock_divider_times_2 & 1u) << 7u));

	// enable auto-pull
	sm_config_set_out_shift(&config, true, true, 32);

	// hsync and vsync are +0 and +1, den is +2 if present, clock is side-set at +2, or +3 if den present
	uint pin_count = ENABLE_DEN_PIN ? 3 : 2;
	sm_config_set_out_pins(&config, SYNC_PIN_BASE, pin_count);

	if constexpr (ENABLE_CLOCK_PIN)
	{
		sm_config_set_sideset_pins(&config, SYNC_PIN_BASE + pin_count);
		pin_count++;
	}

	pio_sm_set_consecutive_pindirs(video_pio, TIMING_SM, SYNC_PIN_BASE, pin_count, true);

	pio_sm_init(video_pio, TIMING_SM, video_htiming_load_offset, &config); // now paused
}

static void setup_timing_programs(const VgaMode& timing)
{
	const uint SET_IRQ_4 = pio_encode_irq_set(false, 4);   //  irq nowait 4  side 0
	const uint CLR_IRQ_4 = pio_encode_irq_clear(false, 4); //  irq clear  4  side 0

	constexpr int TIMING_CYCLE = 3u;
	constexpr int HTIMING_MIN  = TIMING_CYCLE + 1;

	assert(timing.h_active() >= HTIMING_MIN);
	assert(timing.h_pulse >= HTIMING_MIN);
	assert(timing.h_back_porch >= HTIMING_MIN);
	assert(timing.h_front_porch >= HTIMING_MIN);
	assert(timing.h_total() % 2 == 0);
	assert(timing.h_pulse % 2 == 0);

	// horizontal timing:

	// bits are read backwards (lsb to msb) by PIO pogram
	// the scanline starts with the HSYNC pulse!

	// polarity mask to toggle out bits, applied to whole cmd:
	const uint32 polarity_mask = uint32(!timing.h_sync_polarity << 29) + uint32(!timing.v_sync_polarity << 30) +
								 uint32(DEN_POLARITY << 31) + uint32(CLOCK_POLARITY << 12);

	constexpr uint32 TIMING_CYCLES = 3u << 16;
#define MK_CMD(CMD, CYCLES, BITS) ((CMD) | (CYCLES - TIMING_CYCLES) | (BITS)) ^ polarity_mask

	constexpr uint32 hsync_bit = 1u << 29;
	constexpr uint32 vsync_bit = 1u << 30;
	constexpr uint32 den_bit   = 1u << 31;

	const uint32 h_frontporch = uint32(timing.h_front_porch) << 16;
	const uint32 h_active	  = uint32(timing.h_active()) << 16;
	const uint32 h_backporch  = uint32(timing.h_back_porch) << 16;
	const uint32 h_pulse	  = uint32(timing.h_pulse) << 16;

	// display area:
	prog_active[0] = MK_CMD(CLR_IRQ_4, h_pulse, hsync_bit + !vsync_bit);
	prog_active[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + !vsync_bit);
	prog_active[2] = MK_CMD(SET_IRQ_4, h_active, !hsync_bit + !vsync_bit + den_bit);
	prog_active[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + !vsync_bit);

	// vblank, front & back porch:
	prog_vblank[0] = MK_CMD(CLR_IRQ_4, h_pulse, hsync_bit + !vsync_bit);
	prog_vblank[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + !vsync_bit);
	prog_vblank[2] = MK_CMD(CLR_IRQ_4, h_active, !hsync_bit + !vsync_bit);
	prog_vblank[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + !vsync_bit);

	// vblank, vsync pulse:
	prog_vpulse[0] = MK_CMD(CLR_IRQ_4, h_pulse, hsync_bit + vsync_bit);
	prog_vpulse[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + vsync_bit);
	prog_vpulse[2] = MK_CMD(CLR_IRQ_4, h_active, !hsync_bit + vsync_bit);
	prog_vpulse[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + vsync_bit);

	// vertical timing:

	const uint v_active		 = timing.v_active();
	const uint v_front_porch = timing.v_front_porch;
	const uint v_pulse		 = timing.v_pulse;
	const uint v_back_porch	 = timing.v_back_porch;

	program[generate_v_active]	   = {.program = prog_active, .count = count_of(prog_active) * v_active};
	program[generate_v_frontporch] = {.program = prog_vblank, .count = count_of(prog_vblank) * v_front_porch};
	program[generate_v_pulse]	   = {.program = prog_vpulse, .count = count_of(prog_vpulse) * v_pulse};
	program[generate_v_backporch]  = {.program = prog_vblank, .count = count_of(prog_vblank) * v_back_porch};
}

static void setup_gpio_pins()
{
	constexpr uint32 RMASK = ((1u << VIDEO_PIXEL_RCOUNT) - 1u) << VIDEO_PIXEL_RSHIFT;
	constexpr uint32 GMASK = ((1u << VIDEO_PIXEL_GCOUNT) - 1u) << VIDEO_PIXEL_GSHIFT;
	constexpr uint32 BMASK = ((1u << VIDEO_PIXEL_BCOUNT) - 1u) << VIDEO_PIXEL_BSHIFT;

	constexpr uint32 color_pins = (RMASK | GMASK | BMASK) << VIDEO_COLOR_PIN_BASE;
	constexpr uint32 sync_pins	= (3u | (ENABLE_DEN_PIN << 2) | (ENABLE_CLOCK_PIN << 3)) << SYNC_PIN_BASE;

	for (uint pin_mask = color_pins | sync_pins, i = 0; pin_mask; i++, pin_mask >>= 1)
	{
		if (pin_mask & 1) gpio_set_function(i, GPIO_FUNC_PIO0);
	}
}

static void setup_dma()
{
	dma_channel_config config = dma_channel_get_default_config(TIMING_DMA_CHANNEL);

	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + TIMING_SM);  // dreq = TX FIFO not full
	channel_config_set_ring(&config, false /*read*/, 4 /*log2*/); // wrap read at 4 words = sizeof program arrays

	dma_channel_configure(
		TIMING_DMA_CHANNEL, &config,
		&video_pio->txf[TIMING_SM], // write address
		nullptr /*later*/,			// read address: set by isr
		0,							// transfer count: set by isr
		false);						// don't start now

	// configure scanline dma control channel:

	config = dma_channel_get_default_config(SCANLINE_DMA_CTRL_CHANNEL);

	channel_config_set_ring(
		&config, false /*read*/, //
		msbit((scanline_buffer.count * sizeof(ptr) << scanline_buffer.vss)) /*log2bytes*/);

	dma_channel_configure(
		SCANLINE_DMA_CTRL_CHANNEL, &config,
		&dma_channel_hw_addr(SCANLINE_DMA_DATA_CHANNEL)->al3_read_addr_trig, // write address
		scanline_buffer.scanlines,											 // read address
		1,		// send 1 word to data channel ctrl block per transfer
		false); // don't start now

	// configure scanline dma data channel:

	config = dma_channel_get_default_config(SCANLINE_DMA_DATA_CHANNEL);

	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + SCANLINE_SM);	 // dreq = TX FIFO not full
	channel_config_set_chain_to(&config, SCANLINE_DMA_CTRL_CHANNEL); // link to control channel
	channel_config_set_irq_quiet(&config, true);					 // no irpt at end of transfer

	dma_channel_configure(
		SCANLINE_DMA_DATA_CHANNEL, &config,
		&video_pio->txf[SCANLINE_SM], // write address
		nullptr,					  // read address: set by control channel
		scanline_buffer.width / 2,	  // count
		false);						  // don't start now
}

void VideoBackend::start(const VgaMode& vga_mode) throws
{
	assert(get_core_num() == 1);
	assert(get_system_clock() % vga_mode.pixel_clock == 0);
	assert(get_system_clock() % 1000000 == 0);
	assert(scanline_buffer.is_valid());

	stop();
	Video::vga_mode = vga_mode;

	cc_per_us		= get_system_clock() / 1000000;				 // non fract
	cc_per_px		= get_system_clock() / vga_mode.pixel_clock; // non fract
	px_per_scanline = vga_mode.h_total() << vga_mode.vss;		 // non fract
	px_per_frame	= vga_mode.h_total() * vga_mode.v_total();	 // non fract
	cc_per_scanline = cc_per_px * px_per_scanline;				 // non fract
	cc_per_frame	= cc_per_px * px_per_frame;					 // non fract

	us_per_frame	   = cc_per_frame / cc_per_us; // fract
	cc_per_frame_fract = cc_per_frame % cc_per_us; // remainder
	cc_per_frame_rest  = 0;						   // correction counter

	printf("system clock = %u\n", get_system_clock());
	printf("pixel clock  = %u\n", vga_mode.pixel_clock);
	printf("cc_per_us = %u\n", cc_per_us);
	printf("cc_per_px = %u\n", cc_per_px);
	printf("px_per_scanline = %u\n", px_per_scanline);
	printf("cc_per_scanline = %u\n", cc_per_scanline);
	printf("px_per_frame = %u\n", px_per_frame);
	printf("cc_per_frame = %u\n", cc_per_frame);
	printf("us_per_frame = %u, fract = %u/%u\n", us_per_frame, cc_per_frame_fract, cc_per_us);

	setup_scanline_sm(vga_mode);
	setup_timing_sm(vga_mode.pixel_clock);
	setup_timing_programs(vga_mode);
	setup_dma();

	// load line width into scanline sm and start sm:
	pio_sm_restart(video_pio, SCANLINE_SM);
	uint jmp = pio_encode_jmp(scanline_program_load_offset);
	pio_sm_exec(video_pio, SCANLINE_SM, jmp);
	pio_sm_put(video_pio, SCANLINE_SM, vga_mode.h_active() - 1);

	// start timing sm:
	pio_sm_restart(video_pio, TIMING_SM);
	jmp = pio_encode_jmp(video_htiming_load_offset + video_htiming_offset_entry_point);
	pio_sm_exec(video_pio, TIMING_SM, jmp);

	pio_enable_sm_mask_in_sync(video_pio, (1u << SCANLINE_SM) | (1 << TIMING_SM));

	// start timing dma:
	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL, true); // enable DMA_CHANNEL irqs on DMA_IRQ
	irq_set_enabled(TIMING_DMA_IRQ, true);									// enable DMA_IRQ interrupt on this core

	auto& prog = program[state = generate_v_active];
	dma_channel_transfer_from_buffer_now(TIMING_DMA_CHANNEL, prog.program, prog.count); // trigger first irq

	time_us_at_frame_start = time_us_32(); // get system time *immediately* after starting the dma
	in_vblank			   = false;		   // we start in v_active
	line_at_frame_start	   = 0;			   // line number where scanline dma starts reading from scanline_buffer[]

	dma_channel_start(SCANLINE_DMA_CTRL_CHANNEL);
}

void VideoBackend::stop() noexcept
{
	assert(get_core_num() == 1);

	pio_set_sm_mask_enabled(video_pio, (1u << SCANLINE_SM) | (1u << TIMING_SM), false); // stop sm

	//timing_sm:
	irq_set_enabled(TIMING_DMA_IRQ, false);									 // disable interrupt
	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL, false); // disable interrupt source
	dma_channel_abort(TIMING_DMA_CHANNEL);

	//scanline_sm:
	abort_all_dma_channels();

	if (state == video_off) return;
	state = video_off;

	//teardown:
	pio_remove_program(video_pio, &video_scanline_program, scanline_program_load_offset);
	pio_remove_program(video_pio, &video_htiming_program, video_htiming_load_offset);
}

void VideoBackend::initialize() noexcept
{
	// any core

	setup_gpio_pins();

	TIMING_DMA_CHANNEL		  = uint(dma_claim_unused_channel(true));
	SCANLINE_DMA_CTRL_CHANNEL = uint(dma_claim_unused_channel(true));
	SCANLINE_DMA_DATA_CHANNEL = uint(dma_claim_unused_channel(true));
	TIMING_SM				  = uint(pio_claim_unused_sm(video_pio, true));
	SCANLINE_SM				  = uint(pio_claim_unused_sm(video_pio, true));

	irq_add_shared_handler(TIMING_DMA_IRQ, timing_isr, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
}


} // namespace kio::Video


/*































*/
