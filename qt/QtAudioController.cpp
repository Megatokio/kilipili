// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "QtAudioController.h"
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <thread>

#if defined AUDIO_INPUT_BUFFER_SIZE && AUDIO_INPUT_BUFFER_SIZE != 0
static constexpr uint ibu_size = AUDIO_INPUT_BUFFER_SIZE;
#else
static constexpr uint ibu_size = 64;
#endif


#if audio_hw_num_channels > 0

// i hope the QAudioSink can be created on core0
// i hope the QIODevice is a QBuffer or similarly reentrant
static QAudioSink* audio_sink	= nullptr;
static QIODevice*  audio_device = nullptr;


namespace kilipili
{
extern bool					audio_is_running;
extern float				hw_sample_frequency;
extern uint					num_sources;
extern RCPtr<HwAudioSource> audio_sources[];
extern SpinLock				audio_spinlock;
static volatile bool		timer_running			   = false;
static int32				requested_sample_frequency = 0;

static RCPtr<AudioSource<hw_num_channels>> combined_audio = nullptr;


float getAudioSampleFrequency() noexcept { return hw_sample_frequency; }
void  setAudioMaxLatency(uint32 msec) noexcept {}
void  setAudioSampleFrequency(float f) noexcept { requested_sample_frequency = int32(f + 0.5f); }


inline Sample		clamped(int q) noexcept { return q == Sample(q) ? Sample(q) : q < 0 ? -0x7fff : +0x7fff; }
inline MonoSample	clamped(const AudioSample<1, int>& q) { return clamped(q.m); }
inline StereoSample clamped(const AudioSample<2, int>& q) { return StereoSample(clamped(q.l), clamped(q.r)); }

class CombinedAudioSources : public AudioSource<hw_num_channels>
{
public:
	virtual void setSampleRate(float new_sample_frequency) noexcept override {}

	virtual uint getAudio(HwAudioSample* output_buffer, uint count) noexcept override
	{
		assert(count <= ibu_size);
		auto sr = audio_spinlock.lock();

		if (num_sources == 0)
		{
			memset(output_buffer, 0, count * sizeof(*output_buffer)); //
		}
		else if (num_sources == 1)
		{
			uint cnt = audio_sources[0]->getAudio(output_buffer, count);
			if (cnt < count) { audio_sources[--num_sources] = nullptr; }
		}
		else
		{
			AudioSample<hw_num_channels, int> input_buffer[ibu_size];
			memset(input_buffer, 0, sizeof(input_buffer));

			for (uint i = 0; i < num_sources; i++)
			{
				uint cnt = audio_sources[i]->getAudio(output_buffer, count);
				for (uint i = 0; i < cnt; i++) input_buffer[i] += output_buffer[i];
				if (cnt < count)
				{
					swap(audio_sources[i--], audio_sources[--num_sources]);
					audio_sources[num_sources] = nullptr;
				}
			}

			for (uint i = 0; i < count; i++) { *output_buffer++ = clamped(input_buffer[i]); }
		}

		audio_spinlock.unlock(sr);
		return count;
	}
};

int32 fillAudioBuffer(void*) noexcept
{
	assert(get_core_num() == 0);
	if (!audio_is_running) return 0;
	assert(audio_device != nullptr);
	assert(audio_sink != nullptr);

	if (requested_sample_frequency)
	{
		if (requested_sample_frequency != int(hw_sample_frequency))
		{
			combined_audio = new CombinedAudioSources();
			if (requested_sample_frequency != audio_sink->format().sampleRate())
				combined_audio = new SampleRateAdapter(
					combined_audio, requested_sample_frequency, audio_sink->format().sampleRate());
			hw_sample_frequency = requested_sample_frequency;
		}
		requested_sample_frequency = 0;
	}

	HwAudioSample ibu[ibu_size];

	uint frames_needed = uint(audio_sink->bytesFree()) / sizeof(HwAudioSample);

	while (int(frames_needed) > 0)
	{
		uint count = min(frames_needed, ibu_size);
		uint sz	   = combined_audio->getAudio(ibu, count);
		assert(sz == count);
		ssize_t cnt = audio_device->write(ptr(ibu), count * sizeof(HwAudioSample));
		assert(cnt == count * sizeof(HwAudioSample));
		frames_needed -= count;
	}

	return 5000; // usec
}

static void start_timer()
{
	timer_running = true;
	std::thread t([=]() {
		for (int32 delay = 5000; delay; delay = fillAudioBuffer())
		{
			::sleep_us(uint(delay)); //
		}
		timer_running = false;
	});
	t.detach();
}

void startAudio(bool with_timer) noexcept
{
	assert(get_core_num() == 0);

	if (audio_sink == nullptr)
	{
		QAudioFormat format;
		format.setSampleRate(int(hw_sample_frequency));
		format.setChannelCount(hw_num_channels);
		static_assert(sizeof(Sample) == 2);
		format.setSampleFormat(QAudioFormat::Int16);
		QAudioDevice info(QMediaDevices::defaultAudioOutput());
		assert(info.isFormatSupported(format));

		audio_sink = new QAudioSink(format);
		assert(audio_sink->error() == QAudio::NoError);
		size_t buffer_size = sizeof(HwAudioSample) * uint(hw_sample_frequency * 0.1f); // 0.1 sec
		audio_sink->setBufferSize(ssize_t(buffer_size));
		combined_audio = new CombinedAudioSources();
	}
	else if (audio_is_running)
	{
		if (with_timer == timer_running) return;
		else stopAudio();
	}

	audio_device	 = audio_sink->start();
	audio_is_running = true;
	if (with_timer) start_timer();
}

void stopAudio(bool remove_audio_sources) noexcept
{
	if (get_core_num() == 2) // Gui thread: shut down & exit
	{
		// this isn't completely water tight:
		// core0 could startAudio() just right now
		if (audio_is_running)
		{
			audio_is_running = false; // -> fillAudioBuffer() will immediately return
			sleep_us(500);			  // if fillAudioBuffer() is currently executed
		}
		return;
	}

	assert(get_core_num() == 0);

	audio_is_running = false;
	while (timer_running) { sleep_us(1000); }
	audio_device = nullptr;
	if (audio_sink) audio_sink->stop();
	if (remove_audio_sources) removeAllAudioSources();
}


} // namespace kilipili

#endif // num_channels > 0


/*











































*/
