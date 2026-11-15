// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "AudioPlayer.h"
#include "Logger.h"
#include "cstrings.h"
#include "devices/Directory.h"
#include "devices/File.h"
#include "devices/FileSystem.h"


namespace kilipili
{

AudioPlayer::AudioPlayer(cstr fn_pattern) noexcept : dflt_pattern(fn_pattern) {}

AudioPlayer::~AudioPlayer() noexcept
{
	delete[] fn_pattern;
	delete[] next_dir;
	delete[] next_file;
}

void AudioPlayer::playFile(cstr fpath)
{
	delete[] next_file;
	next_file = newcopy(resolveFilePath(fpath));
	debugstr("play file: %s\n", next_file);
}

void AudioPlayer::skip() noexcept
{
	input_file = nullptr; //
}

void AudioPlayer::playDirectory(cstr dpath)
{
	delete[] next_dir;
	// must include the device, so user can change workdir:
	next_dir = newcopy(resolveDirPath(dpath));
	debugstr("play dir: %s\n", next_dir);
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
	delete[] fn_pattern;
	next_dir	= nullptr;
	next_file	= nullptr;
	fn_pattern	= nullptr;
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
		next_file = nullptr; // if open() fails we don't want to come here again

		input_file = openFile(fname);
		return input_file;
	}

	if (current_dir)
	{
		// we are not playing and there is no file requested to play
		// but we are playing from a directory:
	b:
		FileInfo finfo = current_dir->next(fn_pattern ? fn_pattern : dflt_pattern);
		if (!finfo && repeat_dir && !next_dir)
		{
			current_dir->rewind();
			finfo = current_dir->next(fn_pattern ? fn_pattern : dflt_pattern); // NULL if nothing matches
		}

		if (finfo) // NULL if end of dir
		{
			cstr dirpath = current_dir->getFullPath();
			assert(lastchar(dirpath) != '/');
			next_file = newcopy(catstr(dirpath, "/", finfo.fname));
			goto a;
		}

		current_dir = nullptr; // close it
		delete[] fn_pattern;
		fn_pattern = nullptr;
	}

	if (next_dir)
	{
		// we are not playing and there is no file requested to play
		// and we are not playing from a directory
		// but there is a request for a directory to play:

		str dpath = dupstr(next_dir);
		delete[] next_dir;
		next_dir = nullptr; // we don't want to come back to here if openDir() fails

		FileSystemPtr fs = getFS(dpath);

		ptr lc = strchr(dpath, 0) - 1;
		if (*lc == '/') *lc = 0;
		else
		{
			cstr pattern = nullptr;
			if (fs->getFileType(dpath) != DirectoryFile) //
				dpath = fs->resolveDirPathWithPattern(dpath, &pattern);
			assert(fn_pattern == nullptr);
			fn_pattern = newcopy(pattern);
		}
		current_dir = fs->openDir(dpath);


		debugstr("now playing directory: %s\n", dpath);
		goto b;
	}

	return input_file; // nullptr
}


} // namespace kilipili


/*





























*/
