/*
 * pcmtypes.h
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef PCMTYPES_H_
#define PCMTYPES_H_

#include "atoms.h"
#include "windows.h"
#include "chunktypes.h"

namespace music {

/**
 * Types from windows.h
 *
 * typedef uint8_t  BYTE	1
 * typedef uint32_t DWORD	4
 * typedef int32_t  LONG	4
 * typedef uint16_t WORD	2
 *
 *
 * See http://soundfile.sapp.org/doc/WaveFormat/
 *
 * Offset  Size  Type    Name             Description
 * ======================================================================================
 *
 * The canonical WAVE format starts with the RIFF header:
 *
 * 0         4   DWORD   ChunkID          Contains the letters "RIFF" in ASCII form
 *                                        (0x52494646 big-endian form)
 *                                        (0x46464952 little-endian form)
 * 4         4   DWORD   ChunkSize        36 + SubChunk2Size, or more precisely:
 *                                        4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
 *                                        This is the size of the rest of the chunk
 *                                        following this number.  This is the size of the
 *                                        entire file in bytes minus 8 bytes for the
 *                                        two fields not included in this count:
 *                                        ChunkID and ChunkSize.
 * 8         4   DWORD   Format           Contains the letters "WAVE"
 *                                        (0x57415645 big-endian form).
 *                                        (0x45564157 little-endian form)
 *
 * The "WAVE" format consists of two subchunks: "fmt " and "data":
 * The "fmt " subchunk describes the sound data's format:
 *
 * 12        4   DWORD   Subchunk1ID      Contains the letters "fmt "
 *                                        (0x46464952 little-endian form)
 *                                        (0x20746d66 big-endian ASCII form).
 * 16        4   DWORD   Subchunk1Size    16 for PCM.  This is the size of the
 *                                        rest of the Subchunk which follows this number.
 * 20        2   WORD    AudioFormat      PCM = 1 (i.e. Linear quantization)
 *                                        Values other than 1 indicate some
 *                                        form of compression.
 * 22        2   WORD    NumChannels      Mono = 1, Stereo = 2, etc.
 * 24        4   DWORD   SampleRate       8000, 44100, etc.
 * 28        4   DWORD   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
 * 32        2   WORD    BlockAlign       == NumChannels * BitsPerSample/8
 *                                        The number of bytes for one sample including
 *                                        all channels. I wonder what happens when
 *                                        this number isn't an integer?
 * 34        2   WORD    BitsPerSample    8 bits = 8, 16 bits = 16, etc.
 *
 *  Unused data (!), because we are always using PCM format:
 *
 *           2   WORD    ExtraParamSize   if PCM, then doesn't exist
 *           X   ....    ExtraParams      space for extra parameters
 *
 * The "data" subchunk contains the size of the data and the actual sound:
 *
 * 36        4   DWORD   Subchunk2ID      Contains the letters "data"
 *                                        (0x64617461 big-endian form).
 *                                        (0x61746164 little-endian form)
 * 40        4   DWORD   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
 *                                        This is the number of bytes in the data.
 *                                        You can also think of this as the size
 *                                        of the read of the subchunk following this
 *                                        number.
 * 44        *           Data             The actual sound data.
 *
 */


/*
 * TPCMFileHeader
 */
typedef struct CPCMFileHeader {
	TChunkID	pfChunkID;			// Offset 0  "RIFF"
    DWORD		pfChunkSize;		// Offset 4
    TChunkID	pfFormat;			// Offset 8  "WAVE"
    TChunkID	pfSubchunk1ID;		// Offset 12 "fmt "
    DWORD		pfSubchunk1Size;	// Offset 16
    WORD		pfAudioFormat;		// Offset 20
    WORD		pfNumChannels;		// Offset 22
    DWORD		pfSampleRate;		// Offset 24
    DWORD		pfByteRate;			// Offset 28
    WORD		pfBlockAlign;		// Offset 32
    WORD		pfBitsPerSample;	// Offset 34
    TChunkID	pfSubchunk2ID;		// Offset 36 "data"
    DWORD		pfSubchunk2Size;	// Offset 40
} PACKED TPCMFileHeader;


typedef struct CPCMBaseHeader {
	TChunkID	pfChunkID;			// Offset 0  "RIFF"
    DWORD		pfChunkSize;		// Offset 4
    TChunkID	pfFormat;			// Offset 8  "WAVE"
    TChunkID	pfSubchunk1ID;		// Offset 12 "fmt "
    DWORD		pfSubchunk1Size;	// Offset 16
    WORD		pfAudioFormat;		// Offset 20
    WORD		pfNumChannels;		// Offset 22
    DWORD		pfSampleRate;		// Offset 24
    DWORD		pfByteRate;			// Offset 28
    WORD		pfBlockAlign;		// Offset 32
    WORD		pfBitsPerSample;	// Offset 34
} PACKED TPCMBaseHeader;

typedef struct CPCMRF64Header {
	TChunkID	pfChunkID;			// Offset 0  "RF64"
    DWORD		pfChunkSize;		// Offset 4   FFFFFFFFFF = -1
    QWORD		pfChunkSize64;		// Offset 8
    TChunkID	pfFormat;			// Offset 16  "WAVE" ????
    TChunkID	pfSubchunk1ID;		// Offset 20 "fmt "
    DWORD		pfSubchunk1Size;	// Offset 24
    WORD		pfAudioFormat;		// Offset 28
    WORD		pfNumChannels;		// Offset 30
    DWORD		pfSampleRate;		// Offset 32
    DWORD		pfByteRate;			// Offset 36
    WORD		pfBlockAlign;		// Offset 40
    WORD		pfBitsPerSample;	// Offset 42
} PACKED TPCMRF64Header;


typedef struct CPCMInfoHeader {
	TChunkID	pfChunkID;			// Offset 0  "LIST"
    DWORD		pfChunkSize;		// Offset 4
    TChunkID	pfInfoID;			// Offset 8  "INFO"
} PACKED TPCMInfoHeader;


typedef struct CPCMChunkHeader {
	TChunkID	pfChunkID;			// Offset 0  "????"
    DWORD		pfChunkSize;		// Offset 4
} PACKED TPCMChunkHeader;

typedef struct CPCMAtomHeader {
	DWORD		pfChunkID;			// Offset 0  "????"
    DWORD		pfChunkSize;		// Offset 4
} PACKED TPCMAtomHeader;


// See http://www.onicos.com/staff/iz/formats/

/* Windows WAVE File Encoding Tags */
#define WAVE_FORMAT_UNKNOWN					0x0000 /* Unknown Format */
#define WAVE_FORMAT_PCM						0x0001 /* PCM */
#define WAVE_FORMAT_ADPCM					0x0002 /* Microsoft ADPCM Format */
#define WAVE_FORMAT_IEEE_FLOAT				0x0003 /* IEEE Float */
#define WAVE_FORMAT_VSELP					0x0004 /* Compaq Computer's VSELP */
#define WAVE_FORMAT_IBM_CSVD				0x0005 /* IBM CVSD */
#define WAVE_FORMAT_ALAW					0x0006 /* ALAW */
#define WAVE_FORMAT_MULAW					0x0007 /* MULAW */
#define WAVE_FORMAT_OKI_ADPCM				0x0010 /* OKI ADPCM */
#define WAVE_FORMAT_DVI_ADPCM				0x0011 /* Intel's DVI ADPCM */
#define WAVE_FORMAT_MEDIASPACE_ADPCM		0x0012 /*Videologic's MediaSpace ADPCM*/
#define WAVE_FORMAT_SIERRA_ADPCM			0x0013 /* Sierra ADPCM */
#define WAVE_FORMAT_G723_ADPCM				0x0014 /* G.723 ADPCM */
#define WAVE_FORMAT_DIGISTD					0x0015 /* DSP Solution's DIGISTD */
#define WAVE_FORMAT_DIGIFIX					0x0016 /* DSP Solution's DIGIFIX */
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM		0x0017 /* Dialogic OKI ADPCM */
#define WAVE_FORMAT_MEDIAVISION_ADPCM		0x0018 /* MediaVision ADPCM */
#define WAVE_FORMAT_CU_CODEC				0x0019 /* HP CU */
#define WAVE_FORMAT_YAMAHA_ADPCM			0x0020 /* Yamaha ADPCM */
#define WAVE_FORMAT_SONARC					0x0021 /* Speech Compression's Sonarc */
#define WAVE_FORMAT_TRUESPEECH				0x0022 /* DSP Group's True Speech */
#define WAVE_FORMAT_ECHOSC1					0x0023 /* Echo Speech's EchoSC1 */
#define WAVE_FORMAT_AUDIOFILE_AF36			0x0024 /* Audiofile AF36 */
#define WAVE_FORMAT_APTX					0x0025 /* APTX */
#define WAVE_FORMAT_AUDIOFILE_AF10			0x0026 /* AudioFile AF10 */
#define WAVE_FORMAT_PROSODY_1612			0x0027 /* Prosody 1612 */
#define WAVE_FORMAT_LRC						0x0028 /* LRC */
#define WAVE_FORMAT_AC2						0x0030 /* Dolby AC2 */
#define WAVE_FORMAT_GSM610					0x0031 /* GSM610 */
#define WAVE_FORMAT_MSNAUDIO				0x0032 /* MSNAudio */
#define WAVE_FORMAT_ANTEX_ADPCME			0x0033 /* Antex ADPCME */
#define WAVE_FORMAT_CONTROL_RES_VQLPC		0x0034 /* Control Res VQLPC */
#define WAVE_FORMAT_DIGIREAL				0x0035 /* Digireal */
#define WAVE_FORMAT_DIGIADPCM				0x0036 /* DigiADPCM */
#define WAVE_FORMAT_CONTROL_RES_CR10		0x0037 /* Control Res CR10 */
#define WAVE_FORMAT_VBXADPCM				0x0038 /* NMS VBXADPCM */
#define WAVE_FORMAT_ROLAND_RDAC				0x0039 /* Roland RDAC */
#define WAVE_FORMAT_ECHOSC3					0x003A /* EchoSC3 */
#define WAVE_FORMAT_ROCKWELL_ADPCM			0x003B /* Rockwell ADPCM */
#define WAVE_FORMAT_ROCKWELL_DIGITALK		0x003C /* Rockwell Digit LK */
#define WAVE_FORMAT_XEBEC					0x003D /* Xebec */
#define WAVE_FORMAT_G721_ADPCM				0x0040 /* Antex Electronics G.721 */
#define WAVE_FORMAT_G728_CELP				0x0041 /* G.728 CELP */
#define WAVE_FORMAT_MSG723					0x0042 /* MSG723 */
#define WAVE_FORMAT_MPEG					0x0050 /* MPEG Layer 1,2 */
#define WAVE_FORMAT_RT24					0x0051 /* RT24 */
#define WAVE_FORMAT_PAC						0x0051 /* PAC */
#define WAVE_FORMAT_MPEGLAYER3				0x0055 /* MPEG Layer 3 */
#define WAVE_FORMAT_CIRRUS					0x0059 /* Cirrus */
#define WAVE_FORMAT_ESPCM					0x0061 /* ESPCM */
#define WAVE_FORMAT_VOXWARE					0x0062 /* Voxware (obsolete) */
#define WAVE_FORMAT_CANOPUS_ATRAC			0x0063 /* Canopus Atrac */
#define WAVE_FORMAT_G726_ADPCM				0x0064 /* G.726 ADPCM */
#define WAVE_FORMAT_G722_ADPCM				0x0065 /* G.722 ADPCM */
#define WAVE_FORMAT_DSAT					0x0066 /* DSAT */
#define WAVE_FORMAT_DSAT_DISPLAY			0x0067 /* DSAT Display */
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED 	0x0069 /* Voxware Byte Aligned (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC8				0x0070 /* Voxware AC8 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC10			0x0071 /* Voxware AC10 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC16			0x0072 /* Voxware AC16 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC20			0x0073 /* Voxware AC20 (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT24			0x0074 /* Voxware MetaVoice (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29			0x0075 /* Voxware MetaSound (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29HW			0x0076 /* Voxware RT29HW (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR12			0x0077 /* Voxware VR12 (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR18			0x0078 /* Voxware VR18 (obsolete) */
#define WAVE_FORMAT_VOXWARE_TQ40			0x0079 /* Voxware TQ40 (obsolete) */
#define WAVE_FORMAT_SOFTSOUND				0x0080 /* Softsound */
#define WAVE_FORMAT_VOXWARE_TQ60			0x0081 /* Voxware TQ60 (obsolete) */
#define WAVE_FORMAT_MSRT24					0x0082 /* MSRT24 */
#define WAVE_FORMAT_G729A					0x0083 /* G.729A */
#define WAVE_FORMAT_MVI_MV12				0x0084 /* MVI MV12 */
#define WAVE_FORMAT_DF_G726					0x0085 /* DF G.726 */
#define WAVE_FORMAT_DF_GSM610				0x0086 /* DF GSM610 */
//define WAVE_FORMAT_ISIAUDIO				0x0088 /* ISIAudio */
#define WAVE_FORMAT_ONLIVE					0x0089 /* Onlive */
#define WAVE_FORMAT_SBC24					0x0091 /* SBC24 */
#define WAVE_FORMAT_DOLBY_AC3_SPDIF			0x0092 /* Dolby AC3 SPDIF */
#define WAVE_FORMAT_ZYXEL_ADPCM				0x0097 /* ZyXEL ADPCM */
#define WAVE_FORMAT_PHILIPS_LPCBB			0x0098 /* Philips LPCBB */
#define WAVE_FORMAT_PACKED					0x0099 /* Packed */
#define WAVE_FORMAT_RHETOREX_ADPCM			0x0100 /* Rhetorex ADPCM */
#define WAVE_FORMAT_IRAT					0x0101 /* BeCubed Software's IRAT */
#define WAVE_FORMAT_VIVO_G723				0x0111 /* Vivo G.723 */
#define WAVE_FORMAT_VIVO_SIREN				0x0112 /* Vivo Siren */
#define WAVE_FORMAT_DIGITAL_G723			0x0123 /* Digital G.723 */
#define WAVE_FORMAT_CREATIVE_ADPCM			0x0200 /* Creative ADPCM */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8 	0x0202 /* Creative FastSpeech8 */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10 	0x0203 /* Creative FastSpeech10 */
#define WAVE_FORMAT_QUARTERDECK				0x0220 /* Quarterdeck */
#define WAVE_FORMAT_FM_TOWNS_SND			0x0300 /* FM Towns Snd */
#define WAVE_FORMAT_BTV_DIGITAL				0x0400 /* BTV Digital */
#define WAVE_FORMAT_VME_VMPCM				0x0680 /* VME VMPCM */
#define WAVE_FORMAT_OLIGSM					0x1000 /* OLIGSM */
#define WAVE_FORMAT_OLIADPCM				0x1001 /* OLIADPCM */
#define WAVE_FORMAT_OLICELP					0x1002 /* OLICELP */
#define WAVE_FORMAT_OLISBC					0x1003 /* OLISBC */
#define WAVE_FORMAT_OLIOPR					0x1004 /* OLIOPR */
#define WAVE_FORMAT_LH_CODEC				0x1100 /* LH Codec */
#define WAVE_FORMAT_NORRIS					0x1400 /* Norris */
#define WAVE_FORMAT_ISIAUDIO				0x1401 /* ISIAudio */
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS 0x1500 /* Soundspace Music Compression */
#define WAVE_FORMAT_DVM						0x2000 /* DVM */
#define WAVE_FORMAT_EXTENSIBLE				0xFFFE /* SubFormat */
#define WAVE_FORMAT_DEVELOPMENT  	       	0xFFFF /* Development */

//
// See:
//   https://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/RIFF.html#Info
//   http://wiki.audacityteam.org/wiki/WAV
//
// Common used tag atoms:
// Artist (IART)
// Title (INAM) - called "Track Title" in Metadata Editor
// Product (IPRD) - called "Album Title" in Metadata Editor
// Track Number (ITRK) (not specified in the original RIFF standard but players supporting LIST INFO tags often support it)
// Date Created (ICRD) - called "Year" in Metadata Editor
// Genre (IGNR)
// Comments (ICMT)
// Copyright (ICOP)
// Software (ISFT)
//
STATIC_CONST DWORD RIFF_ITRK_ATOM = MAKE_ATOM( 'I','T','R','K' ); // 'ITRK' Track
STATIC_CONST DWORD RIFF_AGES_ATOM = MAKE_ATOM( 'A','G','E','S' ); // 'AGES' Rated
STATIC_CONST DWORD RIFF_CMNT_ATOM = MAKE_ATOM( 'C','M','N','T' ); // 'CMNT' Comment
STATIC_CONST DWORD RIFF_CODE_ATOM = MAKE_ATOM( 'C','O','D','E' ); // 'CODE' EncodedBy
STATIC_CONST DWORD RIFF_COMM_ATOM = MAKE_ATOM( 'C','O','M','M' ); // 'COMM' Comments
STATIC_CONST DWORD RIFF_DIRC_ATOM = MAKE_ATOM( 'D','I','R','C' ); // 'DIRC' Directory
STATIC_CONST DWORD RIFF_DISP_ATOM = MAKE_ATOM( 'D','I','S','P' ); // 'DISP' SoundSchemeTitle
STATIC_CONST DWORD RIFF_DTIM_ATOM = MAKE_ATOM( 'D','T','I','M' ); // 'DTIM' DateTimeOriginal
STATIC_CONST DWORD RIFF_GENR_ATOM = MAKE_ATOM( 'G','E','N','R' ); // 'GENR' Genre
STATIC_CONST DWORD RIFF_IARL_ATOM = MAKE_ATOM( 'I','A','R','L' ); // 'IARL' ArchivalLocation
STATIC_CONST DWORD RIFF_IART_ATOM = MAKE_ATOM( 'I','A','R','T' ); // 'IART' Artist
STATIC_CONST DWORD RIFF_IAS1_ATOM = MAKE_ATOM( 'I','A','S','1' ); // 'IAS1' FirstLanguage
STATIC_CONST DWORD RIFF_IAS2_ATOM = MAKE_ATOM( 'I','A','S','2' ); // 'IAS2' SecondLanguage
STATIC_CONST DWORD RIFF_IAS3_ATOM = MAKE_ATOM( 'I','A','S','3' ); // 'IAS3' ThirdLanguage
STATIC_CONST DWORD RIFF_IAS4_ATOM = MAKE_ATOM( 'I','A','S','4' ); // 'IAS4' FourthLanguage
STATIC_CONST DWORD RIFF_IAS5_ATOM = MAKE_ATOM( 'I','A','S','5' ); // 'IAS5' FifthLanguage
STATIC_CONST DWORD RIFF_IAS6_ATOM = MAKE_ATOM( 'I','A','S','6' ); // 'IAS6' SixthLanguage
STATIC_CONST DWORD RIFF_IAS7_ATOM = MAKE_ATOM( 'I','A','S','7' ); // 'IAS7' SeventhLanguage
STATIC_CONST DWORD RIFF_IAS8_ATOM = MAKE_ATOM( 'I','A','S','8' ); // 'IAS8' EighthLanguage
STATIC_CONST DWORD RIFF_IAS9_ATOM = MAKE_ATOM( 'I','A','S','9' ); // 'IAS9' NinthLanguage
STATIC_CONST DWORD RIFF_IBSU_ATOM = MAKE_ATOM( 'I','B','S','U' ); // 'IBSU' BaseURL
STATIC_CONST DWORD RIFF_ICAS_ATOM = MAKE_ATOM( 'I','C','A','S' ); // 'ICAS' DefaultAudioStream
STATIC_CONST DWORD RIFF_ICDS_ATOM = MAKE_ATOM( 'I','C','D','S' ); // 'ICDS' CostumeDesigner
STATIC_CONST DWORD RIFF_ICMS_ATOM = MAKE_ATOM( 'I','C','M','S' ); // 'ICMS' Commissioned
STATIC_CONST DWORD RIFF_ICMT_ATOM = MAKE_ATOM( 'I','C','M','T' ); // 'ICMT' Comment
STATIC_CONST DWORD RIFF_ICNM_ATOM = MAKE_ATOM( 'I','C','N','M' ); // 'ICNM' Cinematographer
STATIC_CONST DWORD RIFF_ICNT_ATOM = MAKE_ATOM( 'I','C','N','T' ); // 'ICNT' Country
STATIC_CONST DWORD RIFF_ICOP_ATOM = MAKE_ATOM( 'I','C','O','P' ); // 'ICOP' Copyright
STATIC_CONST DWORD RIFF_ICRD_ATOM = MAKE_ATOM( 'I','C','R','D' ); // 'ICRD' DateCreated
STATIC_CONST DWORD RIFF_ICRP_ATOM = MAKE_ATOM( 'I','C','R','P' ); // 'ICRP' Cropped
STATIC_CONST DWORD RIFF_IDIM_ATOM = MAKE_ATOM( 'I','D','I','M' ); // 'IDIM' Dimensions
STATIC_CONST DWORD RIFF_IDIT_ATOM = MAKE_ATOM( 'I','D','I','T' ); // 'IDIT' DateTimeOriginal
STATIC_CONST DWORD RIFF_IDPI_ATOM = MAKE_ATOM( 'I','D','P','I' ); // 'IDPI' DotsPerInch
STATIC_CONST DWORD RIFF_IDST_ATOM = MAKE_ATOM( 'I','D','S','T' ); // 'IDST' DistributedBy
STATIC_CONST DWORD RIFF_IEDT_ATOM = MAKE_ATOM( 'I','E','D','T' ); // 'IEDT' EditedBy
STATIC_CONST DWORD RIFF_IENC_ATOM = MAKE_ATOM( 'I','E','N','C' ); // 'IENC' EncodedBy
STATIC_CONST DWORD RIFF_IENG_ATOM = MAKE_ATOM( 'I','E','N','G' ); // 'IENG' Engineer
STATIC_CONST DWORD RIFF_IGNR_ATOM = MAKE_ATOM( 'I','G','N','R' ); // 'IGNR' Genre
STATIC_CONST DWORD RIFF_IKEY_ATOM = MAKE_ATOM( 'I','K','E','Y' ); // 'IKEY' Keywords
STATIC_CONST DWORD RIFF_ILGT_ATOM = MAKE_ATOM( 'I','L','G','T' ); // 'ILGT' Lightness
STATIC_CONST DWORD RIFF_ILGU_ATOM = MAKE_ATOM( 'I','L','G','U' ); // 'ILGU' LogoURL
STATIC_CONST DWORD RIFF_ILIU_ATOM = MAKE_ATOM( 'I','L','I','U' ); // 'ILIU' LogoIconURL
STATIC_CONST DWORD RIFF_ILNG_ATOM = MAKE_ATOM( 'I','L','N','G' ); // 'ILNG' Language
STATIC_CONST DWORD RIFF_IMBI_ATOM = MAKE_ATOM( 'I','M','B','I' ); // 'IMBI' MoreInfoBannerImage
STATIC_CONST DWORD RIFF_IMBU_ATOM = MAKE_ATOM( 'I','M','B','U' ); // 'IMBU' MoreInfoBannerURL
STATIC_CONST DWORD RIFF_IMED_ATOM = MAKE_ATOM( 'I','M','E','D' ); // 'IMED' Medium
STATIC_CONST DWORD RIFF_IMIT_ATOM = MAKE_ATOM( 'I','M','I','T' ); // 'IMIT' MoreInfoText
STATIC_CONST DWORD RIFF_IMIU_ATOM = MAKE_ATOM( 'I','M','I','U' ); // 'IMIU' MoreInfoURL
STATIC_CONST DWORD RIFF_IMUS_ATOM = MAKE_ATOM( 'I','M','U','S' ); // 'IMUS' MusicBy
STATIC_CONST DWORD RIFF_INAM_ATOM = MAKE_ATOM( 'I','N','A','M' ); // 'INAM' Title
STATIC_CONST DWORD RIFF_IPDS_ATOM = MAKE_ATOM( 'I','P','D','S' ); // 'IPDS' ProductionDesigner
STATIC_CONST DWORD RIFF_IPLT_ATOM = MAKE_ATOM( 'I','P','L','T' ); // 'IPLT' NumColors
STATIC_CONST DWORD RIFF_IPRD_ATOM = MAKE_ATOM( 'I','P','R','D' ); // 'IPRD' Product
STATIC_CONST DWORD RIFF_IPRO_ATOM = MAKE_ATOM( 'I','P','R','O' ); // 'IPRO' ProducedBy
STATIC_CONST DWORD RIFF_IRIP_ATOM = MAKE_ATOM( 'I','R','I','P' ); // 'IRIP' RippedBy
STATIC_CONST DWORD RIFF_IRTD_ATOM = MAKE_ATOM( 'I','R','T','D' ); // 'IRTD' Rating
STATIC_CONST DWORD RIFF_ISBJ_ATOM = MAKE_ATOM( 'I','S','B','J' ); // 'ISBJ' Subject
STATIC_CONST DWORD RIFF_ISFT_ATOM = MAKE_ATOM( 'I','S','F','T' ); // 'ISFT' Software
STATIC_CONST DWORD RIFF_ISGN_ATOM = MAKE_ATOM( 'I','S','G','N' ); // 'ISGN' SecondaryGenre
STATIC_CONST DWORD RIFF_ISHP_ATOM = MAKE_ATOM( 'I','S','H','P' ); // 'ISHP' Sharpness
STATIC_CONST DWORD RIFF_ISMP_ATOM = MAKE_ATOM( 'I','S','M','P' ); // 'ISMP' TimeCode
STATIC_CONST DWORD RIFF_ISRC_ATOM = MAKE_ATOM( 'I','S','R','C' ); // 'ISRC' Source
STATIC_CONST DWORD RIFF_ISRF_ATOM = MAKE_ATOM( 'I','S','R','F' ); // 'ISRF' SourceForm
STATIC_CONST DWORD RIFF_ISTD_ATOM = MAKE_ATOM( 'I','S','T','D' ); // 'ISTD' ProductionStudio
STATIC_CONST DWORD RIFF_ISTR_ATOM = MAKE_ATOM( 'I','S','T','R' ); // 'ISTR' Starring
STATIC_CONST DWORD RIFF_ITCH_ATOM = MAKE_ATOM( 'I','T','C','H' ); // 'ITCH' Technician
STATIC_CONST DWORD RIFF_IWMU_ATOM = MAKE_ATOM( 'I','W','M','U' ); // 'IWMU' WatermarkURL
STATIC_CONST DWORD RIFF_IWRI_ATOM = MAKE_ATOM( 'I','W','R','I' ); // 'IWRI' WrittenBy
STATIC_CONST DWORD RIFF_LANG_ATOM = MAKE_ATOM( 'L','A','N','G' ); // 'LANG' Language
STATIC_CONST DWORD RIFF_LOCA_ATOM = MAKE_ATOM( 'L','O','C','A' ); // 'LOCA' Location
STATIC_CONST DWORD RIFF_PRT1_ATOM = MAKE_ATOM( 'P','R','T','1' ); // 'PRT1' Part
STATIC_CONST DWORD RIFF_PRT2_ATOM = MAKE_ATOM( 'P','R','T','2' ); // 'PRT2' NumberOfParts
STATIC_CONST DWORD RIFF_RATE_ATOM = MAKE_ATOM( 'R','A','T','E' ); // 'RATE' Rate
STATIC_CONST DWORD RIFF_STAR_ATOM = MAKE_ATOM( 'S','T','A','R' ); // 'STAR' Starring
STATIC_CONST DWORD RIFF_STAT_ATOM = MAKE_ATOM( 'S','T','A','T' ); // 'STAT' Statistics
STATIC_CONST DWORD RIFF_TAPE_ATOM = MAKE_ATOM( 'T','A','P','E' ); // 'TAPE' TapeName
STATIC_CONST DWORD RIFF_TCDO_ATOM = MAKE_ATOM( 'T','C','D','O' ); // 'TCDO' EndTimecode
STATIC_CONST DWORD RIFF_TCOD_ATOM = MAKE_ATOM( 'T','C','O','D' ); // 'TCOD' StartTimecode
STATIC_CONST DWORD RIFF_TITL_ATOM = MAKE_ATOM( 'T','I','T','L' ); // 'TITL' Title
STATIC_CONST DWORD RIFF_TLEN_ATOM = MAKE_ATOM( 'T','L','E','N' ); // 'TLEN' Length
STATIC_CONST DWORD RIFF_TORG_ATOM = MAKE_ATOM( 'T','O','R','G' ); // 'TORG' Organization
STATIC_CONST DWORD RIFF_TRCK_ATOM = MAKE_ATOM( 'T','R','C','K' ); // 'TRCK' TrackNumber
STATIC_CONST DWORD RIFF_TURL_ATOM = MAKE_ATOM( 'T','U','R','L' ); // 'TURL' URL
STATIC_CONST DWORD RIFF_TVER_ATOM = MAKE_ATOM( 'T','V','E','R' ); // 'TVER' Version
STATIC_CONST DWORD RIFF_VMAJ_ATOM = MAKE_ATOM( 'V','M','A','J' ); // 'VMAJ' VegasVersionMajor
STATIC_CONST DWORD RIFF_VMIN_ATOM = MAKE_ATOM( 'V','M','I','N' ); // 'VMIN' VegasVersionMinor
STATIC_CONST DWORD RIFF_YEAR_ATOM = MAKE_ATOM( 'Y','E','A','R' ); // 'YEAR' Year

// Artist (IART)
// Title (INAM) - called "Track Title" in Metadata Editor
// Product (IPRD) - called "Album Title" in Metadata Editor
// Track Number (ITRK) (not specified in the original RIFF standard but players supporting LIST INFO tags often support it)
// Date Created (ICRD) - called "Year" in Metadata Editor
// Genre (IGNR)
// Comments (ICMT)
// Copyright (ICOP)
// Software (ISFT)
STATIC_CONST DWORD RIFF_TAG_ATOMS[] = {
		RIFF_IART_ATOM,
		RIFF_IPRD_ATOM,
		RIFF_INAM_ATOM,
		RIFF_ITRK_ATOM,
		RIFF_ICRD_ATOM,
		RIFF_IGNR_ATOM,
		RIFF_ICMT_ATOM,
		//RIFF_DISP_ATOM, <-- DISP is NOT (!) part of LIST container, so size entry in LIST does NOT include DISP !!!
		0
};

} /* namespace music */

#endif /* PCMTYPES_H_ */
