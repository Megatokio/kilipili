// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Devices/devices_types.h"
#include "common/RCPtr.h"


namespace kilipili::Audio
{

// Interface:

class AudioPlayer
{
public:
	AudioPlayer(cstr fn_pattern) noexcept; // e.g. "*.mp3"
	/*virtual*/ ~AudioPlayer() noexcept;

	void play(cstr fpath);
	void playDirectory(cstr dpath);
	void play(cstr fpath, bool loop);
	void playDirectory(cstr dpath, bool loop);

	void stop() noexcept;
	void stopAfterSong() noexcept;
	void pause(bool p = true) noexcept { paused = p; }
	void resume() noexcept { paused = false; } // after pause or stop
	void skip() noexcept;					   // resume next song if playing from dir

	//virtual void setVolume(float) noexcept = 0;

	Devices::File* nextInputFile();

	const char* fn_pattern; // e.g. "*.mp3"

	// while stopped the current file & dir can be remembered:
	cstr next_file = nullptr;
	cstr next_dir  = nullptr;

	// the current directory (if any) while playing:
	RCPtr<Devices::Directory> current_dir;
	RCPtr<Devices::File>	  input_file;

	bool paused		 = false;
	bool repeat_file = false;
	bool repeat_dir	 = false;
};

} // namespace kilipili::Audio
