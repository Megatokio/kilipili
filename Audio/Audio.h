// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSample.h"
#include "AudioSource.h"
#include "audio_options.h"

/* _______________________________________________________________________________________
   namespace Audio
   handles audio output acc. to settings in the boards header file. (e.g. see `boards/` folder)
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
		change the sample frequency used by the hardware.
		the AudioController will try it's best, but especially PWM has very strict requirements.
		the sample frequency is not changed directly but only on the next call to `fillAudioBuffer()`.
		the default sample frequency can be set with AUDIO_DEFAULT_SAMPLE_FREQUENCY.
	*/
extern void setSampleFrequency(float f) noexcept;

/* _______________________________________________________________________________________
		get the actually used sample frequency.
		if the sample frequency was recently changed and `startAudio()` was called with_timer = true
		then this function waits until hw_sample_frequency is updated in the next timer call.
	*/
extern float getSampleFrequency() noexcept;

/* _______________________________________________________________________________________
		set the maximum latency for audio output.
		this applies to the AudioController only, if you have buffered AudioSources they add to this.
		normally the latency is limited by the size of the dma buffer and the hw_sample_frequency.
		the default values are 10 ms for the max. latency, but the dma buffer reduces that to approx. 5ms
		for the default AUDIO_DMA_BUFFER_SIZE and AUDIO_DEFAULT_SAMPLE_FREQUENCY of 44100 Hz.
	*/
extern void setMaxLatency(uint32 msec) noexcept;

/* _______________________________________________________________________________________
	start the audio output.
	note: function `beep()` silently starts the audio output with_time = true if not yet running.
	with_timer = false:
		the AudioController starts no timer for automatic audio update.
		instead the application must call `fillAudioBuffer()` fast enough, probably in it's event loop.
		advantage: fillAudioBuffer() and indirectly all refill functions of your AudioSources are called
			synchronously with your program, not on an interrupt.
	with_timer = true:
		the AudioController starts a timer for automatic audio update.
		advantage: your application doesn't need to call `fillAudioBuffer()` for refilling the AudioSources.
		disadvantage: your refill functions are called on a timer interrupt and you must properly
			synchronize and block everything.
*/
extern void startAudio(bool with_timer) noexcept; // may panic

/* _______________________________________________________________________________________
	stop the audio output.
	stops the timer, if used.
	stops the dma.
	hardware is not unclaimed.
	remove_audio_sources = true => release all AudioSources 
*/
extern void stopAudio(bool remove_audio_sources = false) noexcept;

/* _______________________________________________________________________________________
	query whether the AudioController is running.
*/
extern bool isAudioRunning() noexcept;

/* _______________________________________________________________________________________
	callback for use in your event loop.
	fillAudioBuffer() can be added as a handler to the Dispatcher.
	fillAudioBuffer() calls the refill functions of all added AudioSources.
	if you startAudio() with_timer = true then calling fillAudioBuffer() is not needed 
	and fillAudioBuffer() will return 0 to remove itself from the dispatcher.
	fillAudioBuffer() can be called and does nothing while the AudioController is stopped.
	Note: prefer startAudio() with_timer = true for reliable refilling of the audio buffer.		
*/
extern int32 fillAudioBuffer(void* = nullptr) noexcept;

/* _______________________________________________________________________________________
	add an AudioSource to the controller.
	up to 8 sources can be stored.
	convenience overload: 
	the AudioSource with the wrong number of channels will be wrapped with a NumChannelsAdapter.
	return HwAudioSource which can be used for later removal of the AudioSource.
	returns nullptr if the AudioSource could not be added.
*/
extern HwAudioSource* addAudioSource(RCPtr<AudioSource<1>>) noexcept;
extern HwAudioSource* addAudioSource(RCPtr<AudioSource<2>>) noexcept;
inline HwAudioSource* addAudioSource(RCPtr<AudioSource<0>>) noexcept { return nullptr; }

/* _______________________________________________________________________________________
	remove an AudioSource from the controller.
	seeks and removes the source.
	silently does nothing if it cannot be found.
	a source can also remove itself from the AudioController by returning less samples than requested
	in a call to `virtual bool AudioSource::addAudio()`.
*/
extern void removeAudioSource(RCPtr<HwAudioSource>) noexcept;

extern void removeAllAudioSources() noexcept;

} // namespace kio::Audio

/*

















*/
