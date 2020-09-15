/*
 * dsdtypes.h
 *
 *  Created on: 14.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef DSDTYPES_H_
#define DSDTYPES_H_

#include "chunktypes.h"
#include "id3types.h"
#include "windows.h"
#include "gcc.h"

namespace music {

enum EDSDType {
	EDT_UNKOWN,
	EDT_DSF,
	EDT_DFF
};


typedef struct CDSFHeader {
	TChunkID	dsChunkID;			// Offset 0  "DSD "
	QWORD		dsChunkSize;		// Offset 4	 = 28 Byte!
	QWORD		dsFileSize;			// Offset 12
	QWORD		dsOffsetToID3Tag;	// Offset 20
} PACKED TDSFHeader;


typedef struct CDSFFormat {
	TChunkID	dsChunkID;			// Offset 0  "fmt "
	QWORD		dsChunkSize;		// Offset 4	 = 52 Byte!
	DWORD		dsVersion;			// Offset 12 = 1
	DWORD		dsFormatID;			// Offset 16 = 0: DSD raw
	DWORD		dsChannelType;		// Offset 20 = 2: stereo
	DWORD		dsChannelNum;		// Offset 24 = 2: stereo
	DWORD		dsSampleRate;		// Offset 28 = 2822400, 5644800
	DWORD		dsBitsPerSample;	// Offset 32 = 1, 8
	QWORD		dsSampleCount;		// Offset 36 = number per channel
	DWORD		dsBlockSize;		// Offset 44 = 4096
	DWORD		dsReserved;			// Offset 48 = 0
} PACKED TDSFFormat;


typedef struct CDSFData {
	TChunkID	dsChunkID;			// Offset 0  "data"
	QWORD		dsChunkSize;		// Offset 4
} PACKED TDSFData;


typedef struct CDSFFileHeader {
	TDSFHeader	header;
	TDSFFormat	format;
	TDSFData	data;
} PACKED TDSFFileHeader;



typedef struct CDFFFileHeader {
	TChunkID	dfChunkID;			// Offset 0  "FRM8"
	QWORD		dfChunkSize;		// Offset 4
} PACKED TDFFFileHeader;


typedef struct CDFFDataChunk {
	TChunkID	dfChunkID;			// Offset 0  "DSD "
	QWORD		dfChunkSize;		// Offset 4
} PACKED TDFFDataChunk;


typedef struct CDFFSampleRate {
	TChunkID	dfChunkID;			// Offset 0  "FS "
	QWORD		dfChunkSize;		// Offset 4
	DWORD		dfSampleRate;		// Offset 12
} PACKED TDFFSampleRate;


typedef struct CDFFChannels {
	TChunkID	dfChunkID;			// Offset 0  "CHNL"
	QWORD		dfChunkSize;		// Offset 4
	WORD		dfNumChannels;		// Offset 12
} PACKED TDFFChannels;


typedef struct CDFFSpeakerConfig {
	TChunkID	dfChunkID;			// Offset 0  "LSCO"
	QWORD		dfChunkSize;		// Offset 4
} PACKED TDFFSpeakerConfig;


typedef struct CDFFFormChunk {
	TChunkID	dfChunkID1;			// Offset 0  "FRM8"
	QWORD		dfChunkSize;		// Offset 4
	TChunkID	dfChunkID2;			// Offset 12 "DSD "
} PACKED TDFFFormChunk;


typedef struct CDFFSoundDataChunk {
	TChunkID	dfChunkID;			// Offset 0  "DSD "
	QWORD		dfChunkSize;		// Offset 4
} PACKED TDFFSoundDataChunk;


typedef struct CDFFID3Header {
	TChunkID		dfChunkID;		// Offset 0  "ID3 "
	QWORD			dfChunkSize;	// Offset 4
	TID3v2Header	dfID3Header;	// Offset 16
} PACKED TDFFID3Header;


} /* namespace music */

#endif /* DSDTYPES_H_ */
