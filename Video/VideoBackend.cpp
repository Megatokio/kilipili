// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VideoBackend.h"
#include "Color.h"
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

static_assert(PICO_SCANVIDEO_PIXEL_RSHIFT + PICO_SCANVIDEO_PIXEL_RCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT);
static_assert(PICO_SCANVIDEO_PIXEL_GSHIFT + PICO_SCANVIDEO_PIXEL_GCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT);
static_assert(PICO_SCANVIDEO_PIXEL_BSHIFT + PICO_SCANVIDEO_PIXEL_BCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT);

#define video_pio pio0

constexpr bool ENABLE_VIDEO_RECOVERY = PICO_SCANVIDEO_ENABLE_VIDEO_RECOVERY;
constexpr uint SYNC_PIN_BASE		 = PICO_SCANVIDEO_SYNC_PIN_BASE;
constexpr bool ENABLE_CLOCK_PIN		 = PICO_SCANVIDEO_ENABLE_CLOCK_PIN;
constexpr uint CLOCK_POLARITY		 = PICO_SCANVIDEO_CLOCK_POLARITY;
constexpr bool ENABLE_DEN_PIN		 = PICO_SCANVIDEO_ENABLE_DEN_PIN;
constexpr uint DEN_POLARITY			 = PICO_SCANVIDEO_DEN_POLARITY;
constexpr uint TIMING_DMA_IRQ		 = DMA_IRQ_1; // for timing_dma
constexpr uint TIMING_SM			 = 1u;
constexpr uint TIMING_DMA_CHANNEL	 = PICO_SCANVIDEO_TIMING_DMA_CHANNEL;

constexpr uint	 SCANLINE_DMA_CHANNEL = PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL;
constexpr uint	 DMA_CB_CHANNEL		  = PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL;
constexpr uint	 SCANLINE_SM		  = PICO_SCANVIDEO_SCANLINE_SM1;
constexpr uint32 DMA_CHANNELS_MASK	  = (1u << SCANLINE_DMA_CHANNEL);
constexpr uint32 SM_MASK			  = (1u << SCANLINE_SM);
constexpr bool	 FIXED_FRAGMENT_DMA	  = PICO_SCANVIDEO_FIXED_FRAGMENT_DMA;

// =========================================================

bool in_vblank;
int	 current_frame_start = 0; // rolling index of start of currently displayed frame
int	 current_scanline	 = 0; // rolling index of currently displayed scanline

class ScanlineSM
{
public:
	void setup(const VgaMode* mode) throws;
	void teardown() noexcept;
	void start(); // start or restart
	void stop();

	void waitForVBlank() const noexcept
	{
		while (!in_vblank) { wfe(); }
	}
	void waitForScanline(int scanline) const noexcept
	{
		while (current_scanline - scanline < 0) { wfe(); }
	}

private:
	static void isr_pio0_irq0() noexcept;
	void		prepare_active_scanline() noexcept;
	void		prepare_vblank_scanline() noexcept;
	void		abort_all_scanline_sms() noexcept;
};


static ScanlineSM		   scanline_sm;
static uint				   program_load_offset;
static const pio_program_t scanline_pio_program {video_scanline_program};

struct TimingSM
{
	alignas(16) uint32 prog_active[4];
	alignas(16) uint32 prog_vblank[4];
	alignas(16) uint32 prog_vpulse[4];

	struct
	{
		uint32* program;
		uint32	count;
	} program[4];

	enum State {
		generate_v_active,
		generate_v_frontporch,
		generate_v_pulse,
		generate_v_backporch,
	};
	uint state = generate_v_pulse;

	uint timing_scanline;

	uint8 video_htiming_load_offset;

	void setup(const VgaMode*);
	void teardown() noexcept;
	void start() noexcept; // start or restart
	void stop() noexcept;

	//private:
	void configure_dma_channel();
	void setup_timings(const VgaMode*);
	void install_pio_program(uint32 pixel_clock_frequency);
	void isr();
};


static TimingSM timing_sm;


// =========================================================


static inline void RAM abort_all_dma_channels() noexcept
{
	// for irq handler only

	dma_hw->abort = DMA_CHANNELS_MASK;

	while (dma_channel_is_busy(SCANLINE_DMA_CHANNEL)) tight_loop_contents();

	// we don't want any pending completion IRQ which may have happened in the interim
	dma_hw->ints0 = DMA_CHANNELS_MASK; // this is probably not required because we don't use this IRQ
}

inline void RAM ScanlineSM::abort_all_scanline_sms() noexcept
{
	// there's a lot to do:
	// abort dma
	// drain pio tx fifo
	// drain pio tx register
	// set pio sm to position of PIO_WAIT_IRQ4

	//	abort_all_dma_channels();

	//constexpr uint32 CLR_IRQ_4 = 0xc044;
	//pio_sm_exec(video_pio, SM1, CLR_IRQ_4);	// clear IRQ4 -> never needed. why?

	const uint jmp = pio_encode_jmp(program_load_offset + video_scanline_wrap_target); // WAIT_IRQ4 position
	//const uint drain = pio_encode_out(pio_null, 32);	// drain the OSR (rp2040 §3.4.7.2)

	pio_sm_clear_fifos(video_pio, SCANLINE_SM); // drain the TX fifo
	pio_sm_exec(video_pio, SCANLINE_SM, jmp);	// goto WAIT_IRQ4 position
	//pio_sm_exec(video_pio, SM1, drain);		// drain the OSR -> blocks & eats one word if OSR is empty (as it should be)
}

inline void RAM TimingSM::isr()
{
	auto& prog = program[state++ & 3];
	dma_channel_transfer_from_buffer_now(TIMING_DMA_CHANNEL, prog.program, prog.count);
}

void RAM isr_dma() noexcept
{
	// DMA complete
	// interrupt for for timing pio
	// triggered when DMA completed and needs refill & restart
	// can be interrupted by scanline interrupt

	if (dma_irqn_get_channel_status(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL))
	{
		dma_irqn_acknowledge_channel(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL);
		timing_sm.isr();
	}
}

inline void RAM ScanlineSM::prepare_active_scanline() noexcept
{
	// called for PIO_IRQ0 at start of hsync for active display scanline
	// highest priority!

	if (ENABLE_VIDEO_RECOVERY) abort_all_scanline_sms();

	in_vblank = false;
	current_scanline += 1;

	auto* fsb = scanline_buffer[current_scanline];

	if constexpr (FIXED_FRAGMENT_DMA)
	{
		dma_channel_hw_addr(SCANLINE_DMA_CHANNEL)->al3_transfer_count = scanline_buffer.width / 2;
		dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig		  = uintptr_t(fsb);
	}
	else dma_channel_transfer_from_buffer_now(SCANLINE_DMA_CHANNEL, fsb, scanline_buffer.width / 2);
}

inline void RAM ScanlineSM::prepare_vblank_scanline() noexcept
{
	// called for PIO IRQ #1 at start of hsync for scanlines in vblank
	// highest priority!

	if (!in_vblank) // first scanline in vblank?
	{
		// we could also abort_all_scanline_sms() to stop runaway SMs:
		if (ENABLE_VIDEO_RECOVERY) { abort_all_dma_channels(); }

		in_vblank			= true;
		current_frame_start = current_scanline;
		__sev();
	}
}

//static
void RAM ScanlineSM::isr_pio0_irq0() noexcept
{
	// scanline pio interrupt at start of each scanline
	// highest priority!

	if (video_pio->irq & 1u)
	{
		// handle PIO_IRQ0 from timing SM
		//	 set at start of hsync
		//	 for active display scanline

		video_pio->irq = 1; // clear irq
		scanline_sm.prepare_active_scanline();
	}

	else //if (video_pio->irq & 2u)
	{
		// handle PIO_IRQ1 from timing SM
		//	 set at start of hsync
		//	 for scanlines in vblank

		video_pio->irq = 2; // clear irq
		scanline_sm.prepare_vblank_scanline();
	}
}

static void setup_gpio_pins()
{
	static_assert(
		PICO_SCANVIDEO_PIXEL_RSHIFT + PICO_SCANVIDEO_PIXEL_RCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT,
		"red bits exceed pins");
	static_assert(
		PICO_SCANVIDEO_PIXEL_GSHIFT + PICO_SCANVIDEO_PIXEL_GCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT,
		"green bits exceed pins");
	static_assert(
		PICO_SCANVIDEO_PIXEL_BSHIFT + PICO_SCANVIDEO_PIXEL_BCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT,
		"blue bits exceed pins");

	constexpr uint32 RMASK = ((1u << PICO_SCANVIDEO_PIXEL_RCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_RSHIFT;
	constexpr uint32 GMASK = ((1u << PICO_SCANVIDEO_PIXEL_GCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_GSHIFT;
	constexpr uint32 BMASK = ((1u << PICO_SCANVIDEO_PIXEL_BCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_BSHIFT;

	uint32 pin_mask = RMASK | GMASK | BMASK;
	for (uint i = PICO_SCANVIDEO_COLOR_PIN_BASE; pin_mask; i++, pin_mask >>= 1)
	{
		if (pin_mask & 1) gpio_set_function(i, GPIO_FUNC_PIO0);
	}
}


template<bool FIXED_FRAGMENT_DMA>
static void configure_dma_channels()
{
	// configue scanline dma:

	dma_channel_config config = dma_channel_get_default_config(SCANLINE_DMA_CHANNEL);

	// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + SCANLINE_SM);

	if (FIXED_FRAGMENT_DMA)
	{
		channel_config_set_chain_to(&config, DMA_CB_CHANNEL);
		channel_config_set_irq_quiet(&config, true);
	}

	dma_channel_configure(
		SCANLINE_DMA_CHANNEL, &config, &video_pio->txf[SCANLINE_SM], nullptr /*later*/, 0 /*later*/, false);

	// configure scanline dma channel CB:

	if (FIXED_FRAGMENT_DMA)
	{
		config = dma_channel_get_default_config(DMA_CB_CHANNEL);
		channel_config_set_write_increment(&config, true);

		// wrap the write at 4 or 8 bytes, so each transfer writes the same 1 or 2 ctrl registers:
		channel_config_set_ring(&config, true, 2);

		dma_channel_configure(
			DMA_CB_CHANNEL, &config,
			// ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
			&dma_channel_hw_addr(SCANLINE_DMA_CHANNEL)->al3_read_addr_trig,
			nullptr, // set later
			// send 1 word to ctrl block of data chain per transfer:
			1, false);
	}
}

void ScanlineSM::teardown() noexcept
{
	// TODO release SM and dma?
}

void ScanlineSM::setup(const VgaMode* mode) throws
{
	assert(get_core_num() == 1);
	assert(mode->width <= mode->h_active);
	assert(mode->height * mode->yscale <= mode->v_active);
	assert(scanline_buffer.is_valid());
	assert(scanline_buffer.width == mode->h_active);

	setup_gpio_pins();

	in_vblank			= false;
	current_frame_start = 0;
	current_scanline	= 0;

	uint sys_clk				  = clock_get_hz(clk_sys);
	uint video_clock_down_times_2 = sys_clk / mode->pixel_clock;

	if (ENABLE_CLOCK_PIN)
	{
		if (video_clock_down_times_2 * mode->pixel_clock != sys_clk)
			throw "System clock must be an even multiple of the requested pixel clock";
	}
	else
	{
		if (video_clock_down_times_2 * mode->pixel_clock != sys_clk) // TODO: check wg. odd multiple
			throw "System clock must be an integer multiple of the requested pixel clock";
	}

	// install program:

	const uint PIO_WAIT_IRQ4 = pio_encode_wait_irq(1, false, 4);
	assert(scanline_pio_program.instructions[video_scanline_wrap_target] == PIO_WAIT_IRQ4);
	program_load_offset = pio_add_program(video_pio, &scanline_pio_program);

	// setup scanline SMs:

	pio_sm_config config = video_scanline_program_get_default_config(program_load_offset);

	pio_sm_set_consecutive_pindirs(
		video_pio, SCANLINE_SM, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT, true);
	sm_config_set_out_pins(&config, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT);
	sm_config_set_out_shift(&config, true, true, 32); // autopull
	sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

	sm_config_set_clkdiv_int_frac(
		&config, uint16(video_clock_down_times_2 / 2), uint8((video_clock_down_times_2 & 1u) << 7u));
	pio_sm_init(video_pio, SCANLINE_SM, program_load_offset, &config); // sm paused

	// configure scanline interrupts:

	// set to highest priority:
	irq_set_priority(PIO0_IRQ_0, 0);

	// PIO_IRQ0 and PIO_IRQ1 can trigger IRQ0 of the video_pio:
	pio_set_irq0_source_mask_enabled(video_pio, (1u << (pis_interrupt0)) | (1u << (pis_interrupt1)), true);
	irq_set_exclusive_handler(PIO0_IRQ_0, isr_pio0_irq0);

	configure_dma_channels<FIXED_FRAGMENT_DMA>();
}

void ScanlineSM::start()
{
	assert(get_core_num() == 1);

	stop();

	uint jmp = pio_encode_jmp(program_load_offset);
	pio_sm_exec(video_pio, SCANLINE_SM, jmp);
	pio_sm_put(video_pio, SCANLINE_SM, scanline_buffer.width - 1);

	irq_set_enabled(PIO0_IRQ_0, true);
	pio_set_sm_mask_enabled(video_pio, SM_MASK, true);
}

void ScanlineSM::stop()
{
	assert(get_core_num() == 1);

	pio_set_sm_mask_enabled(video_pio, SM_MASK, false); // stop scanline state machines
	irq_set_enabled(PIO0_IRQ_0, false);					// disable scanline interrupt
	abort_all_dma_channels();
}


void TimingSM::configure_dma_channel()
{
	dma_channel_config config = dma_channel_get_default_config(TIMING_DMA_CHANNEL);

	// Select timing dma dreq to be TIMING_SM TX FIFO not full:
	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + TIMING_SM);

	// wrap the read at 4 words / 16 bytes, which is the size of the program arrays:
	channel_config_set_ring(&config, false /*read*/, 4 /*log2(16)*/);

	if ((1))
	{
		dma_channel_configure(
			TIMING_DMA_CHANNEL, &config,
			&video_pio->txf[TIMING_SM], // write address
			nullptr /*later*/,			// read address
			0 /*later*/,				// transfer count
			false);						// don't start now
	}
	else
	{
		//dma_channel_set_read_addr(TIMING_DMA_CHANNEL, read_addr, false);
		dma_channel_set_write_addr(TIMING_DMA_CHANNEL, &video_pio->txf[TIMING_SM] /*write_addr*/, false);
		//dma_channel_set_trans_count(TIMING_DMA_CHANNEL, transfer_count, false);
		dma_channel_set_config(TIMING_DMA_CHANNEL, &config, false);
	}
}

static void configure_gpio_pins()
{
	uint32 pin_mask = 3u | (ENABLE_DEN_PIN << 2) | (ENABLE_CLOCK_PIN << 3);

	for (uint i = SYNC_PIN_BASE; pin_mask; i++, pin_mask >>= 1)
	{
		if (pin_mask & 1) gpio_set_function(i, GPIO_FUNC_PIO0);
	}
}

void TimingSM::install_pio_program(uint32 pixel_clock_frequency)
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

	//pio_set_irq1_source_enabled(video_pio, pis_sm0_tx_fifo_not_full + TIMING_SM, true);	// TODO required?
}

void TimingSM::setup_timings(const VgaMode* timing)
{
	constexpr uint SET_IRQ_0 = 0xc000; //  0: irq nowait 0  side 0
	constexpr uint SET_IRQ_1 = 0xc001; //  1: irq nowait 1  side 0
	constexpr uint SET_IRQ_4 = 0xc004; //  2: irq nowait 4  side 0
	constexpr uint CLR_IRQ_4 = 0xc044; //  3: irq clear  4  side 0

	assert(SET_IRQ_0 == video_htiming_states_program_instructions[0]); // display scanline irq
	assert(SET_IRQ_1 == video_htiming_states_program_instructions[1]); // vblank scanline irq
	assert(SET_IRQ_4 == video_htiming_states_program_instructions[2]); // scanline pixels start => start scanline SMs
	assert(CLR_IRQ_4 == video_htiming_states_program_instructions[3]); // clear irq / dummy instr

	assert(SET_IRQ_0 == pio_encode_irq_set(false, 0));
	assert(SET_IRQ_1 == pio_encode_irq_set(false, 1));
	assert(SET_IRQ_4 == pio_encode_irq_set(false, 4));
	assert(CLR_IRQ_4 == pio_encode_irq_clear(false, 4));

	constexpr int TIMING_CYCLE = 3u;
	constexpr int HTIMING_MIN  = TIMING_CYCLE + 1;

	assert(timing->h_active >= HTIMING_MIN);
	assert(timing->h_pulse >= HTIMING_MIN);
	assert(timing->h_back_porch >= HTIMING_MIN);
	assert(timing->h_front_porch >= HTIMING_MIN);
	assert(timing->h_total() % 2 == 0);
	assert(timing->h_pulse % 2 == 0);
	// horizontal timing:

	// bits are read backwards (lsb to msb) by PIO pogram
	// the scanline starts with the HSYNC pulse!

	// polarity mask to toggle out bits, applied to whole cmd:
	const uint32 polarity_mask = uint32(!timing->h_sync_polarity << 29) + uint32(!timing->v_sync_polarity << 30) +
								 uint32(DEN_POLARITY << 31) + uint32(CLOCK_POLARITY << 12);

	constexpr uint32 TIMING_CYCLES = 3u << 16;
#define MK_CMD(CMD, CYCLES, BITS) ((CMD) | (CYCLES - TIMING_CYCLES) | (BITS)) ^ polarity_mask

	constexpr uint32 hsync_bit = 1u << 29;
	constexpr uint32 vsync_bit = 1u << 30;
	constexpr uint32 den_bit   = 1u << 31;

	const uint32 h_frontporch = uint32(timing->h_front_porch) << 16;
	const uint32 h_active	  = uint32(timing->h_active) << 16;
	const uint32 h_backporch  = uint32(timing->h_back_porch) << 16;
	const uint32 h_pulse	  = uint32(timing->h_pulse) << 16;

	// display area:
	prog_active[0] = MK_CMD(SET_IRQ_0, h_pulse, hsync_bit + !vsync_bit);
	prog_active[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + !vsync_bit);
	prog_active[2] = MK_CMD(SET_IRQ_4, h_active, !hsync_bit + !vsync_bit + den_bit);
	prog_active[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + !vsync_bit);

	// vblank, front & back porch:
	prog_vblank[0] = MK_CMD(SET_IRQ_1, h_pulse, hsync_bit + !vsync_bit);
	prog_vblank[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + !vsync_bit);
	prog_vblank[2] = MK_CMD(CLR_IRQ_4, h_active, !hsync_bit + !vsync_bit);
	prog_vblank[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + !vsync_bit);

	// vblank, vsync pulse:
	prog_vpulse[0] = MK_CMD(SET_IRQ_1, h_pulse, hsync_bit + vsync_bit);
	prog_vpulse[1] = MK_CMD(CLR_IRQ_4, h_backporch, !hsync_bit + vsync_bit);
	prog_vpulse[2] = MK_CMD(CLR_IRQ_4, h_active, !hsync_bit + vsync_bit);
	prog_vpulse[3] = MK_CMD(CLR_IRQ_4, h_frontporch, !hsync_bit + vsync_bit);

	// vertical timing:

	const uint v_active		 = timing->v_active;
	const uint v_front_porch = timing->v_front_porch;
	const uint v_pulse		 = timing->v_pulse;
	const uint v_back_porch	 = timing->v_back_porch;

	program[generate_v_active]	   = {.program = prog_active, .count = count_of(prog_active) * v_active};
	program[generate_v_frontporch] = {.program = prog_vblank, .count = count_of(prog_vblank) * v_front_porch};
	program[generate_v_pulse]	   = {.program = prog_vpulse, .count = count_of(prog_vpulse) * v_pulse};
	program[generate_v_backporch]  = {.program = prog_vblank, .count = count_of(prog_vblank) * v_back_porch};
}

void TimingSM::setup(const VgaMode* timing)
{
	assert(get_core_num() == 1);

	configure_gpio_pins();
	configure_dma_channel();
	install_pio_program(timing->pixel_clock);
	setup_timings(timing);
	static bool f = 0;
	if (!f) irq_add_shared_handler(TIMING_DMA_IRQ, isr_dma, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
	f = 1;
}

void TimingSM::teardown() noexcept { TODO(); }

void TimingSM::start() noexcept
{
	assert(get_core_num() == 1);

	// restart timing SM and IRQ
	// can be called while SM is running or stopped
	// SM will restart with vblank

	stop();

	pio_sm_clear_fifos(video_pio, TIMING_SM); // drain TX fifo
	//const uint drain = pio_encode_out(pio_null, 32);
	//pio_sm_exec(video_pio, TIMING_SM, drain);		// drain the OSR
	const uint jmp = pio_encode_jmp(video_htiming_load_offset + video_htiming_offset_entry_point);
	pio_sm_exec(video_pio, TIMING_SM, jmp);
	//constexpr uint CLR_IRQ_4 = 0xc044;  			//  3: irq clear  4  side 0
	//pio_sm_exec(video_pio, TIMING_SM, CLR_IRQ_4);	// clear scanline irq (safety)

	pio_sm_set_enabled(video_pio, TIMING_SM, true); // start SM

	state = generate_v_pulse;

	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL, true); // enable DMA_CHANNEL irqs on DMA_IRQ_0 or 1
	irq_set_enabled(TIMING_DMA_IRQ, true); // enable DMA_IRQ_0 or 1 interrupt on this core
	isr();								   // irq_set_pending(DMA_IRQ);
										   // trigger first irq
}

void TimingSM::stop() noexcept
{
	assert(get_core_num() == 1); // if irq_set_enabled() is called

	// stop timing IRQ and SM

	pio_sm_set_enabled(video_pio, TIMING_SM, false); // stop SM

	//irq_set_enabled(DMA_IRQ, false);					// disable interrupt (good?)
	dma_irqn_set_channel_enabled(TIMING_DMA_IRQ, TIMING_DMA_CHANNEL, false); // disable interrupt source
	dma_channel_abort(TIMING_DMA_CHANNEL);
}

void VideoBackend::initialize() noexcept
{
	pio_claim_sm_mask(video_pio, 0x0f); // claim all because we clear instruction memory
	dma_claim_mask((1u << TIMING_DMA_CHANNEL) | (1u << SCANLINE_DMA_CHANNEL));
}

void VideoBackend::setup(const VgaMode* vga_mode, uint scanline_buffer_count) throws
{
	pio_set_sm_mask_enabled(video_pio, 0x0f, false); // stop all 4 state machines
	pio_clear_instruction_memory(video_pio);

	scanline_buffer.setup(vga_mode, scanline_buffer_count);
	scanline_sm.setup(vga_mode);
	timing_sm.setup(vga_mode);
}

void VideoBackend::teardown() noexcept
{
	scanline_sm.teardown();
	scanline_buffer.teardown();
	timing_sm.teardown();

	pio_set_sm_mask_enabled(video_pio, 0x0f, false); // stop all 4 state machines
	pio_clear_instruction_memory(video_pio);
}

void VideoBackend::start() noexcept
{
	scanline_sm.start();
	sleep_ms(1);
	timing_sm.start();
	pio_clkdiv_restart_sm_mask(
		video_pio,
		(1u << PICO_SCANVIDEO_SCANLINE_SM1) | (1u << PICO_SCANVIDEO_TIMING_SM)); // synchronize fractional divider
}

void VideoBackend::stop() noexcept
{
	timing_sm.stop();
	scanline_sm.stop();
}


} // namespace kio::Video


/*































*/
