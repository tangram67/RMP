/*
 * aifftypes.h
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef AIFFTYPES_H_
#define AIFFTYPES_H_

#include "chunktypes.h"
#include "id3types.h"
#include "IEEE754.h"
#include "windows.h"
#include "gcc.h"

namespace music {

typedef struct CAIFFFileHeader {
	TChunkID	aiChunkID;			// Offset 0  "FORM"
	DWORD		aiChunkSize;		// Offset 4
	TChunkID	aiFormType;			// Offset 8  "AIFF"
} PACKED TAIFFFileHeader;


typedef struct CAIFFCommonChunk {
	TChunkID		aiChunkID;			// Offset 0  "COMM"
	DWORD			aiChunkSize;		// Offset 4  = 18
	INT16			aiNumChannels;		// Offset 6	 2 Byte
	DWORD			aiNumSampleFrames;	// Offset 10 4 Byte
	INT16 			aiSampleSize;		// Offset 14 2 Byte
	util::TIEEE754 	aiSampleRate;		// Offset 16 10 Byte --> 18 Byte
} PACKED TAIFFCommonChunk;


typedef struct CAIFFSoundChunk {
	TChunkID	aiChunkID;			// Offset 0  "SSND"
	DWORD		aiChunkSize;		// Offset 4
	DWORD		aiOffset;			// Offset 8  4 Byte
	DWORD		aiBlockSize;		// Offset 12 4 Byte
} PACKED TAIFFSoundChunk;


typedef struct CAIFFID3Chunk {
	TChunkID	aiChunkID;			// Offset 0  "ID3 "
	DWORD		aiChunkSize;		// Offset 4
} PACKED TAIFFID3Chunk;


} /* namespace music */

#endif /* AIFFTYPES_H_ */
