// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSource.h"
#include "SP0256.h"
#include "common/Queue.h"


namespace kio::Audio
{

/*	_______________________________________________________________________________________
	template class SP0256_AudioSource implements a SP0256 speech synthesizer chip.
	The class queues the write commands so that writing and audio output can be done asynchronously.
	The AudioSource should be added to the AudioController.

	num_channels: 1=mono, 2=stereo. should match audio_hw_num_channels.
	queue_size:
		= 0:  close coupling to async. audio output
		= 4:  little delay 
		= 64: push whole sentences in one go

	the specialization with queue_size = 0 provides close coupling to async. audio output:
	  - you can overwrite the waiting command in the input register.

	the version with buffer has some subtle differences to using class SP0256 directly:
	  - acceptsNextCommand() is false until reset() completes.
	  - you can not overwrite the waiting command in the input register.
*/
template<uint num_channels, uint queue_size = 4>
class SP0256_AudioSource : public AudioSource<num_channels>
{
	static_assert(queue_size >= 4 && queue_size <= 256);

public:
	SP0256_AudioSource(float sp_frequency = 3.12e6f, float volume = 0.5f) noexcept;

	void  setVolume(float volume) noexcept { sp0256.setVolume(volume); }
	float getClock() const noexcept { return sp0256.frequency; }
	void  reset() noexcept;
	void  finish() noexcept { do_finish = true; } // self-destruct after the last tune played
	bool  acceptsNextCommand() const noexcept;
	bool  isSpeaking() const noexcept;
	void  writeCommand(uint sp0256_cmd) noexcept;

	// callback for the audio backend:
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;

private:
	void setClock(float f) noexcept { sp0256.setClock(f); }					   // must be called synchronized
	void setSampleRate(float f) noexcept override { sp0256.setSampleRate(f); } // AudioController can call base class

	SP0256<num_channels>			 sp0256;
	Queue<uint8, queue_size, uint16> queue;

	bool do_reset  = false;
	bool do_finish = false;
};


/*	_______________________________________________________________________________________
	specialization with queue_size = 0 for close coupling to async. audio output
*/
template<uint num_channels>
class SP0256_AudioSource<num_channels, 0> : public AudioSource<num_channels>
{
public:
	SP0256_AudioSource(float sp_frequency = 3.12e6f, float volume = 0.5f) noexcept;

	void  setVolume(float volume) noexcept { sp0256.setVolume(volume); }
	float getClock() const noexcept { return sp0256.frequency; }
	void  reset() noexcept;
	void  finish() noexcept { do_finish = true; }
	bool  acceptsNextCommand() const noexcept;
	bool  isSpeaking() const noexcept;
	void  writeCommand(uint sp0256_cmd) noexcept;

	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;

private:
	void setClock(float f) noexcept { sp0256.setClock(f); }
	void setSampleRate(float f) noexcept override { sp0256.setSampleRate(f); }

	SP0256<num_channels> sp0256;

	// shadow command register:
	// command_valid if next != read
	uint16 next_cmd = 0; // lo = cmd, hi = serial no
	uint16 read_cmd = 0; // ""

	bool do_reset  = false;
	bool do_finish = false;
};


// ===================================================================================
// implementations queue_size = 0:


template<uint nc>
SP0256_AudioSource<nc, 0>::SP0256_AudioSource(float frequency, float volume) noexcept : sp0256(frequency, volume)
{}

template<uint nc>
void SP0256_AudioSource<nc, 0>::reset() noexcept
{
	read_cmd = next_cmd; // clear shadow command register
	do_reset = true;	 // async reset chip
}

template<uint nc>
bool SP0256_AudioSource<nc, 0>::isSpeaking() const noexcept
{
	if (next_cmd != read_cmd) return true; // shadow command valid
	if unlikely (do_reset) return false;   // becomes false after reset
	return sp0256.isSpeaking();			   // else actual chip state
}

template<uint nc>
bool SP0256_AudioSource<nc, 0>::acceptsNextCommand() const noexcept
{
	if (next_cmd != read_cmd) return false; // shadow command valid
	if unlikely (do_reset) return true;		// becomes true after reset
	return sp0256.acceptsNextCommand();		// else actual chip state
}

template<uint nc>
void SP0256_AudioSource<nc, 0>::writeCommand(uint cmd) noexcept
{
	// may overwrite current command
	// may be written while do_reset pending!
	next_cmd = (next_cmd & 0xff00) + 0x100 + cmd;
}

template<uint nc>
uint SP0256_AudioSource<nc, 0>::getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept
{
	// audio callback

	if unlikely (do_reset)
	{
		sp0256.reset();	  // doit
		do_reset = false; // done
	}

	if (next_cmd != read_cmd)
	{
		sp0256.writeCommand(uint8(read_cmd)); // doit
		read_cmd = next_cmd;				  // done
	}

	if unlikely (do_finish)
		if (!sp0256.isSpeaking()) return 0;

	sp0256.audioBufferStart(buffer, num_frames);
	sp0256.audioBufferEnd();
	return num_frames;
}


// ===================================================================================
// implementations queue_size > 0:


template<uint nc, uint qs>
SP0256_AudioSource<nc, qs>::SP0256_AudioSource(float frequency, float volume) noexcept : sp0256(frequency, volume)
{}

template<uint nc, uint qs>
void SP0256_AudioSource<nc, qs>::reset() noexcept
{
	do_reset = true;
}

template<uint nc, uint qs>
bool SP0256_AudioSource<nc, qs>::isSpeaking() const noexcept
{
	return queue.avail() || sp0256.isSpeaking() || do_reset;
}

template<uint nc, uint qs>
bool SP0256_AudioSource<nc, qs>::acceptsNextCommand() const noexcept
{
	return queue.free() && !do_reset;
}

template<uint nc, uint qs>
void SP0256_AudioSource<nc, qs>::writeCommand(uint cmd) noexcept
{
	while (queue.free() == 0) { __dmb(); }
	queue.put(cmd);
}

template<uint nc, uint qs>
uint SP0256_AudioSource<nc, qs>::getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept
{
	if unlikely (do_reset)
	{
		queue.flush(); // only reader can flush!
		sp0256.reset();
		do_reset = false;
	}

	if (queue.avail() && sp0256.acceptsNextCommand()) // try only to poll one command
		sp0256.writeCommand(queue.get());			  // because speaking takes longer than playing the buffer

	if unlikely (do_finish)
		if (!sp0256.isSpeaking()) return 0; // remove this AudioSource from AudioController

	sp0256.audioBufferStart(buffer, num_frames);
	sp0256.audioBufferEnd();
	return num_frames;
}

} // namespace kio::Audio

/*





























*/
