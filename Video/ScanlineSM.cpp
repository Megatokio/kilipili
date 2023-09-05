// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// we need O3, also for the included headers!
#pragma GCC push_options
#pragma GCC optimize("O3")

#include "Graphics/Color.h"
#include "ScanlineSM.h"
#include "ScanlinePioProgram.h"
#include "ScanlinePioProgram.h"
#include "composable_scanline.h"
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <hardware/irq.h>
#include <hardware/clocks.h>


namespace kipili::Video
{

#define pio0_hw ((pio_hw_t *)PIO0_BASE)					// assert definition hasn't changed
#undef  pio0_hw											// undef c-style definition
#define pio0_hw reinterpret_cast<pio_hw_t*>(PIO0_BASE)	// replace with c++-style definition

#define dma_hw ((dma_hw_t *)DMA_BASE)					// assert definition hasn't changed
#undef  dma_hw											// undef c-style definition
#define dma_hw reinterpret_cast<dma_hw_t*>(DMA_BASE)	// replace with c++-style definition

#define RAM __attribute__((section(".time_critical.ScanlineSM")))

#define PIO_WAIT_IRQ4 pio_encode_wait_irq(1, false, 4)

constexpr uint DMA_CHANNEL1    = PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL1;
constexpr uint DMA_CHANNEL2    = PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL2;
constexpr uint DMA_CHANNEL3    = PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL3;
constexpr uint DMA_CB_CHANNEL1 = PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL1;
constexpr uint DMA_CB_CHANNEL2 = PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL2;
constexpr uint DMA_CB_CHANNEL3 = PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL3;

constexpr uint SM1	= PICO_SCANVIDEO_SCANLINE_SM1;
constexpr uint SM2	= PICO_SCANVIDEO_SCANLINE_SM2;
constexpr uint SM3	= PICO_SCANVIDEO_SCANLINE_SM3;

constexpr uint PLANE_COUNT = PICO_SCANVIDEO_PLANE_COUNT;

constexpr uint32 DMA_CHANNELS_MASK = (1u << DMA_CHANNEL1) |
		((PLANE_COUNT >= 2) << DMA_CHANNEL2) |
		((PLANE_COUNT >= 3) << DMA_CHANNEL3);

constexpr uint32 SM_MASK = (1u << SM1) |
		((PLANE_COUNT >= 2) << SM2) |
		((PLANE_COUNT >= 3) << SM3);

constexpr bool VARIABLE_FRAGMENT_DMA1 = PICO_SCANVIDEO_PLANE1_VARIABLE_FRAGMENT_DMA;
constexpr bool FIXED_FRAGMENT_DMA1    = PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA;
constexpr bool VARIABLE_FRAGMENT_DMA2 = PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA;
constexpr bool FIXED_FRAGMENT_DMA2    = PICO_SCANVIDEO_PLANE2_FIXED_FRAGMENT_DMA;
constexpr bool VARIABLE_FRAGMENT_DMA3 = PICO_SCANVIDEO_PLANE3_VARIABLE_FRAGMENT_DMA;
constexpr bool FIXED_FRAGMENT_DMA3    = PICO_SCANVIDEO_PLANE3_FIXED_FRAGMENT_DMA;

constexpr bool FRAGMENT_DMA1 = (VARIABLE_FRAGMENT_DMA1 || FIXED_FRAGMENT_DMA1);
constexpr bool FRAGMENT_DMA2 = (VARIABLE_FRAGMENT_DMA2 || FIXED_FRAGMENT_DMA2) && PLANE_COUNT >= 2;
constexpr bool FRAGMENT_DMA3 = (VARIABLE_FRAGMENT_DMA3 || FIXED_FRAGMENT_DMA3) && PLANE_COUNT >= 3;

constexpr bool ENABLE_VIDEO_RECOVERY = PICO_SCANVIDEO_ENABLE_VIDEO_RECOVERY;
constexpr bool ENABLE_CLOCK_PIN = PICO_SCANVIDEO_ENABLE_CLOCK_PIN;


// =========================================================

ScanlineSM scanline_sm;
std::atomic<uint32> scanlines_missed = 0;
VideoQueue video_queue;

// =========================================================


static inline void RAM abort_all_dma_channels() noexcept
{
	// for irq handler only

	dma_hw->abort = DMA_CHANNELS_MASK;

	while (dma_channel_is_busy(DMA_CHANNEL1)) tight_loop_contents();
	while (PLANE_COUNT >= 2 && dma_channel_is_busy(DMA_CHANNEL2)) tight_loop_contents();
	while (PLANE_COUNT >= 3 && dma_channel_is_busy(DMA_CHANNEL3)) tight_loop_contents();

	// we don't want any pending completion IRQ which may have happened in the interim
	dma_hw->ints0 = DMA_CHANNELS_MASK;	// this is probably not required because we don't use this IRQ
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

	const uint jmp = pio_encode_jmp(wait_index);		// goto WAIT_IRQ4 position
	//const uint drain = pio_encode_out(pio_null, 32);	// drain the OSR (rp2040 §3.4.7.2)

	pio_sm_clear_fifos(video_pio, SM1);			// drain the TX fifo
	pio_sm_exec(video_pio, SM1, jmp);			// goto WAIT_IRQ4 position
	//pio_sm_exec(video_pio, SM1, drain);		// drain the OSR -> blocks & eats one word if OSR is empty (as it should be)

	if (PLANE_COUNT >= 2)
	{
		pio_sm_clear_fifos(video_pio, SM2);
		pio_sm_exec(video_pio, SM2, jmp);
		//pio_sm_exec(video_pio, SM2, drain);
	}

	if (PLANE_COUNT >= 3)
	{
		pio_sm_clear_fifos(video_pio, SM3);
		pio_sm_exec(video_pio, SM3, jmp);
		//pio_sm_exec(video_pio, SM3, drain);
	}
}

inline void RAM ScanlineSM::prepare_for_active_scanline() noexcept
{
	// called for PIO_IRQ0 at start of hsync for active display scanline
	// highest priority!

	if (ENABLE_VIDEO_RECOVERY)
		abort_all_scanline_sms();

	// check for line repetition in low-res modes:
	if (unlikely(y_scale > 1))
	{
		if (--y_repeat_countdown) goto start_dma;
		y_repeat_countdown = y_scale;
	}

	// update current scanline and frame idx:
	current_id.scanline += 1;
	if (unlikely(in_vblank))	// => first scanline of frame
	{
		in_vblank = false;
		current_id.scanline = 0;
		current_id.frame += 1;
	}

	// dispose old and get next scanline:
	// TODO we could also repeat the last scanline if scanline missed

	if (current_scanline != missing_scanline)
	{
a:		video_queue.push_free();		// release the recent scanline
		//__sev(); // TODO
	}

	if (video_queue.full_avail())
	{
		current_scanline = &video_queue.get_full();
		if (current_scanline->id < current_id) goto a; // outdated
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

	if (FIXED_FRAGMENT_DMA1) dma_channel_hw_addr(DMA_CHANNEL1)->al3_transfer_count = fsb->fragment_words;
	if (FRAGMENT_DMA1) dma_channel_hw_addr(DMA_CB_CHANNEL1)->al3_read_addr_trig = uintptr_t(fsb->data[0].data);
	else dma_channel_transfer_from_buffer_now(DMA_CHANNEL1, fsb->data[0].data, fsb->data[0].used);

	if (PLANE_COUNT >= 2)
	{
		if (FIXED_FRAGMENT_DMA2) dma_channel_hw_addr(DMA_CHANNEL2)->al3_transfer_count = fsb->fragment_words;
		if (FRAGMENT_DMA2) dma_channel_hw_addr(DMA_CB_CHANNEL2)->al3_read_addr_trig = uintptr_t(fsb->data[1].data);
		else dma_channel_transfer_from_buffer_now(DMA_CHANNEL2, fsb->data[1].data, fsb->data[1].used);
	}

	if (PLANE_COUNT >= 3)
	{
		if (FIXED_FRAGMENT_DMA3) dma_channel_hw_addr(DMA_CHANNEL3)->al3_transfer_count = fsb->fragment_words;
		if (FRAGMENT_DMA3) dma_channel_hw_addr(DMA_CB_CHANNEL3)->al3_read_addr_trig = uintptr_t(fsb->data[2].data);
		else dma_channel_transfer_from_buffer_now(DMA_CHANNEL3, fsb->data[2].data, fsb->data[2].used);
	}
}

inline void RAM ScanlineSM::prepare_for_vblank_scanline() noexcept
{
	// called for PIO IRQ #1 at start of hsync for scanlines in vblank
	// highest priority!

	if (!in_vblank) // first scanline in vblank?
	{
		if (ENABLE_VIDEO_RECOVERY)
		{
			abort_all_dma_channels();	// we could also abort_all_scanline_sms() to stop runaway SMs
		}

		if (current_scanline != missing_scanline)
		{
			current_scanline = missing_scanline;
			video_queue.push_free();	// release the recent scanline
			//__sev(); // TODO
		}

		in_vblank = true;
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

		video_pio->irq = 1;	// clear irq
		scanline_sm.prepare_for_active_scanline();
	}

	else //if (video_pio->irq & 2u)
	{
		// handle PIO_IRQ1 from timing SM
		//	 set at start of hsync
		//	 for scanlines in vblank

		video_pio->irq = 2;	// clear irq
		scanline_sm.prepare_for_vblank_scanline();
	}

	//else
	//{
	//	panic("unexpected pio0 irq");
	//}
}

Scanline* RAM ScanlineSM::getScanlineForGenerating()
{
	if (unlikely(video_queue.free_avail() == 0)) return nullptr;

	ScanlineID& id = last_generated_id; id.scanline += 1;

	if (unlikely(id <= current_id))		// scanline missed?
	{
		id = current_id + 1;
	}

	if (unlikely(id.scanline >= video_mode.height))  // next frame?
	{
		id.scanline = 0;
		id.frame += 1;
	}

	Scanline* scanline = &video_queue.get_free();
	scanline->id = id;

	return scanline;
}

#pragma GCC pop_options

static void setup_gpio_pins()
{
	static_assert(PICO_SCANVIDEO_PIXEL_RSHIFT + PICO_SCANVIDEO_PIXEL_RCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT, "red bits exceed pins");
	static_assert(PICO_SCANVIDEO_PIXEL_GSHIFT + PICO_SCANVIDEO_PIXEL_GCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT, "green bits exceed pins");
	static_assert(PICO_SCANVIDEO_PIXEL_BSHIFT + PICO_SCANVIDEO_PIXEL_BCOUNT <= PICO_SCANVIDEO_COLOR_PIN_COUNT, "blue bits exceed pins");

	constexpr uint32 RMASK = ((1u << PICO_SCANVIDEO_PIXEL_RCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_RSHIFT;
	constexpr uint32 GMASK = ((1u << PICO_SCANVIDEO_PIXEL_GCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_GSHIFT;
	constexpr uint32 BMASK = ((1u << PICO_SCANVIDEO_PIXEL_BCOUNT) - 1u) << PICO_SCANVIDEO_PIXEL_BSHIFT;

	uint32 pin_mask = RMASK|GMASK|BMASK;
	for (uint i = PICO_SCANVIDEO_COLOR_PIN_BASE; pin_mask; i++, pin_mask >>= 1)
	{
		if (pin_mask & 1) gpio_set_function(i, GPIO_FUNC_PIO0);
	}
}

static void configure_sm(uint sm, uint program_load_offset, uint video_clock_down_times_2)
{
	pio_sm_config config = scanline_sm.pio_program->configure_pio(video_pio, sm, program_load_offset);
	sm_config_set_clkdiv_int_frac(&config, uint16(video_clock_down_times_2 / 2), uint8((video_clock_down_times_2 & 1u) << 7u));
	pio_sm_init(video_pio, sm, program_load_offset, &config); // sm paused
}

template<bool FIXED_FRAGMENT_DMA, bool VARIABLE_FRAGMENT_DMA>
static void configure_dma_channels(uint SM, uint DMA_CHANNEL, uint DMA_CB_CHANNEL)
{
	// configue scanline dma:

	dma_channel_config config = dma_channel_get_default_config(DMA_CHANNEL);

	// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + SM);

	if (FIXED_FRAGMENT_DMA || VARIABLE_FRAGMENT_DMA)
	{
		channel_config_set_chain_to(&config, DMA_CB_CHANNEL);
		channel_config_set_irq_quiet(&config, true);
	}

	dma_channel_configure(DMA_CHANNEL, &config, &video_pio->txf[SM], nullptr/*later*/, 0/*later*/, false);

	// configure scanline dma channel CB:

	if (FIXED_FRAGMENT_DMA || VARIABLE_FRAGMENT_DMA)
	{
		config = dma_channel_get_default_config(DMA_CB_CHANNEL);
		channel_config_set_write_increment(&config, true);

		// wrap the write at 4 or 8 bytes, so each transfer writes the same 1 or 2 ctrl registers:
		channel_config_set_ring(&config, true, VARIABLE_FRAGMENT_DMA ? 3 : 2);

		dma_channel_configure(DMA_CB_CHANNEL,
				&config,
				VARIABLE_FRAGMENT_DMA ?
					// ch DMA config (target "ring" buffer size 8) - this is (transfer_count, read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL)->al3_transfer_count :
					// ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL)->al3_read_addr_trig,
				nullptr, // set later
				// send 1 or 2 words to ctrl block of data chain per transfer:
				VARIABLE_FRAGMENT_DMA ? 2 : 1,
				false);
	}
}

/*void ScanlineSM::configure_dma()
{
	//irq_set_priority(DMA_IRQ_0, 0x40);	// higher than other crud
	//dma_set_irq0_channel_mask_enabled(DMA_CHANNELS_MASK, true);

	// todo reset DMA channels

	// configue scanline dma:
	{
		dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL1);

		// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
		channel_config_set_dreq(&channel_config, DREQ_PIO0_TX0 + SM1);

		if (FRAGMENT_DMA1)
		{
			channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL1);
			channel_config_set_irq_quiet(&channel_config, true);
		}

		dma_channel_configure(DMA_CHANNEL1,
							  &channel_config,
							  &video_pio->txf[SM1],
							  nullptr, // set later
							  0, // set later
							  false);
	}

	// configure scanline dma channel CB:
	if (FRAGMENT_DMA1)
	{
		dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL1);
		channel_config_set_write_increment(&chain_config, true);

		// wrap the write at 4 or 8 bytes, so each transfer writes the same 1 or 2 ctrl registers:
		channel_config_set_ring(&chain_config, true, VARIABLE_FRAGMENT_DMA1 ? 3 : 2);

		dma_channel_configure(DMA_CB_CHANNEL1,
				&chain_config,
				VARIABLE_FRAGMENT_DMA1 ?
					// ch DMA config (target "ring" buffer size 8) - this is (transfer_count, read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL1)->al3_transfer_count :
					// ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL1)->al3_read_addr_trig,
				nullptr, // set later
				// send 1 or 2 words to ctrl block of data chain per transfer:
				VARIABLE_FRAGMENT_DMA1 ? 2 : 1,
				false);
	}

	// configure scanline dma for plane 2:
	if (PLANE_COUNT >= 2)
	{
		dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL2);

		// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
		channel_config_set_dreq(&channel_config, DREQ_PIO0_TX0 + SM2);

		if (FRAGMENT_DMA2)
		{
			channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL2);
			channel_config_set_irq_quiet(&channel_config, true);
		}

		dma_channel_configure(DMA_CHANNEL2,
					  &channel_config,
					  &video_pio->txf[SM2],
					  nullptr,	// set later
					  0,		// set later
					  false);
	}

	// configure scanline dma channel CB for plane 2:
	if (FRAGMENT_DMA2)
	{
		dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL2);
		channel_config_set_write_increment(&chain_config, true);

		// wrap the write at 4 or 8 bytes, so each transfer writes the same 1 or 2 ctrl registers:
		channel_config_set_ring(&chain_config, true, VARIABLE_FRAGMENT_DMA2 ? 3 : 2);

		dma_channel_configure(DMA_CB_CHANNEL2,
				&chain_config,
				VARIABLE_FRAGMENT_DMA2 ?
					// ch DMA config (target "ring" buffer size 8) - this is (transfer_count, read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL2)->al3_transfer_count :
					// ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
					&dma_channel_hw_addr(DMA_CHANNEL2)->al3_read_addr_trig,
				nullptr, // set later
				// send 1 or 2 words to ctrl block of data chain per transfer:
				VARIABLE_FRAGMENT_DMA2 ? 2 : 1,
				false);
	}

	// configure scanline dma for plane 3:
	if (PLANE_COUNT >= 3)
	{
		static_assert(!FRAGMENT_DMA3);
		dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL3);

		// Select scanline dma dreq to be SCANLINE_SM TX FIFO not full:
		channel_config_set_dreq(&channel_config, DREQ_PIO0_TX0 + SM3);

		dma_channel_configure(DMA_CHANNEL3,
				  &channel_config,
				  &video_pio->txf[SM3],
				  nullptr, // set later
				  0, // set later
				  false);
	}
}*/

Error ScanlineSM::setup(const VgaMode* mode, const VgaTiming* timing)
{
	assert(mode->width * mode->xscale <= timing->h_active);
	assert(mode->height * mode->yscale <= timing->v_active);
	assert((ScanlineID(100,10)+5).scanline == 105);

	setup_gpio_pins();

	video_mode = *mode;
	video_mode.default_timing = timing;
	missing_scanline = pio_program->missing_scanline;
	y_scale = mode->yscale;
	y_repeat_countdown = 1;
	current_scanline = missing_scanline;
	current_id.frame = 0;
	current_id.scanline = 0;
	last_generated_id = current_id;
	in_vblank = false;				// true if in the vblank interval
	scanlines_missed = 0;
	sem_init(&vblank_begin, 0, 1);

	// get the program, modify it as needed and install it:

	uint16 instructions[32];
	pio_program_t program = pio_program->program;
	memcpy(instructions, program.instructions, program.length * sizeof(uint16));
	program.instructions = instructions;

	pio_program->adapt_for_mode(mode, instructions);
	uint program_load_offset = pio_add_program(video_pio, &program);

	assert(instructions[pio_program->wait_index] == PIO_WAIT_IRQ4);
	wait_index = program_load_offset + pio_program->wait_index;

	// setup scanline SMs:

	uint sys_clk = clock_get_hz(clk_sys);
	uint video_clock_down_times_2 = sys_clk / timing->pixel_clock;

	if (ENABLE_CLOCK_PIN)
	{
		if (video_clock_down_times_2 * timing->pixel_clock != sys_clk)
			return "System clock must be an even multiple of the requested pixel clock";
	}
	else
	{
		if (video_clock_down_times_2 * timing->pixel_clock != sys_clk)	// TODO: check wg. odd multiple
			return "System clock must be an integer multiple of the requested pixel clock";
	}

	configure_sm(SM1, program_load_offset, video_clock_down_times_2);
	if (PLANE_COUNT >= 2) configure_sm(SM2, program_load_offset, video_clock_down_times_2);
	if (PLANE_COUNT >= 3) configure_sm(SM3, program_load_offset, video_clock_down_times_2);

	// configure scanline interrupts:

	// set to highest priority:
	irq_set_priority(PIO0_IRQ_0, 0);

	// PIO_IRQ0 and PIO_IRQ1 can trigger IRQ0 of the video_pio:
	pio_set_irq0_source_mask_enabled(video_pio, (1u << (pis_interrupt0)) | (1u << (pis_interrupt1)), true);
	irq_set_exclusive_handler(PIO0_IRQ_0, isr_pio0_irq0);

	configure_dma_channels<FIXED_FRAGMENT_DMA1, VARIABLE_FRAGMENT_DMA1>(SM1, DMA_CHANNEL1, DMA_CB_CHANNEL1);
	if (PLANE_COUNT >= 2) configure_dma_channels<FIXED_FRAGMENT_DMA2, VARIABLE_FRAGMENT_DMA2>(SM2, DMA_CHANNEL2, DMA_CB_CHANNEL2);
	if (PLANE_COUNT >= 3) configure_dma_channels<FIXED_FRAGMENT_DMA3, VARIABLE_FRAGMENT_DMA3>(SM3, DMA_CHANNEL3, DMA_CB_CHANNEL3);

	return NO_ERROR;
}

void ScanlineSM::start()
{
	stop();

	uint jmp = pio_encode_jmp(wait_index);
	pio_sm_exec(video_pio, SM1, jmp);
	if (PLANE_COUNT >= 2) pio_sm_exec(video_pio, SM2, jmp);
	if (PLANE_COUNT >= 3) pio_sm_exec(video_pio, SM3, jmp);

	irq_set_enabled(PIO0_IRQ_0, true);
	pio_set_sm_mask_enabled(video_pio, SM_MASK, true);
}

void ScanlineSM::stop()
{
	pio_set_sm_mask_enabled(video_pio, SM_MASK, false);	// stop scanline state machines
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

	return video_pio->sm[SM1].instr == PIO_WAIT_IRQ4;
}


} // namespace










