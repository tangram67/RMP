/*
 * id3types.h
 *
 *  Created on: 18.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ID3TYPES_H_
#define ID3TYPES_H_

#include "chunktypes.h"
#include "windows.h"
#include "memory.h"
#include "gcc.h"

namespace music {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TID3Tag = CHAR[3];

using TID3Buffer = util::TDataBuffer<char>;

#else

typedef CHAR TID3Tag[3];

typedef util::TDataBuffer<char> TID3Buffer;

#endif


extern "C" int calc_hash_id(const char* ID, size_t size);


// ID3V1.1 header 
// Length = 128 Byte fixed at end of file
// Version 1.1 adds track no. as last byte of comment
//	Length	Position	Description
//	(bytes)	(bytes)		-----------
//	3		(0-2) 		Tag identification. Must contain 'TAG' if tag exists and is correct.
//	30		(3-32)		Title
//	30		(33-62)		Artist
//	30		(63-92)		Album
//	4		(93-96)		Year
//	30		(97-126)	Comment
//	1		(127)		Genre
typedef struct CID3v1Header {
	TID3Tag		id3Tag; 	// Offset 0  "TAG"
	CHAR		idTitle[30];
	CHAR		idArtist[30];
	CHAR		idAlbum[30];
	CHAR		idYear[4];
	CHAR		idComment[29];
	BYTE		idTrack;
	BYTE		idGenre;
} PACKED TID3v1Header;


typedef struct CID3v2Header {
	TID3Tag		id3Tag; 	// Offset 0  "ID3"
	BYTE		idVersion;	// Offset 3
	BYTE		idRevision;	// Offset 4
	BYTE		idFlags;	// Offset 5
	TInt28		idTagSize;	// Offset 6
} PACKED TID3v2Header;


STATIC_CONST char ID3FID_AUDIOCRYPTO[]			= "AENC"; // Audio encryption
STATIC_CONST char ID3FID_PICTURE[]				= "APIC"; // Attached picture
STATIC_CONST char ID3FID_COMMENT[]				= "COMM"; // Comments
STATIC_CONST char ID3FID_COMILATION[]			= "TCMP"; // Part of a compilation
STATIC_CONST char ID3FID_COMMERCIAL[]			= "COMR"; // Commercial
STATIC_CONST char ID3FID_CRYPTOREG[]			= "ENCR"; // Encryption method registration
STATIC_CONST char ID3FID_EQUALIZATION[]			= "EQUA"; // Equalization
STATIC_CONST char ID3FID_EVENTTIMING[]			= "ETCO"; // Event timing codes
STATIC_CONST char ID3FID_GENERALOBJECT[]		= "GEOB"; // General encapsulated object
STATIC_CONST char ID3FID_GROUPINGREG[]			= "GRID"; // Group identification registration
STATIC_CONST char ID3FID_INVOLVEDPEOPLE[]		= "IPLS"; // Involved people list
STATIC_CONST char ID3FID_LINKEDINFO[]			= "LINK"; // Linked information
STATIC_CONST char ID3FID_CDID[]					= "MCDI"; // Music CD identifier
STATIC_CONST char ID3FID_MPEGLOOKUP[]			= "MLLT"; // MPEG location lookup table
STATIC_CONST char ID3FID_OWNERSHIP[]			= "OWNE"; // Ownership frame
STATIC_CONST char ID3FID_PRIVATE[]				= "PRIV"; // Private frame
STATIC_CONST char ID3FID_PLAYCOUNTER[]			= "PCNT"; // Play counter
STATIC_CONST char ID3FID_POPULARIMETER[]		= "POPM"; // Popularimeter
STATIC_CONST char ID3FID_POSITIONSYNC[]			= "POSS"; // Position synchronisation frame
STATIC_CONST char ID3FID_BUFFERSIZE[]			= "RBUF"; // Recommended buffer size
STATIC_CONST char ID3FID_VOLUMEADJ[]			= "RVAD"; // Relative volume adjustment
STATIC_CONST char ID3FID_REVERB[]				= "RVRB"; // Reverb
STATIC_CONST char ID3FID_SYNCEDLYRICS[]			= "SYLT"; // Synchronized lyric/text
STATIC_CONST char ID3FID_SYNCEDTEMPO[]			= "SYTC"; // Synchronized tempo codes
STATIC_CONST char ID3FID_ALBUM[]				= "TALB"; // Album/Movie/Show title
STATIC_CONST char ID3FID_BPM[]					= "TBPM"; // BPM (beats per minute)
STATIC_CONST char ID3FID_COMPOSER[]				= "TCOM"; // Composer
STATIC_CONST char ID3FID_CONTENTTYPE[]			= "TCON"; // Content type/genre
STATIC_CONST char ID3FID_COPYRIGHT[]			= "TCOP"; // Copyright message
STATIC_CONST char ID3FID_DATE[]					= "TDAT"; // Date
STATIC_CONST char ID3FID_PLAYLISTDELAY[]		= "TDLY"; // Playlist delay
STATIC_CONST char ID3FID_ENCODEDBY[]			= "TENC"; // Encoded by
STATIC_CONST char ID3FID_LYRICIST[]				= "TEXT"; // Lyricist/Text writer
STATIC_CONST char ID3FID_FILETYPE[]				= "TFLT"; // File type
STATIC_CONST char ID3FID_TIME[]					= "TIME"; // Time
STATIC_CONST char ID3FID_CONTENTGROUP[]			= "TIT1"; // Content group description
STATIC_CONST char ID3FID_TITLE[]				= "TIT2"; // Title/songname/content description
STATIC_CONST char ID3FID_SUBTITLE[]				= "TIT3"; // Subtitle/Description refinement
STATIC_CONST char ID3FID_INITIALKEY[]			= "TKEY"; // Initial key
STATIC_CONST char ID3FID_LANGUAGE[]				= "TLAN"; // Language(s)
STATIC_CONST char ID3FID_SONGLEN[]				= "TLEN"; // Length
STATIC_CONST char ID3FID_MEDIATYPE[]			= "TMED"; // Media type
STATIC_CONST char ID3FID_ORIGALBUM[]			= "TOAL"; // Original album/movie/show title
STATIC_CONST char ID3FID_ORIGFILENAME[]			= "TOFN"; // Original filename
STATIC_CONST char ID3FID_ORIGLYRICIST[]			= "TOLY"; // Original lyricist(s)/text writer(s)
STATIC_CONST char ID3FID_ORIGARTIST[]			= "TOPE"; // Original artist(s)/performer(s)
STATIC_CONST char ID3FID_ORIGYEAR[]				= "TORY"; // Original release year
STATIC_CONST char ID3FID_FILEOWNER[]			= "TOWN"; // File owner/licensee
STATIC_CONST char ID3FID_LEADARTIST[]			= "TPE1"; // Lead performer(s)/Soloist(s)
STATIC_CONST char ID3FID_BAND[]					= "TPE2"; // Band/orchestra/accompaniment/albumartist
STATIC_CONST char ID3FID_CONDUCTOR[]			= "TPE3"; // Conductor/performer refinement
STATIC_CONST char ID3FID_MIXARTIST[]			= "TPE4"; // Interpreted, remixed, or otherwise modified by
STATIC_CONST char ID3FID_PARTOFSET[]			= "TPOS"; // Part of a set
STATIC_CONST char ID3FID_PUBLISHER[]			= "TPUB"; // Publisher
STATIC_CONST char ID3FID_TRACKNUM[]				= "TRCK"; // Track number/Position in set
STATIC_CONST char ID3FID_RECORDINGDATES[]		= "TRDA"; // Recording dates
STATIC_CONST char ID3FID_NETRADIOSTATION[]		= "TRSN"; // Internet radio station name
STATIC_CONST char ID3FID_NETRADIOOWNER[]		= "TRSO"; // Internet radio station owner
STATIC_CONST char ID3FID_SIZE[]					= "TSIZ"; // Size
STATIC_CONST char ID3FID_ISRC[]					= "TSRC"; // ISRC (international standard recording code)
STATIC_CONST char ID3FID_ENCODERSETTINGS[]		= "TSSE"; // Software/Hardware and settings used for encoding
STATIC_CONST char ID3FID_USERTEXT[]				= "TXXX"; // User defined text information
STATIC_CONST char ID3FID_YEAR[]					= "TYER"; // Year
STATIC_CONST char ID3FID_ALT_YEAR[]				= "TDRC"; // Year as used by dBpoweramp Release 14.1
STATIC_CONST char ID3FID_UNIQUEFILEID[]			= "UFID"; // Unique file identifier
STATIC_CONST char ID3FID_TERMSOFUSE[]			= "USER"; // Terms of use
STATIC_CONST char ID3FID_UNSYNCEDLYRICS[]		= "USLT"; // Unsynchronized lyric/text transcription
STATIC_CONST char ID3FID_WWWCOMMERCIALINFO[]	= "WCOM"; // Commercial information
STATIC_CONST char ID3FID_WWWCOPYRIGHT[]			= "WCOP"; // Copyright/Legal infromation
STATIC_CONST char ID3FID_WWWAUDIOFILE[]			= "WOAF"; // Official audio file webpage
STATIC_CONST char ID3FID_WWWARTIST[]			= "WOAR"; // Official artist/performer webpage
STATIC_CONST char ID3FID_WWWAUDIOSOURCE[]		= "WOAS"; // Official audio source webpage
STATIC_CONST char ID3FID_WWWRADIOPAGE[]			= "WORS"; // Official internet radio station homepage
STATIC_CONST char ID3FID_WWWPAYMENT[]			= "WPAY"; // Payment
STATIC_CONST char ID3FID_WWWPUBLISHER[]			= "WPUB"; // Official publisher webpage
STATIC_CONST char ID3FID_WWWUSER[]				= "WXXX"; // User defined URL link

static const int ID3HID_AUDIOCRYPTO UNUSED		= calc_hash_id(ID3FID_AUDIOCRYPTO, 0);
static const int ID3HID_PICTURE UNUSED			= calc_hash_id(ID3FID_PICTURE, 0);
static const int ID3HID_COMMENT UNUSED			= calc_hash_id(ID3FID_COMMENT, 0);
static const int ID3HID_COMPILATION UNUSED		= calc_hash_id(ID3FID_COMILATION, 0);
static const int ID3HID_COMMERCIAL UNUSED		= calc_hash_id(ID3FID_COMMERCIAL, 0);
static const int ID3HID_CRYPTOREG UNUSED		= calc_hash_id(ID3FID_CRYPTOREG, 0);
static const int ID3HID_EQUALIZATION UNUSED		= calc_hash_id(ID3FID_EQUALIZATION, 0);
static const int ID3HID_EVENTTIMING UNUSED		= calc_hash_id(ID3FID_EVENTTIMING, 0);
static const int ID3HID_GENERALOBJECT UNUSED	= calc_hash_id(ID3FID_GENERALOBJECT, 0);
static const int ID3HID_GROUPINGREG UNUSED		= calc_hash_id(ID3FID_GROUPINGREG, 0);
static const int ID3HID_INVOLVEDPEOPLE UNUSED	= calc_hash_id(ID3FID_INVOLVEDPEOPLE, 0);
static const int ID3HID_LINKEDINFO UNUSED		= calc_hash_id(ID3FID_LINKEDINFO, 0);
static const int ID3HID_CDID UNUSED				= calc_hash_id(ID3FID_CDID, 0);
static const int ID3HID_MPEGLOOKUP UNUSED		= calc_hash_id(ID3FID_MPEGLOOKUP, 0);
static const int ID3HID_OWNERSHIP UNUSED		= calc_hash_id(ID3FID_OWNERSHIP, 0);
static const int ID3HID_PRIVATE UNUSED			= calc_hash_id(ID3FID_PRIVATE, 0);
static const int ID3HID_PLAYCOUNTER UNUSED		= calc_hash_id(ID3FID_PLAYCOUNTER, 0);
static const int ID3HID_POPULARIMETER UNUSED	= calc_hash_id(ID3FID_POPULARIMETER, 0);
static const int ID3HID_POSITIONSYNC UNUSED		= calc_hash_id(ID3FID_POSITIONSYNC, 0);
static const int ID3HID_BUFFERSIZE UNUSED		= calc_hash_id(ID3FID_BUFFERSIZE, 0);
static const int ID3HID_VOLUMEADJ UNUSED		= calc_hash_id(ID3FID_VOLUMEADJ, 0);
static const int ID3HID_REVERB UNUSED			= calc_hash_id(ID3FID_REVERB, 0);
static const int ID3HID_SYNCEDLYRICS UNUSED		= calc_hash_id(ID3FID_SYNCEDLYRICS, 0);
static const int ID3HID_SYNCEDTEMPO UNUSED		= calc_hash_id(ID3FID_SYNCEDTEMPO, 0);
static const int ID3HID_ALBUM UNUSED			= calc_hash_id(ID3FID_ALBUM, 0);
static const int ID3HID_BPM UNUSED				= calc_hash_id(ID3FID_BPM, 0);
static const int ID3HID_COMPOSER UNUSED			= calc_hash_id(ID3FID_COMPOSER, 0);
static const int ID3HID_CONTENTTYPE UNUSED		= calc_hash_id(ID3FID_CONTENTTYPE, 0);
static const int ID3HID_COPYRIGHT UNUSED		= calc_hash_id(ID3FID_COPYRIGHT, 0);
static const int ID3HID_DATE UNUSED				= calc_hash_id(ID3FID_DATE, 0);
static const int ID3HID_PLAYLISTDELAY UNUSED	= calc_hash_id(ID3FID_PLAYLISTDELAY, 0);
static const int ID3HID_ENCODEDBY UNUSED		= calc_hash_id(ID3FID_ENCODEDBY, 0);
static const int ID3HID_LYRICIST UNUSED			= calc_hash_id(ID3FID_LYRICIST, 0);
static const int ID3HID_FILETYPE UNUSED			= calc_hash_id(ID3FID_FILETYPE, 0);
static const int ID3HID_TIME UNUSED				= calc_hash_id(ID3FID_TIME, 0);
static const int ID3HID_CONTENTGROUP UNUSED		= calc_hash_id(ID3FID_CONTENTGROUP, 0);
static const int ID3HID_TITLE UNUSED			= calc_hash_id(ID3FID_TITLE, 0);
static const int ID3HID_SUBTITLE UNUSED			= calc_hash_id(ID3FID_SUBTITLE, 0);
static const int ID3HID_INITIALKEY UNUSED		= calc_hash_id(ID3FID_INITIALKEY, 0);
static const int ID3HID_LANGUAGE UNUSED			= calc_hash_id(ID3FID_LANGUAGE, 0);
static const int ID3HID_SONGLEN UNUSED			= calc_hash_id(ID3FID_SONGLEN, 0);
static const int ID3HID_MEDIATYPE UNUSED		= calc_hash_id(ID3FID_MEDIATYPE, 0);
static const int ID3HID_ORIGALBUM UNUSED		= calc_hash_id(ID3FID_ORIGALBUM, 0);
static const int ID3HID_ORIGFILENAME UNUSED		= calc_hash_id(ID3FID_ORIGFILENAME, 0);
static const int ID3HID_ORIGLYRICIST UNUSED		= calc_hash_id(ID3FID_ORIGLYRICIST, 0);
static const int ID3HID_ORIGARTIST UNUSED		= calc_hash_id(ID3FID_ORIGARTIST, 0);
static const int ID3HID_ORIGYEAR UNUSED			= calc_hash_id(ID3FID_ORIGYEAR, 0);
static const int ID3HID_FILEOWNER UNUSED		= calc_hash_id(ID3FID_FILEOWNER, 0);
static const int ID3HID_LEADARTIST UNUSED		= calc_hash_id(ID3FID_LEADARTIST, 0);
static const int ID3HID_BAND UNUSED				= calc_hash_id(ID3FID_BAND, 0);
static const int ID3HID_CONDUCTOR UNUSED		= calc_hash_id(ID3FID_CONDUCTOR, 0);
static const int ID3HID_MIXARTIST UNUSED		= calc_hash_id(ID3FID_MIXARTIST, 0);
static const int ID3HID_PARTOFSET UNUSED		= calc_hash_id(ID3FID_PARTOFSET, 0);
static const int ID3HID_PUBLISHER UNUSED		= calc_hash_id(ID3FID_PUBLISHER, 0);
static const int ID3HID_TRACKNUM UNUSED			= calc_hash_id(ID3FID_TRACKNUM, 0);
static const int ID3HID_RECORDINGDATES UNUSED	= calc_hash_id(ID3FID_RECORDINGDATES, 0);
static const int ID3HID_NETRADIOSTATION UNUSED	= calc_hash_id(ID3FID_NETRADIOSTATION, 0);
static const int ID3HID_NETRADIOOWNER UNUSED	= calc_hash_id(ID3FID_NETRADIOOWNER, 0);
static const int ID3HID_SIZE UNUSED				= calc_hash_id(ID3FID_SIZE, 0);
static const int ID3HID_ISRC UNUSED				= calc_hash_id(ID3FID_ISRC, 0);
static const int ID3HID_ENCODERSETTINGS UNUSED	= calc_hash_id(ID3FID_ENCODERSETTINGS, 0);
static const int ID3HID_USERTEXT UNUSED			= calc_hash_id(ID3FID_USERTEXT, 0);
static const int ID3HID_YEAR UNUSED				= calc_hash_id(ID3FID_YEAR, 0);
static const int ID3HID_ALT_YEAR UNUSED			= calc_hash_id(ID3FID_ALT_YEAR, 0);
static const int ID3HID_UNIQUEFILEID UNUSED		= calc_hash_id(ID3FID_UNIQUEFILEID, 0);
static const int ID3HID_TERMSOFUSE UNUSED		= calc_hash_id(ID3FID_TERMSOFUSE, 0);
static const int ID3HID_UNSYNCEDLYRICS UNUSED	= calc_hash_id(ID3FID_UNSYNCEDLYRICS, 0);
static const int ID3HID_WWWCOMMERCIALINFO UNUSED	= calc_hash_id(ID3FID_WWWCOMMERCIALINFO, 0);
static const int ID3HID_WWWCOPYRIGHT UNUSED		= calc_hash_id(ID3FID_WWWCOPYRIGHT, 0);
static const int ID3HID_WWWAUDIOFILE UNUSED		= calc_hash_id(ID3FID_WWWAUDIOFILE, 0);
static const int ID3HID_WWWARTIST UNUSED		= calc_hash_id(ID3FID_WWWARTIST, 0);
static const int ID3HID_WWWAUDIOSOURCE UNUSED	= calc_hash_id(ID3FID_WWWAUDIOSOURCE, 0);
static const int ID3HID_WWWRADIOPAGE UNUSED		= calc_hash_id(ID3FID_WWWRADIOPAGE, 0);
static const int ID3HID_WWWPAYMENT UNUSED		= calc_hash_id(ID3FID_WWWPAYMENT, 0);
static const int ID3HID_WWWPUBLISHER UNUSED		= calc_hash_id(ID3FID_WWWPUBLISHER, 0);
static const int ID3HID_WWWUSER UNUSED			= calc_hash_id(ID3FID_WWWUSER, 0);

} /* namespace music */

#endif /* ID3TYPES_H_ */
