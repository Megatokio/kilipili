// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// we need O3, also for the included headers!
#pragma GCC push_options
#pragma GCC optimize("O3")

#include "ScanlineSM.h"
#include "Color.h"
#include "ScanlinePioProgram.h"
#include "composable_scanline.h"
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pio.h>


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

#define RAM __attribute__((section(".time_critical.ScanlineSM")))

#define PIO_WAIT_IRQ4 pio_encode_wait_irq(1, false, 4)

constexpr uint DMA_CHANNEL	  = PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL;
constexpr uint DMA_CB_CHANNEL = PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL;

constexpr uint SM = PICO_SCANVIDEO_SCANLINE_SM1;

constexpr uint32 DMA_CHANNELS_MASK = (1u << DMA_CHANNEL);

constexpr uint32 SM_MASK = (1u << SM);

constexpr bool FIXED_FRAGMENT_DMA = PICO_SCANVIDEO_FIXED_FRAGMENT_DMA;

constexpr bool ENABLE_VIDEO_RECOVERY = PICO_SCANVIDEO_ENABLE_VIDEO_RECOVERY;
constexpr bool ENABLE_CLOCK_PIN		 = PICO_SCANVIDEO_ENABLE_CLOCK_PIN;


// =========================================================


struct DMAScanlineBuffer
{
	uint8	 count = 0;			  // number of scanlines in buffer
	uint8	 yscale;			  // repetition of each scanline for lowres screen modes
	uint16	 length;			  // length of scanlines in words
	uint32** scanlines = nullptr; // array of pointers to scanlines, ready for fragment dma

	void setup(VgaMode videomode, uint new_count) throws
	{
		assert(count == 0);							// must not be in use
		assert((new_count & (new_count - 1)) == 0); // must be 2^N

		yscale	  = uint8(videomode.yscale);
		length	  = uint16(videomode.width / 2 + 2);
		scanlines = new uint32*[new_count * yscale];

		for (uint32** z = scanlines; count; count--)
		{
			uint32* sl = new uint32[length];
			uint16* p  = uint16ptr(sl);
			p[0]	   = CMD::RAW_RUN; // cmd
			//p[1]     = p[2];				// 1st pixel
			//p[2]     = uint16(width-3+1);	// count-3 +1 for final black pixel
			//p[3++]   = pixels
			p[length / 2 - 2] = 0;		  // final black pixel (actually transparent)
			p[length / 2 - 1] = CMD::EOL; // end of line; total count of uint16 values must be even!

			for (uint y = 0; y < yscale; y++) *z++ = sl;
		}
	}

	void teardown()
	{
		if (scanlines)
		{
			for (uint32** z = scanlines; count; count--)
			{
				delete[] * z;
				z += yscale;
			}
			delete[] scanlines;
			scanlines = nullptr;
		}
	}
};

static DMAScanlineBuffer dma_scanline_buffer;


// =========================================================

ScanlineSM			scanline_sm;
std::atomic<uint32> scanlines_missed = 0;
VideoQueue			video_queue;

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

	const uint jmp = pio_encode_jmp(wait_index); // goto WAIT_IRQ4 position
	//const uint drain = pio_encode_out(pio_null, 32);	// drain the OSR (rp2040 §3.4.7.2)

	pio_sm_clear_fifos(video_pio, SM); // drain the TX fifo
	pio_sm_exec(video_pio, SM, jmp);   // goto WAIT_IRQ4 position
	//pio_sm_exec(video_pio, SM1, drain);		// drain the OSR -> blocks & eats one word if OSR is empty (as it should be)
}

inline void RAM ScanlineSM::prepare_for_active_scanline() noexcept
{
	// called for PIO_IRQ0 at start of hsync for active display scanline
	// highest priority!

	if (ENABLE_VIDEO_RECOVERY) abort_all_scanline_sms();

	// check for line repetition in low-res modes:
	if (unlikely(y_scale > 1))
	{
		if (--y_repeat_countdown) goto start_dma;
		y_repeat_countdown = y_scale;
	}

	// update current scanline and frame idx:
	current_id.scanline += 1;
	if (unlikely(in_vblank)) // => first scanline of frame
	{
		in_vblank			= false;
		current_id.scanline = 0;
		current_id.frame += 1;
	}

	// dispose old and get next scanline:
	// TODO we could also repeat the last scanline if scanline missed

	if (current_scanline != missing_scanline)
	{
	a:
		video_queue.push_free(); // release the recent scanline
		__sev();
	}

	if (video_queue.full_avail())
	{
		current_scanline = &video_queue.get_full();
		if (current_scanline->id < current_id)
			goto a; // outdated
					// else if the scanline is too early then we display it too early
					// and remain out of sync. but this should not happen.
	}
	else
	{
		current_scanline = missing_scanline;
		scanlines_missed = scanlines_missed + 1; // atomic_fetch_add not implemented, but in the isr this should work
	}

start_dma:

	auto* fsb = current_scanline;

	if (FIXED_FRAGMENT_DMA) dma_channel_hw_addr(DMA_CHANNEL)->al3_transfer_count = fsb->fragment_words;
	if (FIXED_FRAGMENT_DMA) dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = uintptr_t(fsb->data);
	else dma_channel_transfer_from_buffer_now(DMA_CHANNEL, fsb->data, fsb->used);
}

inline void RAM ScanlineSM::prepare_for_vblank_scanline() noexcept
{
	// called for PIO IRQ #1 at start of hsync for scanlines in vblank
	// highest priority!

	if (!in_vblank) // first scanline in vblank?
	{
		if (ENABLE_VIDEO_RECOVERY)
		{
			abort_all_dma_channels(); // we could also abort_all_scanline_sms() to stop runaway SMs
		}

		if (current_scanline != missing_scanline)
		{
			current_scanline = missing_scanline;
			video_queue.push_free(); // release the recent scanline
									 //__sev(); // TODO
		}

		in_vblank		   = true;
		y_repeat_countdown = 1; // => next prepare_for_active_scanline() will read next scanline

		sem_release(&scanline_sm.vblank_begin);
	}
}

static void RAM isr_pio0_irq0()
//extern "C" void __isr inline RAM isr_pio0_0(void)
{
	// scanline pio interrupt at start of each scanline
	// highest priority!

	if (video_pio->irq & 1u)
	{
		// handle PIO_IRQ0 from timing SM
		//	 set at start of hsync
		//	 for active display scanline

		video_pio->irq = 1; // clear irq
		scanline_sm.prepare_for_active_scanline();
	}

	else //if (video_pio->irq & 2u)
	{
		// handle PIO_IRQ1 from timing SM
		//	 set at start of hsync
		//	 for scanlines in vblank

		video_pio->irq = 2; // clear irq
		scanline_sm.prepare_for_vblank_scanline();
	}
}

Scanline* RAM ScanlineSM::getScanlineForGenerating()
{
	if (unlikely(video_queue.free_avail() == 0)) return nullptr;

	ScanlineID& id = last_generated_id;
	id.scanline += 1;

	if (unlikely(id <= current_id)) // scanline missed?
	{
		id = current_id + 1;
	}

	if (unlikely(id.scanline >= video_mode.height)) // next frame?
	{
		id.scanline = 0;
		id.frame += 1;
	}

	Scanline* scanline = &video_queue.get_free();
	scanline->id	   = id;

	return scanline;
}

#pragma GCC pop_options

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

static void configure_sm(uint sm, uint program_load_offset, uint video_clock_down_times_2)
{
	pio_sm_config config = scanline_sm.pio_program->configure_pio(video_pio, sm, program_load_offset);
	sm_config_set_clkdiv_int_frac(
		&config, uint16(video_clock_down_times_2 / 2), uint8((video_clock_down_times_2 & 1u) << 7u));
	pio_sm_init(video_pio, sm, program_load_offset, &config); // sm paused
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

//Error ScanlineSM::setup(const VgaMode* mode, uint32* scanlinebuffer[], int count, int size)
Error ScanlineSM::setup(const VgaMode* mode)
{
	assert(get_core_num() == 1);

	const VgaTiming* timing = mode->timing;
	assert(mode->width * mode->xscale <= timing->h_active);
	assert(mode->height * mode->yscale <= timing->v_active);
	assert((ScanlineID(100, 10) + 5).scanline == 105);

	setup_gpio_pins();

	video_mode			= *mode;
	video_mode.timing	= timing;
	missing_scanline	= pio_program->missing_scanline;
	y_scale				= mode->yscale;
	y_repeat_countdown	= 1;
	current_scanline	= missing_scanline;
	current_id.frame	= 0;
	current_id.scanline = 0;
	last_generated_id	= current_id;
	in_vblank			= false; // true if in the vblank interval
	scanlines_missed	= 0;
	sem_init(&vblank_begin, 0, 1);

	// get the program, modify it as needed and install it:

	uint16		  instructions[32];
	pio_program_t program = pio_program->program;
	memcpy(instructions, program.instructions, program.length * sizeof(uint16));
	program.instructions = instructions;

	pio_program->adapt_for_mode(mode, instructions);
	uint program_load_offset = pio_add_program(video_pio, &program);

	assert(instructions[pio_program->wait_index] == PIO_WAIT_IRQ4);
	wait_index = program_load_offset + pio_program->wait_index;

	// setup scanline SMs:

	uint sys_clk				  = clock_get_hz(clk_sys);
	uint video_clock_down_times_2 = sys_clk / timing->pixel_clock;

	if (ENABLE_CLOCK_PIN)
	{
		if (video_clock_down_times_2 * timing->pixel_clock != sys_clk)
			return "System clock must be an even multiple of the requested pixel clock";
	}
	else
	{
		if (video_clock_down_times_2 * timing->pixel_clock != sys_clk) // TODO: check wg. odd multiple
			return "System clock must be an integer multiple of the requested pixel clock";
	}

	configure_sm(SM, program_load_offset, video_clock_down_times_2);

	// configure scanline interrupts:

	// set to highest priority:
	irq_set_priority(PIO0_IRQ_0, 0);

	// PIO_IRQ0 and PIO_IRQ1 can trigger IRQ0 of the video_pio:
	pio_set_irq0_source_mask_enabled(video_pio, (1u << (pis_interrupt0)) | (1u << (pis_interrupt1)), true);
	irq_set_exclusive_handler(PIO0_IRQ_0, isr_pio0_irq0);

	configure_dma_channels<FIXED_FRAGMENT_DMA>();

	return NO_ERROR;
}

void ScanlineSM::start()
{
	assert(get_core_num() == 1);

	stop();

	uint jmp = pio_encode_jmp(wait_index);
	pio_sm_exec(video_pio, SM, jmp);

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


// =====================================================================


void ScanlineSM::waitForScanline(ScanlineID scanline)
{
	while (scanline_sm.current_id < scanline) { __wfe(); }
}

bool ScanlineSM::in_hblank()
{
	// if scanline pio is waiting for IRQ4 then it is not generating pixels:

	return video_pio->sm[SM].instr == PIO_WAIT_IRQ4;
}


} // namespace kio::Video
