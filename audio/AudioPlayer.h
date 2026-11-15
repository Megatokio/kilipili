// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "devices/devices_types.h"


namespace kilipili
{

// Interface:

class AudioPlayer
{
public:
	AudioPlayer(cstr fn_pattern) noexcept; // e.g. "*.mp3"
	/*virtual*/ ~AudioPlayer() noexcept;

	void playFile(cstr fpath);
	void playDirectory(cstr dpath);

	void stop() noexcept;
	void stopAfterSong() noexcept;
	void pause(bool p = true) noexcept { paused = p; }
	void resume() noexcept { paused = false; } // after pause or stop
	void skip() noexcept;					   // resume next song if playing from dir

	//virtual void setVolume(float) noexcept = 0;

	File* nextInputFile();

	const cstr dflt_pattern; // e.g. "*.mp3"

	cstr next_file = nullptr; // playFile()
	cstr next_dir  = nullptr; // playDirectory()

	// current file and directory while playing:
	DirectoryPtr current_dir;
	FilePtr		 input_file;
	cstr		 fn_pattern = nullptr; // e.g. "C*"

	bool paused		 = false;
	bool repeat_file = false;
	bool repeat_dir	 = false;
};

} // namespace kilipili
