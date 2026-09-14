// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "AudioPlayer.h"
#include "Devices/Directory.h"
#include "Devices/File.h"
#include "Devices/FileSystem.h"
#include "Logger.h"
#include "cstrings.h"


using namespace kilipili::Devices;

namespace kilipili::Audio
{

AudioPlayer::AudioPlayer(cstr fn_pattern) noexcept : fn_pattern(fn_pattern) {}

AudioPlayer::~AudioPlayer() noexcept
{
	delete[] next_dir;
	delete[] next_file;
}

void AudioPlayer::play(cstr fpath)
{
	delete[] next_file;
	next_file = newcopy(makeFullPath(fpath));
	debugstr("play file: %s\n", next_file);
}

void AudioPlayer::skip() noexcept
{
	input_file = nullptr; //
}

void AudioPlayer::playDirectory(cstr dpath)
{
	delete[] next_dir;
	next_dir = newcopy(makeFullPath(dpath));
	debugstr("play dir: %s\n", next_dir);
}

void AudioPlayer::play(cstr fpath, bool loop)
{
	play(fpath);
	repeat_file = loop;
}

void AudioPlayer::playDirectory(cstr dpath, bool loop)
{
	playDirectory(dpath);
	repeat_dir = loop;
}

void AudioPlayer::stop() noexcept
{
	skip();
	stopAfterSong();
}

void AudioPlayer::stopAfterSong() noexcept
{
	current_dir = nullptr;
	delete[] next_dir;
	delete[] next_file;
	next_dir	= nullptr;
	next_file	= nullptr;
	repeat_file = false;
	paused		= false;
}

//void AudioPlayer::setVolume(float) noexcept {}

File* AudioPlayer::nextInputFile()
{
	if (input_file && repeat_file)
	{
		input_file->setFpos(0);
		return input_file;
	}

	input_file = nullptr; // close it

	if (next_file)
	{
		// we are not playing
		// but there's a music file requested to play:
	a:
		logline("now playing: %s", next_file);

		cstr fname = dupstr(next_file);
		delete[] next_file;
		next_file  = nullptr; // if open() fails we don't want to come here again
		input_file = openFile(fname);
		return input_file;
	}

	if (current_dir)
	{
		// we are not playing and there is no file requested to play
		// but we are playing from a directory:
	b:
		FileInfo finfo = current_dir->next(fn_pattern);
		if (!finfo && repeat_dir && !next_dir)
		{
			current_dir->rewind();
			finfo = current_dir->next(fn_pattern); // NULL if nothing matches
		}

		if (finfo) // NULL if end of dir
		{
			next_file = newcopy(catstr(current_dir->getFullPath(), "/", finfo.fname));
			goto a; // return nextInputFile();
		}

		current_dir = nullptr; // close it
	}

	if (next_dir)
	{
		// we are not playing and there is no file requested to play
		// and we are not playing from a directory
		// but there is a request for a directory to play:

		cstr dpath = dupstr(next_dir);
		delete[] next_dir;
		next_dir	= nullptr; // we don't want to come back to here if openDir() fails
		current_dir = openDir(dpath);
		current_dir->rewind();
		goto b; // return nextInputFile();
	}

	return input_file; // nullptr
}


} // namespace kilipili::Audio


/*





























*/
