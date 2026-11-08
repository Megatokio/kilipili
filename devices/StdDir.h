// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Directory.h"
#include <dirent.h>

/*
	class `StdDir` represents a directory in the normal filesystem of the host system.
	it can be created in two ways:

	- indirectly by calling the FileSystem function `openDir("disk:/some/path")`
	  then the directory uses 1-byte character encoding for paths and filenames,
	  currently always latin-1, the standard encoding on the RP Pico.
	  StdDir is used in the desktop emulator only, not on the Pico itself.

	- directly by creating it with `new StdDir("/some/path")`
	  then the directory uses utf-8 character encoding for paths and filenames,
	  assuming this is not the emulator but some desktop tool.
*/

namespace kilipili
{

class StdDir : public Directory
{
	NO_COPY_MOVE(StdDir);

public:
	StdDir(cstr path_utf8) throws;		 // "/path/to/dir"
	virtual ~StdDir() noexcept override; //

	virtual void	 rewind() throws override;
	virtual FileInfo next(cstr pattern = nullptr) throws override;

	// you can only call StdDir::getFullPath() if you created the StdDir directly.
	// then getFullPath() returns the utf-8 encoded host-style path: "/path/to/dir"
	// else you actually call Directory::getFullPath()
	// which returns the ucs1-encoded dos-like path: "disk:/path/to/dir"
	cstr getFullPath() const noexcept { return dirpath_utf8; }

private:
	friend class StdFS;

	DIR* dir		  = nullptr;
	cstr dirpath_utf8 = nullptr; // "/path/to/dir"

	FileInfo makeFileInfo(const dirent*);
	StdDir(RCPtr<FileSystem>, cstr path_utf8, cstr path_latin1) throws; // "/path/to/dir", "disk:/path/to/dir"
};

} // namespace kilipili


/*

























*/
