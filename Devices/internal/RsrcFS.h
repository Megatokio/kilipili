// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Array.h"
#include "Directory.h"
#include "FileSystem.h"

extern const unsigned char resource_file_data[] __weak_symbol;

namespace kio::Devices
{

/*	class RsrcFS is the FileSystem for the resource files stored in Flash.
	The resources are uint8[] arrays written by class RsrcFileEncode. (in desktop_tools/rsrc_writer/)
	Compressed files are decoded by HeatShrinkDecoder.

	uncompressed[] =
	  char[] filename   0-terminated string
	  uint32 size       sizeof data[]
	  char[] data       uncompressed file data

	compressed[] =
	  char[] filename   0-terminated string
	  uint32 size       uncompressed data size | 0x80000000
	  uint24 csize      sizeof cdata[]
	  uint8  flags		wbits<<4 + lbits
	  char[] data       compressed file data

ALT uncompressed[] =
	  char[] filename   0-terminated string
	  uint24 size       data size (after flag)
	  uint8  flag = 0
	  char[] data       uncompressed file data

ALT	compressed[] =
	  char[] filename   0-terminated string
	  uint24 csize      compressed size (incl. usize)
	  uint8  flags != 0 windowsize<<4 + lookaheadsize
	  uint32 usize      uncompressed size
	  char[] data       compressed file data
*/

class RsrcFS final : public FileSystem
{
public:
	RsrcFS(cstr name = "rsrc") throws : FileSystem(name) {}

	virtual uint64		 getFree() override { return 0; }
	virtual uint64		 getSize() override;
	virtual DirectoryPtr openDir(cstr path) override;
	virtual FilePtr		 openFile(cstr path, FileOpenMode mode = READ) override;

	virtual FileType getFileType(cstr path) noexcept override;
	virtual void	 makeDir(cstr) throws override { throw NOT_WRITABLE; }
	virtual void	 remove(cstr) throws override { throw NOT_WRITABLE; }
	virtual void	 rename(cstr, cstr) throws override { throw NOT_WRITABLE; }
	virtual void	 setFmode(cstr, FileMode, uint8) throws override { throw NOT_WRITABLE; }
	virtual void	 setMtime(cstr, uint32) throws override { throw NOT_WRITABLE; }
};


class RsrcDir final : public Directory
{
public:
	virtual void	 rewind() throws override;
	virtual FileInfo next(cstr pattern = nullptr) throws override;

private:
	cuptr		dpos = nullptr;
	Array<cstr> subdirs; // so far returned by next()

	friend class RsrcFS;
	RsrcDir(RCPtr<RsrcFS> fs, cstr full_path);
	bool is_in_subdirs(cstr path, cptr sep);
};

} // namespace kio::Devices
