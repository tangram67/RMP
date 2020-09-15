/*
 * mp3types.h
 *
 *  Created on: 08.11.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef MP3TYPES_H_
#define MP3TYPES_H_

namespace music {

// Layer index values:
// 0 0 - Not defined, invalid
// 0 1 - Layer III
// 1 0 - Layer II
// 1 1 - Layer I
enum MPEGLayer {
	MPEG_UNKNOWN = 0,
	MP1 = 1,
	MP2 = 2,
	MP3 = 3
};

typedef struct CMP3FrameValues {
	int  sync;
	int  version;
	int  layer;
	bool protection;
	int  bitrate;
	int  samplerate;
	bool padding;
	bool option;
	int  mode;
	int  extmode;
	bool copyright;
	bool original;
	int  emphasis;
} TMP3FrameValues;

typedef struct CMP3FrameParams {
	MPEGLayer layer;
	int    version;
	size_t framesize;
	size_t bitrate;
	size_t samplerate;
	int    channels;
} TMP3FrameParams;

typedef struct CMP3FrameHeader {
	TMP3FrameValues values;
	TMP3FrameParams stream;
} TMP3FrameHeader;


static const int MP3Versions[4] = {
	2 /*2.5*/, 0, 2, 1
};

static const size_t MP3BitRates[2][3][16] = // in Bits per sec
{
	{ // MPEG 2 & 2.5
		{ 0,  8000, 16000, 24000, 32000, 40000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 }, // Layer III
		{ 0,  8000, 16000, 24000, 32000, 40000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 }, // Layer II
		{ 0, 32000, 48000, 56000, 64000, 80000, 96000,112000, 128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 }  // Layer I
	},
	{ // MPEG 1
		{ 0, 32000, 40000, 48000,  56000,  64000,  80000,  96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 }, // Layer III
		{ 0, 32000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 384000, 0 }, // Layer II
		{ 0, 32000, 64000, 96000, 128000, 160000, 192000, 224000, 256000, 288000, 320000, 352000, 384000, 416000, 448000, 0 }  // Layer I
	}
};

static const size_t MP3SampleRates[4][4] = // in Samples per sec
{
	{ 32000, 16000,  8000, 0 }, // MPEG 2.5
	{     0,     0,     0, 0 }, // reserved
	{ 22050, 24000, 16000, 0 }, // MPEG 2
	{ 44100, 48000, 32000, 0 }  // MPEG 1
};

} /* namespace music */

#endif /* MP3TYPES_H_ */
