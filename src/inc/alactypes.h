/*
 * alactypes.h
 *
 *  Created on: 01.10.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ALACTYPES_H_
#define ALACTYPES_H_

#include "gcc.h"
#include "atoms.h"
#include "windows.h"
#include "chunktypes.h"

namespace music {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TChunkSize = DWORD;

#else

typedef DWORD TChunkSize;

#endif


typedef struct CAlacFileHeader {
	TChunkSize	alChunkSize;	// Offset 0
	TChunkID	alChunkID;		// Offset 4  "ftyp"
	TTypeID		alTypeID;		// Offset 8  "M4A "
} PACKED TAlacFileHeader;


typedef struct CAlacAtomHeader {
	TChunkSize	alChunkSize;	// Offset 0
	TChunkID	alChunkID;		// Offset 4
	DWORD		alVersionFlag;	// Offset 8
} PACKED TAlacAtomHeader;


// Apple Lossless Magic Cookie
typedef struct CAlacStreamChunk {
	TChunkSize	alChunkSize;		// Offset 0
	TChunkID	alChunkID;			// Offset 4  "stsd"
	DWORD		alVersionFlag;		// Offset 8  = 0
	DWORD		alEntryCount;		// Offset 12 = 1
	DWORD		alSampleEntrySize; 	// Offset 16 = sizeof(AudioSampleEntry)(36) + sizeof(full ISO box header)(12) + sizeof(Apple Lossless Magic Cookie)
	TTypeID		alEntryType;  		// Offset 20 "alac"
	BYTE		alReserved1[7];		// Offset 24
	BYTE		alReferenceIndex;	// Offset 31 = 1
	BYTE		alReserved2[8];		// Offset 32
	WORD		alNumChannels;		// Offset 40
	WORD		alSampleSize;		// Offset 42
	BYTE		alReserved3[2];		// Offset 44
	DWORD		alSampleRate;		// Offset 48
} PACKED TAlacStreamChunk;


typedef struct CAlacMediaChunk {
	TChunkSize	alChunkSize;		// Offset 0
	TChunkID	alChunkID;			// Offset 4  "mdhd"
	DWORD		alVersionFlag;		// Offset 8  = 0
	BYTE		alReserved[8];		// Offset 12
	DWORD		alSampleRate;		// Offset 20
	DWORD		alNumFrames;		// Offset 24 --> Duration = alNumFrames / alSampleRate
} PACKED TAlacMediaChunk;


typedef struct CAlacTagChunk {
	TChunkSize	alChunkSize;		// Offset 0
	DWORD		alChunkID;			// Offset 4
	DWORD		alDataSize;			// Offset 8
	DWORD		alDataID;			// Offset 12 "data"
	DWORD		alVersionFlag;		// Offset 16 = 0
	BYTE		alReserved[4];		// Offset 20
} PACKED TAlacTagChunk;


typedef struct CAlacBlockHeader {
	BYTE		alBlockID;			// Offset 0  = 0x20
	BYTE		alReserved[4];		// Offset 1
	BYTE		alByte1;			// Offset 5  = 0x13
	BYTE		alByte2;			// Offset 5  = var.
} PACKED TAlacBlockHeader;


STATIC_CONST BYTE ALAC_BLOCK_ID = 0x20;

STATIC_CONST DWORD ALAC_TAG_ATOM_1			= MAKE_ATOM( 'd', 'a', 't', 'a' );   // 'data'
STATIC_CONST DWORD ALAC_TAG_ATOM_2			= MAKE_ATOM( 'm', 'e', 'a', 'n' );   // 'mean'

STATIC_CONST DWORD ALAC_META_ATOM			= MAKE_ATOM( 'm', 'e', 't', 'a' );   // 'meta'
STATIC_CONST DWORD ALAC_ILST_ATOM			= MAKE_ATOM( 'i', 'l', 's', 't' );   // 'ilst'
STATIC_CONST DWORD ALAC_MDHD_ATOM			= MAKE_ATOM( 'm', 'd', 'h', 'd' );   // 'mdhd'

STATIC_CONST DWORD ALAC_MOOV_ATOM			= MAKE_ATOM( 'm', 'o', 'o', 'v' );   // 'moov'
STATIC_CONST DWORD ALAC_TRAK_ATOM			= MAKE_ATOM( 't', 'r', 'a', 'k' );   // 'trak'
STATIC_CONST DWORD ALAC_MDIA_ATOM			= MAKE_ATOM( 'm', 'd', 'i', 'a' );   // 'mdia'
STATIC_CONST DWORD ALAC_MINF_ATOM			= MAKE_ATOM( 'm', 'i', 'n', 'f' );   // 'minf'
STATIC_CONST DWORD ALAC_STBL_ATOM			= MAKE_ATOM( 's', 't', 'b', 'l' );   // 'stbl'
STATIC_CONST DWORD ALAC_UDTA_ATOM			= MAKE_ATOM( 'u', 'd', 't', 'a' );   // 'udta'
STATIC_CONST DWORD ALAC_STSD_ATOM			= MAKE_ATOM( 's', 't', 's', 'd' );   // 'stsd'
STATIC_CONST DWORD ALAC_ESDS_ATOM			= MAKE_ATOM( 'e', 's', 'd', 's' );   // 'esds'

STATIC_CONST DWORD ALAC_TITLE_ATOM			= MAKE_ATOM( 0xa9, 'n', 'a', 'm' );  // '©nam'
STATIC_CONST DWORD ALAC_ARTIST_ATOM			= MAKE_ATOM( 0xa9, 'A', 'R', 'T' );  // '©ART'
STATIC_CONST DWORD ALAC_ALBUMARTIST_ATOM	= MAKE_ATOM(  'a', 'A', 'R', 'T' );  // 'aART'
STATIC_CONST DWORD ALAC_PERFORMER_ATOM		= MAKE_ATOM(  'p', 'e', 'r', 'f' );  // 'perf'
STATIC_CONST DWORD ALAC_ALBUM_ATOM			= MAKE_ATOM( 0xa9, 'a', 'l', 'b' );  // '©alb'
STATIC_CONST DWORD ALAC_DAY_ATOM			= MAKE_ATOM( 0xa9, 'd', 'a', 'y' );  // '©day'
STATIC_CONST DWORD ALAC_CUST_GENRE_ATOM		= MAKE_ATOM( 0xa9, 'g', 'e', 'n' );  // '©gen'
STATIC_CONST DWORD ALAC_GENRE_ATOM			= MAKE_ATOM(  'g', 'n', 'r', 'e' );  // 'gnre'
STATIC_CONST DWORD ALAC_TRACK_ATOM			= MAKE_ATOM(  't', 'r', 'k', 'n' );  // 'trkn'
STATIC_CONST DWORD ALAC_DISK_ATOM			= MAKE_ATOM(  'd', 'i', 's', 'k' );  // 'disk'
STATIC_CONST DWORD ALAC_COVER_ART_ATOM		= MAKE_ATOM(  'c', 'o', 'v', 'r' );  // 'covr'
STATIC_CONST DWORD ALAC_COMPILATION_ATOM	= MAKE_ATOM(  'c', 'p', 'i', 'l' );  // 'cpil'
STATIC_CONST DWORD ALAC_COMMENT_ATOM		= MAKE_ATOM( 0xa9, 'c', 'm', 't' );  // 'cpil'
STATIC_CONST DWORD ALAC_LYRICS_ATOM			= MAKE_ATOM( 0xa9, 'l', 'y', 'r' );  // '©lyr'
STATIC_CONST DWORD ALAC_COMPOSER_ATOM		= MAKE_ATOM( 0xa9, 'w', 'r', 't' );  // '©wrt'
STATIC_CONST DWORD ALAC_WILDCARD_ATOM		= MAKE_ATOM(  '-', '-', '-', '-' );  // '----'


STATIC_CONST DWORD ALAC_HAS_CHILD_ATOMS[] = {
		ALAC_MOOV_ATOM,
		ALAC_TRAK_ATOM,
		ALAC_MDIA_ATOM,
		ALAC_MINF_ATOM,
		ALAC_STBL_ATOM,
		ALAC_UDTA_ATOM,
		0
};

STATIC_CONST DWORD ALAC_TAG_ATOMS[] = {
		ALAC_TITLE_ATOM,
		ALAC_ARTIST_ATOM,
		ALAC_ALBUMARTIST_ATOM,
		ALAC_ALBUM_ATOM,
		ALAC_DAY_ATOM,
		ALAC_CUST_GENRE_ATOM,
		ALAC_GENRE_ATOM,
		ALAC_TRACK_ATOM,
		ALAC_DISK_ATOM,
		ALAC_COVER_ART_ATOM,
		ALAC_COMPILATION_ATOM,
		ALAC_COMMENT_ATOM,
		ALAC_LYRICS_ATOM,
		ALAC_COMPOSER_ATOM,
		0
};


} /* namespace music */

#endif /* ALACTYPES_H_ */
