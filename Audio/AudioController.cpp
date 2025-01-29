// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "AudioController.h"
#include "AudioSource.h"
#include "audio_options.h"
#include "i2s_audio.pio.h"
#include "pwm_audio.pio.h"
#include "sid_audio.pio.h"
#include "utilities.h"
#include "utilities/system_clock.h"
#include <cstdio>
#include <cstring>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/sync.h>
#include <new>


namespace kio::Audio
{

#if audio_hw_num_channels != 0

// clang-format off
  #define pio0_hw ((pio_hw_t *)PIO0_BASE)				 // assert definition hasn't changed
  #undef pio0_hw										 // undef c-style definition
  #define pio0_hw reinterpret_cast<pio_hw_t*>(PIO0_BASE) // replace with c++-style definition
  
  #define dma_hw ((dma_hw_t *)DMA_BASE)					 // assert definition hasn't changed
  #undef dma_hw											 // undef c-style definition
  #define dma_hw reinterpret_cast<dma_hw_t*>(DMA_BASE)	 // replace with c++-style definition
// clang-format on

  #define WRAP(X)  #X
  #define XWRAP(X) WRAP(X)
  #define XRAM	   __attribute__((section(".scratch_x.VB" XWRAP(__LINE__))))	 // the 4k page with the core1 stack
  #define RAM	   __attribute__((section(".time_critical.VB" XWRAP(__LINE__)))) // general ram

// =========================================================


  #define audio_pio pio0 // same as video pio

constexpr uint dma_buffer_num_frames = AUDIO_DMA_BUFFER_NUM_FRAMES;
constexpr uint dma_buffer_frame_size = audio_hw_sample_size * (audio_hw == I2S ? 2 : 1);
constexpr uint dma_buffer_size		 = dma_buffer_num_frames * dma_buffer_frame_size;

static_assert((dma_buffer_num_frames & (dma_buffer_num_frames - 1)) == 0, "dma buffer size must be 2^N");
static_assert(dma_buffer_num_frames >= 64, "dma buffer size too small");
static_assert(dma_buffer_num_frames <= 4096, "dma buffer_size too big"); // some calculations overflow

constexpr uint cc_per_sample_pwm = (255 + 9) * 16;
constexpr uint cc_per_sample_sid = 260;
constexpr uint cc_per_sample_i2s = 64;

constexpr uint dma_irqn			= 0; // note: VIDEO uses DMA_IRQ_1
constexpr bool has_second_sm	= hw_num_channels >= 2 && audio_hw != I2S;
constexpr uint num_sm			= 1 + has_second_sm;
constexpr uint dma_num_channels = num_sm;

static uint8		sm[num_sm];
static uint8		dma_channel[dma_num_channels];
static spin_lock_t* spinlock = nullptr;

static uint32 dma_buffer[dma_num_channels][dma_buffer_size / 4];
static uint	  dma_wi = 0;

static constexpr uint		max_sources = 8;
static uint					num_sources = 0;
static RCPtr<HwAudioSource> audio_sources[max_sources];

  #if defined PICO_AUDIO_SIGMA_DELTA
static int sid_last_sample[hw_num_channels];
  #else
extern int* sid_last_sample; // dummy, syntax only
  #endif

static constexpr uint32 dither_table[16] = {
	0b0000000000000000u << 16, // 0/16
	0b0000000010000000u << 16, // 1/16
	0b0000100000010000u << 16, // 2/16
	0b0001000010000100u << 16, // 3/16
	0b0010001000100010u << 16, // 4/16
	0b0010010010010010u << 16, // 5/16
	0b0010101001010010u << 16, // 6/16
	0b0101010100101010u << 16, // 7/16
	0b1010101010101010u << 16, // 8/16
	0b1010101011010101u << 16, // 9/16
	0b1010110110101101u << 16, // 10/16
	0b1011011011011011u << 16, // 11/16
	0b1011101110111011u << 16, // 12/16
	0b1101111011110111u << 16, // 13/16
	0b1110111111101111u << 16, // 14/16
	0b1111111011111111u << 16, // 15/16
};

static float		 requested_sample_frequency = AUDIO_DEFAULT_SAMPLE_FREQUENCY;
float				 hw_sample_frequency		= AUDIO_DEFAULT_SAMPLE_FREQUENCY; // actual sample frequency
static float		 max_latency				= 10e-3f;
static uint			 max_frames_ahead			= dma_buffer_num_frames - 1;
static volatile bool check_timing				= false;
static bool			 is_running					= false;
static alarm_id_t	 timer_id					= 0;
static uint32		 timer_period_us			= 0;

struct Locker // lock for audio_sources[]
{
	uint32 state;
	Locker() noexcept { state = spin_lock_blocking(spinlock); }
	~Locker() noexcept { spin_unlock(spinlock, state); }
};


// =========================================================

static void init_pio() noexcept
{
	if constexpr (audio_hw == I2S)
	{
		uint load_offset = uint(pio_add_program(audio_pio, &i2s_audio_program));
		uint entry_point = load_offset + i2s_audio_offset_entry_point;

		pio_sm_config config = i2s_audio_program_get_default_config(load_offset);
		sm_config_set_out_pins(&config, audio_i2s_data_pin, 1);
		sm_config_set_sideset_pins(&config, audio_i2s_clock_pin_base);
		sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
		sm_config_set_out_shift(&config, false, true, 16 + 16); // shift left, auto pull, 32 bit
		pio_sm_init(audio_pio, sm[0], entry_point, &config);

		constexpr uint pin_mask = (1u << audio_i2s_data_pin) | (3u << audio_i2s_clock_pin_base);
		pio_sm_set_pindirs_with_mask(audio_pio, sm[0], pin_mask, pin_mask);
		pio_sm_set_pins(audio_pio, sm[0], 0); // clear pins

		pio_sm_exec(audio_pio, sm[0], pio_encode_jmp(entry_point));
	}
	if constexpr (audio_hw == PWM)
	{
		uint load_offset = uint(pio_add_program(audio_pio, &pwm_audio_program));
		uint entry_point = load_offset + pwm_audio_offset_entry_point;

		pio_sm_config config = pwm_audio_program_get_default_config(load_offset);
		sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
		sm_config_set_out_shift(&config, true, false, 8 + 8 + 16); // shift right, no autopull, 32 bits
		//sm_config_set_clkdiv_int_frac(&config,1,0);

		for (uint i = 0; i < num_sm; i++)
		{
			constexpr uint8 pins[2] = {audio_left_pin, audio_right_pin};
			sm_config_set_out_pins(&config, pins[i], 1);
			sm_config_set_sideset_pins(&config, pins[i]);
			pio_sm_init(audio_pio, sm[i], entry_point, &config);
			pio_sm_set_consecutive_pindirs(audio_pio, sm[i], pins[i], 1, true);
			pio_sm_exec(audio_pio, sm[i], pio_encode_jmp(entry_point));
		}
	}
	if constexpr (audio_hw == SIGMA_DELTA) // TODO
	{
		uint load_offset = uint(pio_add_program(audio_pio, &sid_audio_program));
		uint entry_point = load_offset + sid_audio_offset_entry_point;

		pio_sm_config config = sid_audio_program_get_default_config(load_offset);
		sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
		sm_config_set_out_shift(&config, false, true, 8 + 8 + 8 + 8); // shift left, autopull, 32 bits

		for (uint i = 0; i < num_sm; i++)
		{
			constexpr uint8 pins[2] = {audio_left_pin, audio_right_pin};
			sm_config_set_out_pins(&config, pins[i], 1);
			sm_config_set_sideset_pins(&config, pins[i]);
			pio_sm_init(audio_pio, sm[i], entry_point, &config);
			pio_sm_set_consecutive_pindirs(audio_pio, sm[i], pins[i], 1, true);

			// load the const value '127' into the ISR register:
			pio_sm_put(audio_pio, sm[i], 127);							// put '127' into the tx fifo
			pio_sm_exec(audio_pio, sm[i], pio_encode_out(pio_isr, 32)); // load it into isr

			// jump to start
			pio_sm_exec(audio_pio, sm[i], pio_encode_jmp(entry_point));
		}
	}
}

static void start_pio() noexcept
{
	uint mask = (1u << sm[0]) | (1u << sm[num_sm - 1]);
	pio_enable_sm_mask_in_sync(audio_pio, mask);
}

static void stop_pio() noexcept
{
	uint mask = (1u << sm[0]) | (1u << sm[num_sm - 1]);
	pio_set_sm_mask_enabled(audio_pio, mask, false);
}

static void RAM audio_isr() noexcept
{
	for (uint i = 0; i < dma_num_channels; i++)
	{
		if (dma_irqn_get_channel_status(dma_irqn, dma_channel[i]))
		{
			dma_irqn_acknowledge_channel(dma_irqn, dma_channel[i]);
			dma_channel_hw_addr(dma_channel[i])->al3_read_addr_trig = uint32(dma_buffer[i]);
			return;
		}
	}
}

static void init_dma() noexcept
{
	// we must raise dma irq priority because maybe the usb interrupt blocks for too long:
	// or maybe it is our own timer?
	constexpr uint PICO_HIGH_IRQ_PRIORITY = (PICO_DEFAULT_IRQ_PRIORITY + PICO_HIGHEST_IRQ_PRIORITY) / 2;

	for (uint i = 0; i < dma_num_channels; i++) dma_channel_cleanup(dma_channel[i]); // safety
	irq_set_priority(DMA_IRQ_0 + dma_irqn, PICO_HIGH_IRQ_PRIORITY);
	irq_add_shared_handler(DMA_IRQ_0 + dma_irqn, audio_isr, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
	irq_set_enabled(DMA_IRQ_0 + dma_irqn, true); // enable DMA interrupts on this core: this affects all DMA channels!
}

static void start_dma() noexcept
{
	for (uint i = 0; i < dma_num_channels; i++)
	{
		dma_irqn_acknowledge_channel(dma_irqn, dma_channel[i]);		  // discard any pending event (required?)
		dma_irqn_set_channel_enabled(dma_irqn, dma_channel[i], true); // enable interrupt source

		dma_channel_config config = dma_channel_get_default_config(dma_channel[i]);
		channel_config_set_dreq(&config, DREQ_PIO0_TX0 + sm[i]); // dreq = TX FIFO not full

		dma_channel_configure(
			dma_channel[i], &config,
			&audio_pio->txf[sm[i]], // write address
			dma_buffer[i],			// read address
			dma_buffer_size / 4,	// transfer count
			true);					// start now
	}
}

static void stop_dma() noexcept
{
	for (uint i = 0; i < dma_num_channels; i++) dma_channel_cleanup(dma_channel[i]);
}

static void update_timing() noexcept
{
	float sysclock = float(get_system_clock());

	if constexpr (audio_hw == I2S)
	{
		uint32 div = uint32(sysclock / (requested_sample_frequency * cc_per_sample_i2s) * 256 + 0.5f);
		pio_sm_set_clkdiv_int_frac8(audio_pio, sm[0], div / 256, uint8(div));
		hw_sample_frequency = sysclock / float(div) * 256 / cc_per_sample_i2s;
	}
	if constexpr (audio_hw == PWM)
	{
		// pwm sample frequency varies from 23674 Hz @ 100MHz to 68655 Hz @ 290MHz
		// no need to update clock divider as long as we always use 1.00.

		hw_sample_frequency = sysclock / cc_per_sample_pwm;
	}
	if constexpr (audio_hw == SIGMA_DELTA)
	{
		// the resulting clock divider is approx. in range 10 … 25.
		// TODO: if fractional part results in audible artifacts then without.

		uint32 div			= uint32(sysclock / (requested_sample_frequency * cc_per_sample_sid) * 256 + 0.5f);
		uint16 d			= uint16(div / 256);
		uint8  f			= uint8(div);
		hw_sample_frequency = sysclock / float(div) * 256 / cc_per_sample_sid;

		if constexpr (num_sm == 2)
		{
			// keep them synchronized:
			if (is_running) pio_set_sm_mask_enabled(audio_pio, (1 << sm[0]) | (1 << sm[1]), false);
			pio_sm_set_clkdiv_int_frac8(audio_pio, sm[0], d, f);
			pio_sm_set_clkdiv_int_frac8(audio_pio, sm[1], d, f);
			if (is_running) pio_enable_sm_mask_in_sync(audio_pio, (1 << sm[0]) | (1 << sm[1]));
		}
		else pio_sm_set_clkdiv_int_frac8(audio_pio, sm[0], d, f);
	}

	// calculate max_frames_ahead:
	limit(.001f, max_latency, 1.f);
	uint32 max_frames = uint32(hw_sample_frequency * max_latency);
	max_frames_ahead  = min(max_frames, dma_buffer_num_frames - 1);

	// calculate reload value for timer: (if used)
	timer_period_us = max_frames_ahead * 1000000u / 2u / uint32(hw_sample_frequency);

	debugstr("Audio: sample frequency = %u\n", uint32(hw_sample_frequency));
	debugstr("Audio: max_frames_ahead = %u\n", max_frames_ahead);
	debugstr("Audio: timer_period_us  = %u\n", timer_period_us);
}

inline uint32 pwm_sample(int32 sample) noexcept
{
	// data format:  0xDDDDLLHH
	// D = dither bits for lower 4 bits of sample
	// L = run length low for upper 8 bits of sample
	// H = run length high for upper 8 bits of sample

	if unlikely (sample != int16(sample)) return sample >= 0 ? 0xffff00ffu : 0x0000ff00u;

	sample		   = sample + 0x8000; // signed -> unsigned
	uint32 setbits = uint32(sample) >> 8;
	uint32 dither  = dither_table[uint8(sample) >> 4];

	return dither + setbits + ((255 - setbits) << 8);
}

inline int8 sid_sample(int32& current_sample, int32 sample) noexcept
{
	sample = sample >> 4; // 12 bit sample. TODO: measure actual hardware

	int delta = sample - current_sample;
	if (delta >= 0)
	{
		delta = min(127, delta);
		current_sample += delta;
		return int8(delta);
	}
	else
	{
		delta = min(127, -delta);
		current_sample -= delta;
		return int8(0x80 | delta);
	}
}

inline int16 i2s_sample(int32 sample) noexcept
{
	int16 result = int16(sample); // signed int16 sample
	return result == sample ? result : sample < 0 ? -0x8000 : 0x7fff;
}

static void dma_buffer_write(const AudioSample<hw_num_channels, int>* source, uint num_frames) noexcept
{
	// copy & convert source[] -> dma_buffer[]
	// handles wrap-around in dma_buffer[]

	constexpr uint dma_buffersize = dma_buffer_num_frames;
	constexpr uint dma_buffermask = dma_buffersize - 1;

	uint wi = dma_wi; // write index in dma buffer

	assert_lt(wi, dma_buffersize);
	assert_lt(num_frames, dma_buffersize);

	if (num_frames > dma_buffersize - wi)
	{
		dma_buffer_write(source, dma_buffersize - wi);
		source += dma_buffersize - wi;
		num_frames -= dma_buffersize - wi;
		wi = 0;
	}

	if constexpr (audio_hw == I2S)
	{
		// I2S uses interleaved l+r samples in dma_buffer[0]:
		int16* dest = reinterpret_cast<int16*>(&dma_buffer[0][wi]);
		for (uint i = 0; i < num_frames; i++)
		{
			*dest++ = i2s_sample(source->left());
			*dest++ = i2s_sample(source->right());
			source++;
		}
	}
	if constexpr (audio_hw == PWM && dma_num_channels == 1)
	{
		assert(wi >= 0 && wi + num_frames <= NELEM(dma_buffer[0]));

		uint32* dest = &dma_buffer[0][wi];
		for (uint i = 0; i < num_frames; i++)
		{
			*dest++ = pwm_sample(source->mono());
			source++;
		}
	}
	if constexpr (audio_hw == PWM && dma_num_channels == 2)
	{
		assert(wi >= 0 && wi + num_frames <= NELEM(dma_buffer[0]));

		uint32* l = &dma_buffer[0][wi];
		uint32* r = &dma_buffer[1][wi];
		for (uint i = 0; i < num_frames; i++)
		{
			*l++ = pwm_sample(source->left());
			*r++ = pwm_sample(source->right());
			source++;
		}
	}
	if constexpr (audio_hw == SIGMA_DELTA && dma_num_channels == 1)
	{
		int&  current_sample = sid_last_sample[0];
		int8* dest			 = reinterpret_cast<int8*>(dma_buffer[0]) + wi;
		for (uint i = 0; i < num_frames; i++)
		{
			*dest++ = sid_sample(current_sample, source->mono());
			source++;
		}
	}
	if constexpr (audio_hw == SIGMA_DELTA && dma_num_channels == 2)
	{
		// Each dma sends 32 bit words to it's pio which contain 4 samples.
		// The SM shifts the OSR left to get the sigma bit first => first sample is in high byte at high address.

		assert(num_frames % 4 == 0);
		int&  current_left_sample  = sid_last_sample[0];
		int&  current_right_sample = sid_last_sample[1];
		int8* l					   = reinterpret_cast<int8*>(dma_buffer[0]) + wi;
		int8* r					   = reinterpret_cast<int8*>(dma_buffer[1]) + wi;

		for (uint i = 0; i < num_frames / 4; i++)
		{
			l[3] = sid_sample(current_left_sample, source[0].left());
			l[2] = sid_sample(current_left_sample, source[1].left());
			l[1] = sid_sample(current_left_sample, source[2].left());
			l[0] = sid_sample(current_left_sample, source[3].left());
			r[3] = sid_sample(current_right_sample, source[0].right());
			r[2] = sid_sample(current_right_sample, source[1].right());
			r[1] = sid_sample(current_right_sample, source[2].right());
			r[0] = sid_sample(current_right_sample, source[3].right());
			l += 4;
			r += 4;
			source += 4;
		}
	}

	dma_wi = (wi + num_frames) & dma_buffermask;
}

static uint dma_frames_avail() noexcept
{
	constexpr uint dma_buffersize = dma_buffer_num_frames;
	constexpr uint dma_buffermask = dma_buffersize - 1;

	uint read_offset = dma_channel_hw_addr(dma_channel[0])->read_addr - uint32(dma_buffer[0]);
	uint ri			 = read_offset / dma_buffer_frame_size;
	uint wi			 = dma_wi;
	assert_le(ri, dma_buffer_num_frames);
	assert_le(wi, dma_buffer_num_frames);

	return (wi - ri) & dma_buffermask;
}

static int64 fill_buffer(alarm_id_t, void*) noexcept
{
	uint32 old_idle = LoadSensor::isr_start();

	if unlikely (check_timing)
	{
		check_timing = false;
		float old	 = hw_sample_frequency;
		update_timing();
		if (hw_sample_frequency != old)
			for (uint i = 0; i < num_sources; i++) { audio_sources[i]->setSampleRate(hw_sample_frequency); }
	}

	constexpr uint packetsize = 1 + 3 * (audio_hw == SIGMA_DELTA);
	constexpr uint packetmask = ~(packetsize - 1);

	uint frames_needed = (max_frames_ahead - dma_frames_avail()) & packetmask;

	assert_lt(int(frames_needed), int(dma_buffer_num_frames)); // may be negative!

	// since we run on a timer we don't want to use too much ram on the stack:
	constexpr uint							 ibu_size = 64;
	static AudioSample<hw_num_channels, int> input_buffer[ibu_size]; // 64 * 2 * 4 = 512 bytes
	AudioSample<hw_num_channels>			 ibu[ibu_size];

	while (int(frames_needed) > 0)
	{
		uint count = min(frames_needed, ibu_size);
		memset(input_buffer, 0, count * sizeof(*input_buffer));

		for (uint i = 0; i < num_sources; i++)
		{
			uint cnt = audio_sources[i]->getAudio(ibu, count);
			for (uint i = 0; i < cnt; i++) input_buffer[i] += ibu[i];
			if (cnt < count)
			{
				swap(audio_sources[i--], audio_sources[--num_sources]);
				audio_sources[num_sources] = nullptr;
			}
		}

		dma_buffer_write(input_buffer, count);
		frames_needed -= count;
	}

	LoadSensor::isr_end(old_idle);
	return -int32(timer_period_us);
}

int32 AudioController::fillBuffer(void*) noexcept
{
	if (is_running)
	{
		if (timer_id <= 0) // no timer used
		{
			fill_buffer(0, nullptr);
			// return slightly less than the time for 64 samples:
			// (64 = ibu_size in fill_buffer())
			// => fill_buffer() will normally only loop once
			//    if fillBuffer() / disp() is called frequently enough.
			return -63 * 1000000 / int32(hw_sample_frequency);
		}
		else return 0; // timer used => remove me
	}
	return 1000000 / 25; // 1/25 sec reaction time
}

void AudioController::startAudio(bool with_timer) noexcept
{
	if (is_running)
	{
		if (with_timer == (timer_id > 0)) return;
		else stopAudio();
	}

	check_timing								   = true;
	dma_wi										   = 0;
	dma_channel_hw_addr(dma_channel[0])->read_addr = uint32(dma_buffer[0]); // dma_ri = 0
	fill_buffer(0, nullptr);
	start_dma();
	start_pio();

	is_running = true;

	if (with_timer)
	{
		assert(timer_period_us != 0);
		timer_id = add_alarm_in_us(timer_period_us, fill_buffer, nullptr, false);
		if (timer_id <= 0) panic("Audio: no timer available!");
	}
}

void AudioController::stopAudio(bool remove_audio_sources) noexcept
{
	if (!is_running) return;
	is_running = false;

	if (timer_id > 0) cancel_alarm(timer_id);
	timer_id = 0;

	stop_dma();
	stop_pio();

	if (remove_audio_sources) removeAllAudioSources();
}

bool AudioController::isRunning() noexcept
{
	return is_running; //
}

AudioController::AudioController() noexcept
{
	spinlock = spin_lock_instance(next_striped_spin_lock_num());

	if constexpr (audio_hw == I2S)
	{
		gpio_set_function(audio_i2s_data_pin, GPIO_FUNC_PIO0);
		gpio_set_function(audio_i2s_clock_pin_base, GPIO_FUNC_PIO0);
		gpio_set_function(audio_i2s_clock_pin_base + 1, GPIO_FUNC_PIO0);
	}

	if constexpr (audio_hw == PWM || audio_hw == SIGMA_DELTA)
	{
		gpio_set_function(audio_left_pin, GPIO_FUNC_PIO0);
		if constexpr (hw_num_channels == 2) gpio_set_function(audio_right_pin, GPIO_FUNC_PIO0);
	}

	for (uint i = 0; i < num_sm; i++)
	{
		dma_channel[i] = uint8(dma_claim_unused_channel(true));
		sm[i]		   = uint8(pio_claim_unused_sm(audio_pio, true));
	}

	init_pio();
	init_dma();
}

void AudioController::setMaxLatency(uint32 ms) noexcept
{
	max_latency	 = float(ms) * .001f;
	check_timing = true;
}

void AudioController::setSampleFrequency(float f) noexcept
{
	// sample frequency will be updated next time fill_buffer() is called

	requested_sample_frequency = f;
	check_timing			   = true;
}

float AudioController::getSampleFrequency() noexcept
{
	if (check_timing && is_running)
	{
		if (timer_id > 0)
			while (check_timing) wfe();
		else fill_buffer(0, nullptr);
	}
	return hw_sample_frequency;
}

HwAudioSource* AudioController::addAudioSource(RCPtr<AudioSource<hw_num_channels>> ac) noexcept
{
	Locker _;
	if (ac == nullptr || num_sources >= max_sources) return nullptr;
	else return audio_sources[num_sources++] = std::move(ac);
}

HwAudioSource* AudioController::addAudioSource(RCPtr<AudioSource<3 - hw_num_channels>> ac) noexcept
{
	return addAudioSource(new (std::nothrow) NumChannelsAdapter<3 - hw_num_channels, hw_num_channels>(std::move(ac)));
}

void AudioController::removeAudioSource(RCPtr<HwAudioSource> ac) noexcept
{
	{
		Locker _;
		for (uint i = 0; i < num_sources; i++)
		{
			if (audio_sources[i] == ac)
			{
				swap(audio_sources[i], audio_sources[--num_sources]);
				audio_sources[num_sources] = nullptr; // not delete here: channel still held by ac
				break;
			}
		}
	}
	// note: channel ac deleted here (probably) after lock released
}

void AudioController::removeAllAudioSources() noexcept
{
	RCPtr<HwAudioSource> as = nullptr;
	while (num_sources)
	{
		{
			Locker _;
			swap(audio_sources[--num_sources], as);
		}
		as = nullptr;
	}
}

void sysclockChanged(uint32 /*new_clock*/) noexcept
{
	// called from set_system_clock():
	// sample frequency will be changed next time fill_buffer() is called

	check_timing = true;
}

void beep(float frequency, float volume, uint32 duration_ms)
{
	// create a channel which produces a square wave of the given frequency, volume and duration:
	// frequency = Hz
	// volume = -1.0 .. +1.0
	// duration = msec

	class BeepingAudioSource : public HwAudioSource
	{
	public:
		uint   num_phases_remaining;
		float  samples_per_phase;
		float  position_in_phase = 0;
		Sample sample, _padding;
		BeepingAudioSource(float frequency, float volume, uint32 duration_ms) :
			num_phases_remaining(uint(float(duration_ms) / 1000 * frequency * 2)),
			samples_per_phase(hw_sample_frequency / frequency * 0.5f),
			sample(Sample(minmax(-0x7fff, int(volume * 0x8000), 0x7fff)))
		{}
		virtual uint getAudio(HwAudioSample* z, uint zcnt) noexcept override
		{
			for (uint i = 0; i < zcnt; i++)
			{
				if unlikely (position_in_phase >= samples_per_phase)
				{
					if unlikely (num_phases_remaining == 0) return zcnt - i; // remove channel

					num_phases_remaining--;
					position_in_phase -= samples_per_phase;
					sample = -sample;
				}

				position_in_phase++;
				*z++ = sample;
			}
			return zcnt;
		}
	};

	AudioController& ac = AudioController::getRef();
	if (!is_running) ac.startAudio(true);
	ac.addAudioSource(new BeepingAudioSource(frequency, volume, duration_ms));
}

#endif // audio_hw_num_channels != 0


AudioController& AudioController::getRef() noexcept
{
	// may panic on first call if HW can't be claimed

	static AudioController audiocontroller;
	return audiocontroller;
}


#if audio_hw_num_channels == 0

static bool is_running			= false;
float		hw_sample_frequency = AUDIO_DEFAULT_SAMPLE_FREQUENCY;

AudioController::AudioController() noexcept {}
void		   AudioController::setSampleFrequency(float) noexcept {}
void		   AudioController::setMaxLatency(uint32) noexcept {}
void		   AudioController::startAudio(bool) noexcept { is_running = 1; }
void		   AudioController::stopAudio() noexcept { is_running = 0; }
HwAudioSource* AudioController::addAudioSource(RCPtr<AudioSource<1>>) noexcept { return nullptr; }
HwAudioSource* AudioController::addAudioSource(RCPtr<AudioSource<2>>) noexcept { return nullptr; }
void		   AudioController::removeAudioSource(RCPtr<HwAudioSource>) noexcept {}
bool		   AudioController::isRunning() noexcept { return is_running; }
void		   AudioController::fillBuffer() noexcept {}

void beep(float, float, uint32 duration_ms)
{
	if constexpr (audio_hw == AudioHardware::NONE) return;

	// audio_hw = AudioHardware::BUZZER

	static bool		  initialized = false;
	static alarm_id_t id		  = 0;

	auto switch_off = [](alarm_id_t, void*) {
		gpio_put(audio_buzzer_pin, 0);
		id = 0;
		return int64(0); // do not reshedule
	};

	if unlikely (!initialized)
	{
		gpio_init(audio_buzzer_pin);
		gpio_set_dir(audio_buzzer_pin, GPIO_OUT);
		initialized = true;
	}

	gpio_put(audio_buzzer_pin, 1);

	if (id) cancel_alarm(id);
	id = add_alarm_in_ms(duration_ms, switch_off, nullptr, false);
	assert(id >= 0);
	return;
}

#endif

} // namespace kio::Audio


/*









































































*/
