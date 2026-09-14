// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Audio/Audio.h"
#include "Audio/AudioSource.h"
#include "Devices/File.h"
#include "MadFrame.h"
#include "MadStream.h"
#include "MadSynth.h"
#include "common/Queue.h"
#include "common/standard_types.h"
#include "common/trace.h"

namespace kilipili::Audio
{

template<uint nc, uint size>
class PipedAdapter : public AudioSource<nc>
{
public:
	Queue<AudioSample<nc>, size> queue;
	bool						 eof {false};

	uint getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept override;
	//void setSampleRate(float /*new_sample_frequency*/) noexcept override {}
};

template<uint nc, uint size>
uint PipedAdapter<nc, size>::getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept
{
	// provide audio data in AudioController callback:

	trace(__func__);

	uint n = queue.read(buffer, num_frames);
	if (n == num_frames) return n;
	if (eof) return n; // -> this will remove us from the AudioController
	AudioSample<nc> s = n ? buffer[n - 1] : AudioSample<nc>();
	while (n < num_frames) buffer[n++] = s;
	return n;
}


class Mp3Player
{
	Id("Mp3Player");
	using FilePtr	   = Devices::FilePtr;
	using DirectoryPtr = Devices::DirectoryPtr;

	static int callback(void* self) noexcept;

public:
	Mp3Player();
	~Mp3Player() noexcept;
	Mp3Player(const Mp3Player&) = delete;

	// run state machine to play the file:
	int run() noexcept;

	void  play(cstr fpath);
	void  playDirectory(cstr dpath);
	void  play(cstr fpath, bool loop);
	void  playDirectory(cstr dpath, bool loop);
	void  stop();
	void  stopAfterSong();
	void  pause(bool onoff = true);
	void  skip(); // resume next song if playing from dir
	void  setFpos(float seconds);
	float getSize();
	float getFpos();
	void  setVolume(float);
	cstr  getFilename();

	virtual void header(const MadHeader*) {}			// optional
	virtual void filter(const MadStream*, MadFrame*) {} // optional

	struct Buffer
	{
		static constexpr uint size = 2000;

		void   reset() noexcept { count = 0; }
		uchar  data[size]; // input stream buffer
		uint16 count = 0;
		int16  rc	 = 0;
	};
	RCPtr<Buffer>	 buffer; // stream buffer for mp3 decoder
	RCPtr<MadSynth>	 synth;	 // size > 13 kB
	RCPtr<MadFrame>	 frame;	 // size > 9 kB
	RCPtr<MadStream> stream; // size ~ 100 byte

	RCPtr<Devices::File> input_file;
	using AudioAdapter = PipedAdapter<hw_num_channels, 4 kB>;
	RCPtr<AudioAdapter> output_adapter;

	// while stopped the current file & dir can be remembered:
	cstr next_file = nullptr;
	cstr next_dir  = nullptr;

	// the current directory (if any) while playing:
	DirectoryPtr mp3_dir;

	uint   sample_rate {0};
	uchar  options {0};
	uchar  rc {0}; // RCPtr<>
	uchar  state {0};
	ushort nsamples_done;

	bool paused		 = false;
	bool repeat_file = false;
	bool repeat_dir	 = false;
};


} // namespace kilipili::Audio
