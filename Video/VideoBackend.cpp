// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VideoBackend.h"
#include "ScanlineBuffer.h"
#include "VgaMode.h"
#include "basic_math.h"
#include "scanline.pio.h"
#include "timing.pio.h"
#include "utilities/system_clock.h"
#include "video_options.h"
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <pico/sync.h>
#include <stdio.h>
#include <string.h>

namespace kio::Video
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

constexpr uint SIZEOF_COLOR		  = VIDEO_COLOR_PIN_COUNT <= 8 ? 1 : 2;
constexpr uint SYNC_PIN_BASE	  = VIDEO_SYNC_PIN_BASE;
constexpr uint HSYNC_PIN		  = VIDEO_SYNC_PIN_BASE;
constexpr uint VSYNC_PIN		  = VIDEO_SYNC_PIN_BASE + 1;
constexpr uint CLOCK_PIN_BASE	  = VIDEO_CLOCK_PIN_BASE;
constexpr uint CLOCK_PIN		  = VIDEO_CLOCK_PIN_BASE;
constexpr uint DEN_PIN			  = VIDEO_CLOCK_PIN_BASE + 1;
constexpr bool ENABLE_CLOCK		  = VIDEO_CLOCK_ENABLE;
constexpr bool CLOCK_POLARITY	  = VIDEO_CLOCK_POLARITY;
constexpr bool ENABLE_DEN		  = VIDEO_DEN_ENABLE;
constexpr bool DEN_POLARITY		  = VIDEO_DEN_POLARITY;
constexpr uint TIMING_DMA_IRQ_IDX = 1;

static uint8 TIMING_SM;
static uint8 SCANLINE_SM;
static uint8 TIMING_DMA_CHANNEL;
static uint8 SCANLINE_DMA_CTRL_CHANNEL;
static uint8 SCANLINE_DMA_DATA_CHANNEL;


// =========================================================

VgaMode vga_mode;

uint32			cc_per_px;
uint32			cc_per_scanline; //
uint32			cc_per_frame;	 //
uint			cc_per_us;		 // cpu clock cycles per microsecond
volatile bool	in_vblank;
volatile int	line_at_frame_start;
volatile uint32 time_us_at_frame_start; // timestamp of start of active screen lines (updated ~1 scanline early)
volatile uint32 time_cc_at_frame_start; // timestamp of start of active screen lines, precise to the cc
volatile int	current_frame = 0;

static uint us_per_frame;		// integer part of fractional value!
static uint cc_per_frame_fract; // the fractional part [cc]
static uint cc_per_frame_rest;	// accumulator in ISR


alignas(16) static uint32 prog_active[4];
alignas(16) static uint32 prog_vblank[4];
alignas(16) static uint32 prog_vpulse[4];

static struct
{
	uint32* program;
	uint32	count; // scanlines
} program[4];

enum State {
	in_active_screen,
	in_frontporch,
	in_pulse,
	in_backporch,
	not_started,
	not_initialized,
};

static volatile State state = not_initialized;
static uint8		  scanline_program_load_offset;
static uint8		  timing_program_load_offset;


// =========================================================

uint32 RAM time_cc_32() noexcept
{
	// calculated from the usec time
	// => up to cc_per_us too low ((plus time for evaluation))
a:
	uint32 cc0 = time_cc_at_frame_start;
	uint32 us0 = time_us_at_frame_start;
	if (cc0 != time_cc_at_frame_start) goto a;
	return cc0 + (time_us_32() - us0) * cc_per_us + cc_per_frame_rest;
}


static __always_inline bool dma_irqn_get_channel_status(uint irq_index, uint channel)
{
	//invalid_params_if(HARDWARE_DMA, irq_index >= NUM_DMA_IRQS);
	//check_dma_channel_param(channel);
	return dma_hw->irq_ctrl[irq_index].ints & (1u << channel);
}
static __always_inline void dma_irqn_acknowledge_channel(uint irq_index, uint channel)
{
	//invalid_params_if(HARDWARE_DMA, irq_index >= NUM_DMA_IRQS);
	//check_dma_channel_param(channel);
	dma_hw->irq_ctrl[irq_index].ints = 1u << channel;
}
static __always_inline dma_channel_hw_t* dma_channel_hw_addr(uint channel)
{
	//check_dma_channel_param(channel);
	return &dma_hw->ch[channel];
}
static __always_inline void
dma_channel_transfer_from_buffer_now(uint channel, const volatile void* read_addr, uint32_t transfer_count)
{
	//check_dma_channel_param(channel);
	dma_channel_hw_t* hw		= dma_channel_hw_addr(channel);
	hw->read_addr				= uintptr_t(read_addr);
	hw->al1_transfer_count_trig = transfer_count;
}

static void RAM timing_isr() noexcept
{
	// DMA complete
	// interrupt for for timing pio
	// triggered when DMA completed and needs refill & restart
	// can be interrupted by scanline interrupt

	if (dma_irqn_get_channel_status(TIMING_DMA_IRQ_IDX, TIMING_DMA_CHANNEL))
	{
		dma_irqn_acknowledge_channel(TIMING_DMA_IRQ_IDX, TIMING_DMA_CHANNEL);

		state	   = State((state + 1) & 3);
		auto& prog = program[state];
		dma_channel_transfer_from_buffer_now(TIMING_DMA_CHANNEL, prog.program, prog.count);

		if (state == in_frontporch)
		{
			in_vblank			= true;
			current_frame		= current_frame + 1;
			line_at_frame_start = line_at_frame_start + vga_mode.height;

			time_us_at_frame_start = time_us_at_frame_start + us_per_frame;
			time_cc_at_frame_start = time_cc_at_frame_start + cc_per_frame;

			if ((cc_per_frame_rest += cc_per_frame_fract) >= cc_per_us)
			{
				cc_per_frame_rest -= cc_per_us;
				time_us_at_frame_start = time_us_at_frame_start + 1;
			}

			__sev();
		}
		else if (state == in_active_screen)
		{
			in_vblank = false;

			__sev();
		}
	}
}


// =========================================================

static void initialize_gpio_pins() noexcept
{
	constexpr uint32 RMASK = ((1u << VIDEO_PIXEL_RCOUNT) - 1u) << VIDEO_PIXEL_RSHIFT;
	constexpr uint32 GMASK = ((1u << VIDEO_PIXEL_GCOUNT) - 1u) << VIDEO_PIXEL_GSHIFT;
	constexpr uint32 BMASK = ((1u << VIDEO_PIXEL_BCOUNT) - 1u) << VIDEO_PIXEL_BSHIFT;
	constexpr uint32 IMASK = ((1u << VIDEO_PIXEL_ICOUNT) - 1u) << VIDEO_PIXEL_ISHIFT;

	constexpr uint32 color_pins = (RMASK | GMASK | BMASK | IMASK) << VIDEO_COLOR_PIN_BASE;
	constexpr uint32 sync_pins	= (3 << SYNC_PIN_BASE) | (ENABLE_CLOCK << CLOCK_PIN) | (ENABLE_DEN << DEN_PIN);

	for (uint pin_mask = color_pins | sync_pins, i = 0; pin_mask; i++, pin_mask >>= 1)
	{
		if (pin_mask & 1) gpio_set_function(i, GPIO_FUNC_PIO0);
	}
}

static void initialize_state_machines() noexcept
{
	// load programs, configure pins and configure state machines
	// hsync polarity, vsync polarity and clock dividers will be set in start()

	// set signal polarity:
	if (ENABLE_CLOCK) gpio_set_outover(CLOCK_PIN, CLOCK_POLARITY);
	if (ENABLE_DEN) gpio_set_outover(DEN_PIN, DEN_POLARITY);

	// set pins to output:
	if (ENABLE_CLOCK) pio_sm_set_consecutive_pindirs(video_pio, SCANLINE_SM, CLOCK_PIN, 1, true);
	if (ENABLE_DEN) pio_sm_set_consecutive_pindirs(video_pio, SCANLINE_SM, DEN_PIN, 1, true);
	pio_sm_set_consecutive_pindirs(video_pio, TIMING_SM, SYNC_PIN_BASE, 2, true /*out*/);
	pio_sm_set_consecutive_pindirs(video_pio, SCANLINE_SM, VIDEO_COLOR_PIN_BASE, VIDEO_COLOR_PIN_COUNT, true);

	// load state machine programs:
	timing_program_load_offset	 = uint8(pio_add_program(video_pio, &timing_program));
	scanline_program_load_offset = uint8(pio_add_program(
		video_pio, VIDEO_COLOR_PIN_COUNT <= 8 ? //
					   ENABLE_DEN	? &scanline_den_8bpp_program :
					   ENABLE_CLOCK ? &scanline_clk_8bpp_program :
									  &scanline_vga_8bpp_program :
					   ENABLE_DEN	? &scanline_den_program :
					   ENABLE_CLOCK ? &scanline_clk_program :
									  &scanline_vga_program));

	// setup scanline state machine:
	pio_sm_config config = scanline_vga_program_get_default_config(scanline_program_load_offset);
	sm_config_set_out_pins(&config, VIDEO_COLOR_PIN_BASE, VIDEO_COLOR_PIN_COUNT);
	sm_config_set_out_shift(&config, true, true, 32); // autopull
	sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
	sm_config_set_sideset_pins(&config, CLOCK_PIN_BASE);						// side-set: den+clock
	pio_sm_init(video_pio, SCANLINE_SM, scanline_program_load_offset, &config); // sm paused

	// setup timing state machine:
	config = timing_program_get_default_config(timing_program_load_offset);
	sm_config_set_out_shift(&config, true, true, 32);						// autopull
	sm_config_set_out_pins(&config, SYNC_PIN_BASE, 2);						// hsync+vsync
	pio_sm_init(video_pio, TIMING_SM, timing_program_load_offset, &config); // now paused
}

static void setup_state_machines(const VgaMode& vga_mode, uint32 cc_per_pixel) noexcept
{
	// hsync, vsync polarity and clock dividers depend on vga_mode and must be set each start:

	// set signal polarity:
	gpio_set_outover(HSYNC_PIN, vga_mode.h_sync_polarity);
	gpio_set_outover(VSYNC_PIN, vga_mode.v_sync_polarity);

	pio_sm_set_clkdiv_int_frac(video_pio, TIMING_SM, uint16(cc_per_pixel / 2), uint8((cc_per_pixel & 1u) << 7u));
	pio_sm_set_clkdiv_int_frac(video_pio, SCANLINE_SM, uint16(cc_per_pixel / 2), uint8((cc_per_pixel & 1u) << 7u));

	// load line width into scanline sm and start sm:
	pio_sm_drain_tx_fifo(video_pio, SCANLINE_SM);
	pio_sm_restart(video_pio, SCANLINE_SM);
	uint jmp = pio_encode_jmp(scanline_program_load_offset);
	pio_sm_exec(video_pio, SCANLINE_SM, jmp);
	pio_sm_put(video_pio, SCANLINE_SM, vga_mode.h_active() - 1);

	// start timing sm:
	pio_sm_drain_tx_fifo(video_pio, TIMING_SM);
	pio_sm_restart(video_pio, TIMING_SM);
	jmp = pio_encode_jmp(timing_program_load_offset + timing_offset_entry_point);
	pio_sm_exec(video_pio, TIMING_SM, jmp);
}

static void setup_timing_programs(const VgaMode& vga_mode)
{
	constexpr int count_base = 3;
	assert(vga_mode.h_active() >= count_base);
	assert(vga_mode.h_pulse >= count_base);
	assert(vga_mode.h_back_porch >= count_base);
	assert(vga_mode.h_front_porch >= count_base);
	assert(vga_mode.h_active() <= 1600);
	assert(vga_mode.h_pulse < 200);
	assert(vga_mode.h_back_porch < 200);
	assert(vga_mode.h_front_porch < 200);

	// horizontal timing:

	// bits are read backwards (lsb to msb) by PIO pogram
	// the scanline starts with the front porch!
	// note: formerly the scanline started with the hsync pulse.
	//       makes no difference with my monitor, but 1280*768 now works slightly better.

	const uint set_irq_4 = pio_encode_irq_set(false, 4);
	const uint clr_irq_4 = pio_encode_irq_clear(false, 4);

	constexpr uint32 hsync = 1u << 30;
	constexpr uint32 vsync = 1u << 31;

	// clang-format off
	const uint32 h_frontporch = uint32(vga_mode.h_front_porch - count_base) << 16;
	const uint32 h_active	  = uint32(vga_mode.h_active()    - count_base) << 16;
	const uint32 h_backporch  = uint32(vga_mode.h_back_porch  - count_base) << 16;
	const uint32 h_pulse	  = uint32(vga_mode.h_pulse       - count_base) << 16;

	// display area:
	prog_active[1] = clr_irq_4 + h_pulse      +  hsync + !vsync;
	prog_active[2] = clr_irq_4 + h_backporch  + !hsync + !vsync;
	prog_active[3] = set_irq_4 + h_active     + !hsync + !vsync;
	prog_active[0] = clr_irq_4 + h_frontporch + !hsync + !vsync;

	// vblank, front & back porch:
	prog_vblank[1] = clr_irq_4 + h_pulse      +  hsync + !vsync;
	prog_vblank[2] = clr_irq_4 + h_backporch  + !hsync + !vsync;
	prog_vblank[3] = clr_irq_4 + h_active     + !hsync + !vsync;
	prog_vblank[0] = clr_irq_4 + h_frontporch + !hsync + !vsync;

	// vblank, vsync pulse:
	prog_vpulse[1] = clr_irq_4 + h_pulse	  +  hsync +  vsync;
	prog_vpulse[2] = clr_irq_4 + h_backporch  + !hsync +  vsync;
	prog_vpulse[3] = clr_irq_4 + h_active     + !hsync +  vsync;
	prog_vpulse[0] = clr_irq_4 + h_frontporch + !hsync +  vsync;
	// clang-format on

	// vertical timing:

	program[in_active_screen] = {.program = prog_active, .count = count_of(prog_active) * vga_mode.v_active()};
	program[in_frontporch]	  = {.program = prog_vblank, .count = count_of(prog_vblank) * vga_mode.v_front_porch};
	program[in_pulse]		  = {.program = prog_vpulse, .count = count_of(prog_vpulse) * vga_mode.v_pulse};
	program[in_backporch]	  = {.program = prog_vblank, .count = count_of(prog_vblank) * vga_mode.v_back_porch};
}

static void setup_dma(const VgaMode& vga_mode)
{
	// configure timing dma:

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
		&video_pio->txf[SCANLINE_SM],			  // write address
		nullptr,								  // read address: set by control channel
		vga_mode.h_active() / (4 / SIZEOF_COLOR), // count
		false);									  // don't start now

	// configure timing dma interrupt:

	dma_channel_acknowledge_irq1(TIMING_DMA_CHANNEL);							// discard any pending event (required?)
	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ_IDX, TIMING_DMA_CHANNEL, true); // enable DMA_CHANNEL irqs on DMA_IRQ
	irq_set_enabled(DMA_IRQ_0 + TIMING_DMA_IRQ_IDX, true);						// enable DMA_IRQ interrupt on this core
}

static void stop_sm_and_dma() noexcept
{
	// stop timing irq & dma:
	irq_set_enabled(DMA_IRQ_0 + TIMING_DMA_IRQ_IDX, false);						 // disable interrupt
	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ_IDX, TIMING_DMA_CHANNEL, false); // disable interrupt source
	dma_channel_cleanup(TIMING_DMA_CHANNEL);

	// stop scanline dma:
	dma_channel_cleanup(SCANLINE_DMA_DATA_CHANNEL);
	dma_channel_cleanup(SCANLINE_DMA_CTRL_CHANNEL);

	// stop sm:
	pio_set_sm_mask_enabled(video_pio, (1u << SCANLINE_SM) | (1u << TIMING_SM), false);
}

static constexpr uint32 full_mhz(uint32 f) noexcept { return (f + 1 MHz / 2) / (1 MHz) * (1 MHz); }

sysclock_params calc_new_sysclock(uint32 min_clock, uint32 pixel_clock)
{
	// the resulting sysclock should be higher than min_sys_clock
	// the resulting sysclock must be lower than VIDEO_MAX_SYSCLOCK_MHz
	// the resulting sysclock must be a multiple of 1 MHz, preferrably with no rounding error
	// the pixel clock multiplier must be at least 2.

	uint32 max_clock = max(150 MHz, VIDEO_MAX_SYSCLOCK_MHz MHz, 2 * pixel_clock);
	max_clock		 = (max_clock + 99) / pixel_clock * pixel_clock;
	min_clock		 = min(min_clock, max_clock);
	min_clock		 = max(min_clock, 90 MHz, 2 * pixel_clock);
	min_clock		 = (min_clock + pixel_clock - 99) / pixel_clock * pixel_clock;

	sysclock_params best = calc_sysclock_params(min_clock);
	for (uint clock = min_clock + pixel_clock; best.err && clock <= max_clock; clock += pixel_clock)
	{
		sysclock_params params = calc_sysclock_params(clock);
		if (params.err < best.err) best = params;
	}
	return best;
}

void VideoBackend::start(const VgaMode& vga_mode, uint32 min_sys_clock) throws
{
	assert(get_core_num() == 1);
	assert(scanline_buffer.is_valid());
	assert(state != not_initialized);

	// we need a multiple of pixel clock and a multiple of 1 MHz less than fMAX:
	// `min_sys_clock` is only used as a hint.
	if (!min_sys_clock) min_sys_clock = uint32(vga_mode.width) * vga_mode.v_total() * 8 * 60; // best guess
	sysclock_params params = calc_new_sysclock(min_sys_clock, vga_mode.pixel_clock);
	assert(params.sysclock % 1000000 == 0);

	if (params.voltage > vreg_get_voltage())
	{
		vreg_set_voltage(params.voltage); // up
		sleep_ms(5);
	}

	if (state != not_started)
	{
		while (in_vblank) { wfe(); }
		while (!in_vblank) { wfe(); }
		stop_sm_and_dma();
	}

	// *** VIDEO GENERATION STOPPED ***

	set_sys_clock_pll(params.vco, params.div1, params.div2);
	assert(get_system_clock() == params.sysclock);

	uint32	   new_sys_clock = params.sysclock;
	const uint cc_per_pixel	 = (new_sys_clock + vga_mode.pixel_clock / 2) / vga_mode.pixel_clock;
	uint32	   pixel_clock	 = new_sys_clock / cc_per_pixel;

	Video::vga_mode				= vga_mode;
	Video::vga_mode.pixel_clock = pixel_clock;
	setup_state_machines(vga_mode, cc_per_pixel);
	setup_timing_programs(vga_mode);
	setup_dma(vga_mode);

	cc_per_px			   = cc_per_pixel;							  // non fract
	uint32 px_per_scanline = vga_mode.h_total() << vga_mode.vss;	  // non fract
	uint32 px_per_frame	   = vga_mode.h_total() * vga_mode.v_total(); // non fract
	cc_per_scanline		   = cc_per_pixel * px_per_scanline;		  // non fract
	cc_per_frame		   = cc_per_pixel * px_per_frame;			  // non fract
	cc_per_us			   = new_sys_clock / 1000000;				  // non fract
	us_per_frame		   = cc_per_frame / cc_per_us;				  // fract
	cc_per_frame_fract	   = cc_per_frame % cc_per_us;				  // remainder
	cc_per_frame_rest	   = 0;										  // correction counter
	line_at_frame_start	   = 0;		// line number where scanline dma starts reading from scanline_buffer[]
	in_vblank			   = false; // we start in v_active

	// restart everything:

	pio_enable_sm_mask_in_sync(video_pio, (1u << SCANLINE_SM) | (1 << TIMING_SM));

	state	   = in_active_screen;
	auto& prog = program[in_active_screen];
	for (volatile uint32 now = time_us_32(); now == time_us_32();) {}
	dma_channel_transfer_from_buffer_now(TIMING_DMA_CHANNEL, prog.program, prog.count);
	time_us_at_frame_start = time_us_32(); // get *exact* system time for start of dma
	dma_channel_start(SCANLINE_DMA_CTRL_CHANNEL);

	// *** VIDEO GENERATION RESTARTED ***

	sysclock_changed(new_sys_clock);

	if (params.voltage < vreg_get_voltage())
	{
		sleep_ms(5);
		vreg_set_voltage(params.voltage); // down
	}

	uint fps = (pixel_clock / vga_mode.h_total() * 1000 + vga_mode.v_total() / 2) / vga_mode.v_total();
	debugstr("set resolution %i x %i @ %u.%03u Hz\n", vga_mode.width, vga_mode.height, fps / 1000, fps % 1000);
	debugstr("system clock = %u\n", new_sys_clock);
	debugstr("pixel clock  = %u\n", pixel_clock);
	debugstr("cc_per_us = %u\n", cc_per_us);
	debugstr("cc_per_px = %u\n", cc_per_px);
	debugstr("px_per_scanline = %u\n", px_per_scanline);
	debugstr("cc_per_scanline = %u\n", cc_per_scanline);
	debugstr("px_per_frame = %u\n", px_per_frame);
	debugstr("cc_per_frame = %u\n", cc_per_frame);
	debugstr("us_per_frame = %u, fract = %u/%u\n", us_per_frame, cc_per_frame_fract, cc_per_us);
}

void VideoBackend::stop() noexcept
{
	assert(get_core_num() == 1);
	assert(state != not_initialized);

	static uint32  two_black_pixels			   = 0;
	static uint32* address_of_two_black_pixels = &two_black_pixels;

	// set data dma to fixed source address:
	dma_channel_hw_t* hw = dma_channel_hw_addr(SCANLINE_DMA_DATA_CHANNEL);
	hw->al1_ctrl		 = hw->al1_ctrl & ~DMA_CH0_CTRL_TRIG_INCR_READ_BITS;

	// set ctrl dma to fixed source address & source address = &address_of_two_black_pixels:
	hw			  = dma_channel_hw_addr(SCANLINE_DMA_CTRL_CHANNEL);
	hw->al1_ctrl  = hw->al1_ctrl & ~DMA_CH0_CTRL_TRIG_INCR_READ_BITS;
	hw->read_addr = uint32(&address_of_two_black_pixels);

	// wait for data dma to read from two_black_pixels:
	hw = dma_channel_hw_addr(SCANLINE_DMA_DATA_CHANNEL);
	while (hw->read_addr != uint32(&two_black_pixels)) {}
}

void VideoBackend::initialize() noexcept
{
	// any core

	TIMING_DMA_CHANNEL		  = uint8(dma_claim_unused_channel(true));
	SCANLINE_DMA_CTRL_CHANNEL = uint8(dma_claim_unused_channel(true));
	SCANLINE_DMA_DATA_CHANNEL = uint8(dma_claim_unused_channel(true));
	TIMING_SM				  = uint8(pio_claim_unused_sm(video_pio, true));
	SCANLINE_SM				  = uint8(pio_claim_unused_sm(video_pio, true));

	initialize_gpio_pins();
	initialize_state_machines();
	irq_set_exclusive_handler(DMA_IRQ_0 + TIMING_DMA_IRQ_IDX, timing_isr);
	state = not_started;
}


} // namespace kio::Video


/*































*/
