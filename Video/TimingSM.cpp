// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// we need O3, also for the included headers!
#include <string.h>
#pragma GCC push_options
#pragma GCC optimize("O3")

#include "TimingSM.h"
#include "VgaMode.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "scanvideo_options.h"
#include "timing.pio.h"
#include <hardware/pio.h>


namespace kio::Video
{

#define RAM __attribute__((section(".time_critical.TimingSM")))

constexpr uint DMA_CHANNEL		= PICO_SCANVIDEO_TIMING_DMA_CHANNEL;
constexpr uint TIMING_SM		= PICO_SCANVIDEO_TIMING_SM;
constexpr uint SYNC_PIN_BASE	= PICO_SCANVIDEO_SYNC_PIN_BASE;
constexpr bool ENABLE_CLOCK_PIN = PICO_SCANVIDEO_ENABLE_CLOCK_PIN;
constexpr uint CLOCK_POLARITY	= PICO_SCANVIDEO_CLOCK_POLARITY;
constexpr bool ENABLE_DEN_PIN	= PICO_SCANVIDEO_ENABLE_DEN_PIN;
constexpr uint DEN_POLARITY		= PICO_SCANVIDEO_DEN_POLARITY;
constexpr uint DMA_IRQ			= DMA_IRQ_1;


// =========================================================

TimingSM timing_sm;

// =========================================================

inline void RAM TimingSM::isr()
{
	auto& prog = program[state++ & 3];
	dma_channel_transfer_from_buffer_now(DMA_CHANNEL, prog.program, prog.count);
}

void RAM isr_dma() noexcept
{
	// DMA complete
	// interrupt for for timing pio
	// triggered when DMA completed and needs refill & restart
	// can be interrupted by scanline interrupt

	if (dma_irqn_get_channel_status(DMA_IRQ, DMA_CHANNEL))
	{
		dma_irqn_acknowledge_channel(DMA_IRQ, DMA_CHANNEL);
		timing_sm.isr();
	}
}


#pragma GCC pop_options

// =========================================================


void TimingSM::configure_dma_channel()
{
	dma_channel_config config = dma_channel_get_default_config(DMA_CHANNEL);

	// Select timing dma dreq to be TIMING_SM TX FIFO not full:
	channel_config_set_dreq(&config, DREQ_PIO0_TX0 + TIMING_SM);

	// wrap the read at 4 words / 16 bytes, which is the size of the program arrays:
	channel_config_set_ring(&config, false /*read*/, 4 /*log2(16)*/);

	if ((1))
	{
		dma_channel_configure(
			DMA_CHANNEL, &config,
			&video_pio->txf[TIMING_SM], // write address
			nullptr /*later*/,			// read address
			0 /*later*/,				// transfer count
			false);						// don't start now
	}
	else
	{
		//dma_channel_set_read_addr(DMA_CHANNEL, read_addr, false);
		dma_channel_set_write_addr(DMA_CHANNEL, &video_pio->txf[TIMING_SM] /*write_addr*/, false);
		//dma_channel_set_trans_count(DMA_CHANNEL, transfer_count, false);
		dma_channel_set_config(DMA_CHANNEL, &config, false);
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
	if (!f) irq_add_shared_handler(DMA_IRQ, isr_dma, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
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

	dma_irqn_set_channel_enabled(DMA_IRQ, DMA_CHANNEL, true); // enable DMA_CHANNEL irqs on DMA_IRQ_0 or 1
	irq_set_enabled(DMA_IRQ, true);							  // enable DMA_IRQ_0 or 1 interrupt on this core
	isr();													  // irq_set_pending(DMA_IRQ);
															  // trigger first irq
}

void TimingSM::stop() noexcept
{
	assert(get_core_num() == 1); // if irq_set_enabled() is called

	// stop timing IRQ and SM

	pio_sm_set_enabled(video_pio, TIMING_SM, false); // stop SM

	//irq_set_enabled(DMA_IRQ, false);					// disable interrupt (good?)
	dma_irqn_set_channel_enabled(DMA_IRQ, DMA_CHANNEL, false); // disable interrupt source
	dma_channel_abort(DMA_CHANNEL);
}


} // namespace kio::Video
