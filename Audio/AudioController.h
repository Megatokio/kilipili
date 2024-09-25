// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSource.h"
#include "audio_options.h"

/* _______________________________________________________________________________________
   class AudioController
   provides the audio output.
*/


namespace kio::Audio
{

// enumeration of supported types of audio hardware.
// these are to be compared with `audio_hw` which is set depending on settings in your boards header file.
enum AudioHardware : uint8 { NONE, BUZZER, I2S, PWM, SIGMA_DELTA };

// actual HW sample frequency, set by AudioController
extern float hw_sample_frequency;

// actual HW number of channels, set from information from your boards header file
constexpr uint hw_num_channels = audio_hw_num_channels;

// AudioSample with the same number of channels as the hardware
using HwAudioSample = AudioSample<hw_num_channels>;

// AudioSource with the same number of channels as the hardware
using HwAudioSource = AudioSource<hw_num_channels>;


/* _______________________________________________________________________________________
   create a short mono `beep` of given frequency, volume and duration.
   function `beep()` is implemented even if no hardware is present or only a simple buzzer is attached.
*/
extern void beep(float frequency_hz = 880, float volume = .5f, uint32 duration_ms = 600);


/* _______________________________________________________________________________________
   class AudioController
   this is a singleton which manages all audio output.
   it handles audio output acc. to settings in your boards header file. (e.g. see `boards/` folder)
*/
class AudioController
{
public:
	/* _______________________________________________________________________________________
		get reference to singleton.
		on first call the required hardware is claimed.
		panics if claiming the required hardware fails.
	*/
	static AudioController& getRef() noexcept;

	/* _______________________________________________________________________________________
		change the sample frequency used by the hardware.
		the AudioController will try it's best, but especially PWM has very strict requirements.
		the sample frequency is not changed directly but only on the next call to `fillBuffer()`.
		the default sample frequency can be set with AUDIO_DEFAULT_SAMPLE_FREQUENCY.
	*/
	void setSampleFrequency(float f) noexcept;

	/* _______________________________________________________________________________________
		get the actually used sample frequency.
		if the sample frequency was recently changed and `startAudio()` was called with_timer = true
		then this function waits until hw_sample_frequency is updated in the next timer call.
	*/
	float getSampleFrequency() noexcept;

	/* _______________________________________________________________________________________
		set the maximum latency for audio output.
		this applies to the AudioController only, if you have buffered AudioSources they add to this.
		normally the latency is limited by the size of the dma buffer and the hw_sample_frequency.
		the default values are 10 ms for the max. latency, but the dma buffer reduces that to approx. 5ms
		for the default AUDIO_DMA_BUFFER_NUM_FRAMES and AUDIO_DEFAULT_SAMPLE_FREQUENCY of 44100 Hz.
	*/
	void setMaxLatency(uint32 msec) noexcept;

	/* _______________________________________________________________________________________
		start the audio output.
		note: function `beep()` silently starts the audio output with_time = true if not yet running.
		with_timer = false:
			the AudioController starts no timer for automatic audio update.
			instead the application must call `fillBuffer()` fast enough, probably in it's event loop.
			advantage: fillBuffer() and indirectly all refill functions of your AudioSources are called
				synchronously with your program, not on an interrupt.
		with_timer = true:
			the AudioController starts a timer for automatic audio update.
			advantage: your application doesn't need to call `fillBuffer()` for refilling the AudioSources.
			disadvantage: your refill functions are called on a timer interrupt and you must properly
				synchronize and block everything.
	*/
	void startAudio(bool with_timer) noexcept; // may panic

	/* _______________________________________________________________________________________
		stop the audio output.
		stops the timer, if used.
		stops the dma.
		the AudioSources are not deleted!
		hardware is not unclaimed.
	*/
	void stopAudio() noexcept;

	/* _______________________________________________________________________________________
		query whether the AudioController is running.
	*/
	bool isRunning() noexcept;

	/* _______________________________________________________________________________________
		callback to be called from your event loop.
		fillBuffer() calls in return the refill functions of your AudioSources.
		if you startAudio() with_timer = true then calling fillBuffer() is not needed.
		silently does nothing if the controller is not running or was started with_timer = true.
	*/
	void fillBuffer() noexcept; // for call by state machine

	/* _______________________________________________________________________________________
		add an AudioSource to the controller.
		up to 8 sources can be stored.
		convenience overload: 
		the AudioSource with the wrong number of channels will be wrapped with a NumChannelsAdapter.
		return HwAudioSource which can be used for later removal of the AudioSource.
		returns nullptr if the AudioSource could not be added.
	*/
	HwAudioSource* addChannel(RCPtr<AudioSource<1>>) noexcept;
	HwAudioSource* addChannel(RCPtr<AudioSource<2>>) noexcept;

	/* _______________________________________________________________________________________
		remove an AudioSource from the controller.
		seeks and removes the source.
		silently does nothing if it cannot be found.
		a source can also remove itself from the AudioController by returning less samples than requested
		in a call to `virtual bool AudioSource::addAudio()`.
	*/
	void removeChannel(RCPtr<HwAudioSource>) noexcept;

private:
	AudioController() noexcept;
};

} // namespace kio::Audio

/*

















*/
