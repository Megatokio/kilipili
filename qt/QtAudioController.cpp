// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "QtAudioController.h"
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <thread>


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
static volatile bool		timer_running = false;

void  setAudioSampleFrequency(float f) noexcept {}
void  setAudioMaxLatency(uint32 msec) noexcept {}
float getAudioSampleFrequency() noexcept { return hw_sample_frequency; }


inline Sample		clamped(int q) noexcept { return q == Sample(q) ? Sample(q) : q < 0 ? -0x7fff : +0x7fff; }
inline MonoSample	clamped(const AudioSample<1, int>& q) { return clamped(q.m); }
inline StereoSample clamped(const AudioSample<2, int>& q) { return StereoSample(clamped(q.l), clamped(q.r)); }

int32 fillAudioBuffer(void*) noexcept
{
	assert(get_core_num() == 0);
	if (!audio_is_running) return 0;
	assert(audio_device != nullptr);
	assert(audio_sink != nullptr);

	static constexpr uint			  ibu_size = 64;
	AudioSample<hw_num_channels, int> input_buffer[ibu_size];
	AudioSample<hw_num_channels>	  ibu[ibu_size];

	uint frames_needed = uint(audio_sink->bytesFree()) / sizeof(HwAudioSample);

	auto sr = audio_spinlock.lock();
	while (int(frames_needed) > 0)
	{
		memset(input_buffer, 0, sizeof(input_buffer));

		uint count = min(frames_needed, ibu_size);
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

		for (uint i = 0; i < count; i++) { ibu[i] = clamped(input_buffer[i]); }
		ssize_t cnt = audio_device->write(ptr(ibu), count * sizeof(HwAudioSample));
		assert(cnt == count);
		frames_needed -= count;
	}
	audio_spinlock.unlock(sr);

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
		uint buffer_size = uint(sizeof(HwAudioSample) * hw_sample_frequency * 0.1f); // 0.1 sec
		audio_sink->setBufferSize(buffer_size);
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
