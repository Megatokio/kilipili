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
constexpr uint DMA_IRQ				 = DMA_IRQ_1; // for timing_dma
constexpr uint TIMING_SM			 = 1u;
constexpr uint SCANLINE_SM			 = 0u;

constexpr uint	 DMA_CHANNEL		= PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL;
constexpr uint	 DMA_CB_CHANNEL		= PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL;
constexpr uint	 SM					= PICO_SCANVIDEO_SCANLINE_SM1;
constexpr uint32 DMA_CHANNELS_MASK	= (1u << DMA_CHANNEL);
constexpr uint32 SM_MASK			= (1u << SM);
constexpr bool	 FIXED_FRAGMENT_DMA = PICO_SCANVIDEO_FIXED_FRAGMENT_DMA;

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


// =========================================================


static inline void RAM abort_all_dma_channels() noexcept
{
	// for irq handler only

	dma_hw->abort = DMA_CHANNELS_MASK;

	while (dma_channel_is_busy(DMA_CHANNEL)) tight_loop_contents();

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

	pio_sm_clear_fifos(video_pio, SM); // drain the TX fifo
	pio_sm_exec(video_pio, SM, jmp);   // goto WAIT_IRQ4 position
	//pio_sm_exec(video_pio, SM1, drain);		// drain the OSR -> blocks & eats one word if OSR is empty (as it should be)
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
		dma_channel_hw_addr(DMA_CHANNEL)->al3_transfer_count	= scanline_buffer.width / 2;
		dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = uintptr_t(fsb);
	}
	else dma_channel_transfer_from_buffer_now(DMA_CHANNEL, fsb, scanline_buffer.width / 2);
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

	dma_channel_config config = dma_channel_get_default_config(DMA_CHANNEL);

	// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + SM);

	if (FIXED_FRAGMENT_DMA)
	{
		channel_config_set_chain_to(&config, DMA_CB_CHANNEL);
		channel_config_set_irq_quiet(&config, true);
	}

	dma_channel_configure(DMA_CHANNEL, &config, &video_pio->txf[SM], nullptr /*later*/, 0 /*later*/, false);

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
			&dma_channel_hw_addr(DMA_CHANNEL)->al3_read_addr_trig,
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

	pio_sm_set_consecutive_pindirs(video_pio, SM, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT, true);
	sm_config_set_out_pins(&config, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT);
	sm_config_set_out_shift(&config, true, true, 32); // autopull
	sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

	sm_config_set_clkdiv_int_frac(
		&config, uint16(video_clock_down_times_2 / 2), uint8((video_clock_down_times_2 & 1u) << 7u));
	pio_sm_init(video_pio, SM, program_load_offset, &config); // sm paused

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
	pio_sm_exec(video_pio, SM, jmp);
	pio_sm_put(video_pio, SM, scanline_buffer.width - 1);

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

void initialize() noexcept
{
	//
}

void VideoBackend::setup(const VgaMode* vga_mode, uint scanline_buffer_count) throws
{
	scanline_buffer.setup(vga_mode, scanline_buffer_count);
	scanline_sm.setup(vga_mode);
	//
}

void VideoBackend::teardown() noexcept
{
	scanline_sm.teardown();
	scanline_buffer.teardown();
}

void VideoBackend::start() noexcept
{
	scanline_sm.start();
	//
}

void VideoBackend::stop() noexcept
{
	scanline_sm.stop();
	//
}


} // namespace kio::Video


/*































*/
