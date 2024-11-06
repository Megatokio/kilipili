// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "exportStSoundWavFile.h"
#include "extern/StSoundLibrary/StSoundLibrary.h"
#include <cstdio>

namespace kio::Audio
{

struct WAVHeader
{
	uint8  RIFFMagic[4]	 = {'R', 'I', 'F', 'F'};
	uint32 FileLength	 = 0; // length of data that follows
	uint8  FileType[4]	 = {'W', 'A', 'V', 'E'};
	uint8  FormMagic[4]	 = {'f', 'm', 't', ' '};
	uint32 FormLength	 = htole32(0x10);
	uint16 SampleFormat	 = htole16(1);
	uint16 NumChannels	 = htole16(1);
	uint32 PlayRate		 = htole32(44100);
	uint32 BytesPerSec	 = htole32(44100 * (16 / 8));
	uint16 Pad			 = htole16(16 / 8);
	uint16 BitsPerSample = htole16(16);
	uint8  DataMagic[4]	 = {'d', 'a', 't', 'a'};
	uint32 DataLength	 = 0;
};

void exportStSoundWavFile(cstr filename, cstr destfile)
{
	YMMUSIC* pMusic = ymMusicCreate();

	if (ymMusicLoad(pMusic, filename) == false)
	{
		auto err = ymMusicGetLastError(pMusic);
		ymMusicDestroy(pMusic);
		throw usingstr("Error in loading file %s:\n%s\n", filename, err);
	}

	ymMusicInfo_t info;
	ymMusicGetInfo(pMusic, &info);

	printf("Generating wav file from \"%s\"\n", filename);
	printf("%s\n%s\n(%s)\n", info.pSongName, info.pSongAuthor, info.pSongComment);

	FILE* out = fopen(destfile, "wb");
	if (!out)
	{
		ymMusicDestroy(pMusic);
		throw usingstr("Unable to create file \"%s\"\n", destfile);
	}

	WAVHeader head;
	fwrite(&head, 1, sizeof(WAVHeader), out); // reserve space

	ymMusicSetLoopMode(pMusic, YMFALSE);
	ymu32		   totalNbSample	 = 0;
	constexpr uint NBSAMPLEPERBUFFER = 1024;
	ymsample	   convertBuffer[NBSAMPLEPERBUFFER];

	while (ymMusicCompute(pMusic, convertBuffer, NBSAMPLEPERBUFFER))
	{
		fwrite(convertBuffer, sizeof(ymsample), NBSAMPLEPERBUFFER, out);
		totalNbSample += NBSAMPLEPERBUFFER;
	}

	fseek(out, 0, SEEK_SET);
	head.FormLength	   = 0x10;
	head.SampleFormat  = 1;
	head.NumChannels   = 1;
	head.PlayRate	   = 44100;
	head.BitsPerSample = 16;
	head.BytesPerSec   = 44100 * (16 / 8);
	head.Pad		   = (16 / 8);
	head.DataLength	   = totalNbSample * (16 / 8);
	head.FileLength	   = head.DataLength + sizeof(WAVHeader) - 8;
	fwrite(&head, 1, sizeof(WAVHeader), out);
	fseek(out, 0, SEEK_END);
	fclose(out);
	printf("%d samples written (%.02f MB).\n", totalNbSample, double(totalNbSample * sizeof(ymsample)) / (1024 * 1024));
	ymMusicDestroy(pMusic);
}

} // namespace kio::Audio
