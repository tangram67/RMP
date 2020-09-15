/*
 * audiotypes.h
 *
 *  Created on: 14.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef AUDIOTYPES_H_
#define AUDIOTYPES_H_

#include <vector>
#include <unordered_map>
#include "../config.h"
#include "gcc.h"
#include "memory.h"
#include "nullptr.h"
#include "datetime.h"

#define USE_MEMORY_MAPPED_SAMPLE_BUFFER
#define PRIME_ALBUM_ARTIST

namespace music {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TSample = uint8_t;
using PSample = TSample*;
using TSampleBuffer = util::TDataBuffer<TSample>;

# ifdef USE_MEMORY_MAPPED_SAMPLE_BUFFER
using TSampleBitBuffer = util::TMappedBuffer<TSample>;
# else
using TSampleBitBuffer = TSampleBuffer;
# endif

using snd_pcm_int_t = int;
using snd_pcm_uint_t = unsigned int;

#else

typedef uint8_t TSample;
typedef TSample* PSample;

# ifdef USE_MEMORY_MAPPED_SAMPLE_BUFFER
typedef util::TMappedBuffer<TSample> TSampleBitBuffer;
# else
typedef TSampleBuffer TSampleBitBuffer;
# endif

typedef int snd_pcm_int_t;
typedef unsigned int snd_pcm_uint_t;

#endif


STATIC_CONST size_t AUDIO_SAMPLE_SIZE = sizeof(TSample);
STATIC_CONST size_t AUDIO_SAMPLE_BIT_SIZE = 8 * AUDIO_SAMPLE_SIZE;
STATIC_CONST TSample AUDIO_SAMPLE_MASK = TSample(-1);

STATIC_CONST char CHAR_NUMERICAL_ARTIST = '1';
STATIC_CONST char CHAR_VARIOUS_ARTIST = '2';

class TTrack;
class TSong;
struct CErrorSong;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PTrack = TTrack*;
using TTrackMap = std::map<std::string, PTrack>;
using TTrackList = std::vector<PTrack>;
using TErrorList = std::vector<CErrorSong>;

using PSong = TSong*;
using TSongList = std::vector<PSong>;
using PSongList = TSongList*;
using TSongMap = std::unordered_map<std::string, PSong>;
using TSongItem = std::pair<std::string, PSong>;
using TSongInsert = std::pair<TSongMap::iterator, bool>;
using TSongIterator = TSongMap::iterator;
using TSongConstIterator = TSongMap::const_iterator;
using TSongSorter = std::function<bool(PSong, PSong)>;

#else

typedef TTrack* PTrack;
typedef std::map<std::string, PTrack> TTrackMap;
typedef std::vector<PTrack> TTrackList;
typedef std::vector<CErrorSong> TErrorList;

typedef TSong* PSong;
typedef std::vector<PSong> TSongList;
typedef TSongList* PSongList;
typedef std::unordered_map<std::string, PSong> TSongMap;
typedef std::pair<std::string, PSong> TSongItem;
typedef std::pair<TSongMap::iterator,bool> TSongInsert;
typedef TSongMap::iterator TSongIterator;
typedef TSongMap::const_iterator TSongConstIterator;
typedef std::function<bool(PSong, PSong)> TSongSorter;

#endif

typedef struct CErrorSong {
	std::string file;
	std::string text;
	std::string hint;
	std::string url;
	int error;
} TErrorSong;

typedef struct CCurrentSong {
	PSong song;
	std::string playlist;

	void prime() {
		song = nil;
	}
	void clear() {
		prime();
		playlist.clear();
	}

	CCurrentSong& operator= (const CCurrentSong& value) {
		song = value.song;
		playlist = value.playlist;
		return *this;
	}

	CCurrentSong() {
		prime();
	}
} TCurrentSong;

typedef struct CCurrentSongs {
	PSong last;
	PSong current;
	PSong next;
	std::string playlist;

	void prime() {
		last = nil;
		current = nil;
		next = nil;
	}
	void clear() {
		prime();
		playlist.clear();
	}

	CCurrentSongs& operator= (const CCurrentSongs& value) {
		last = value.last;
		current = value.current;
		next = value.next;
		playlist = value.playlist;
		return *this;
	}

	CCurrentSongs() {
		prime();
	}
} TCurrentSongs;

typedef struct CTracks {
	TSongList songs;
	TSongMap files;
} TTracks;

typedef struct CAlbum {
	std::string name;
	std::string artist;
	std::string originalartist;
	std::string genre;

	std::string displayname;
	std::string displayartist;
	std::string displayoriginalartist;
	std::string displaygenre;

	std::string hash;
	std::string url;

	bool compilation;
	util::TTimePart inserted;
	util::TTimePart date;
	TSongList songs;

	CAlbum& operator= (const CAlbum& value) {
		songs.clear();
		name = value.name;
		artist = value.artist;
		originalartist = value.originalartist;
		genre = value.genre;
		displayname = value.displayname;
		displayartist = value.displayartist;
		displayoriginalartist = value.displayoriginalartist;
		displaygenre = value.displaygenre;
		hash = value.hash;
		url = value.url;
		compilation = value.compilation;
		inserted = value.inserted;
		date = value.date;
		for (size_t i=0; i<value.songs.size(); ++i)
			songs.push_back(value.songs[i]);
		return *this;
	}

	CAlbum() {
		compilation = false;
		inserted = util::epoch();
		date = util::epoch();
	}
} TAlbum;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PAlbum = TAlbum*;
using TAlbumList = std::vector<PAlbum>;
using TConstAlbumList = std::vector<const TAlbum*>;

using TAlbumMap = std::map<std::string, TAlbum>;
using TAlbumItem = std::pair<std::string, TAlbum>;
using TAlbumIterator = TAlbumMap::iterator;
using TAlbumConstIterator = TAlbumMap::const_iterator;

using THashedMap = std::unordered_map<std::string, TAlbum>;
using THashedItem = std::pair<std::string, TAlbum>;
using THashedIterator = THashedMap::iterator;
using THashedConstIterator = THashedMap::const_iterator;

#else

typedef TAlbum* PAlbum;
typedef std::vector<PAlbum> TAlbumList;
typedef std::vector<const TAlbum*> TConstAlbumList;

typedef std::map<std::string, TAlbum> TAlbumMap;
typedef std::pair<std::string, TAlbum> TAlbumItem;
typedef TAlbumMap::iterator TAlbumIterator;
typedef TAlbumMap::const_iterator TAlbumConstIterator;

typedef std::map<std::string, TAlbum> THashedMap;
typedef std::pair<std::string, TAlbum> THashedItem;
typedef THashedMap::iterator THashedIterator;
typedef THashedMap::const_iterator THashedConstIterator;

#endif

typedef struct CArtist {
	std::string name;
	std::string originalname;
	std::string displayname;
	std::string displayoriginalname;
	std::string hash;
	std::string url;
	TAlbumMap albums;
} TArtist;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TArtistMap = std::map<std::string, TArtist>;
using TArtistItem = std::pair<std::string, TArtist>;
using TArtistIterator = TArtistMap::iterator;
using TArtistConstIterator = TArtistMap::const_iterator;

using TLetterMap = std::map<char, size_t>;
using TLetterItem = std::pair<char, size_t>;
using TLetterIterator = TLetterMap::iterator;
using TLetterConstIterator = TLetterMap::const_iterator;

using TConstArtistList = std::vector<const music::TArtist*>;

#else

typedef std::map<std::string, TArtist> TArtistMap;
typedef std::pair<std::string, TArtist> TArtistItem;
typedef TArtistMap::iterator TArtistIterator;
typedef TArtistMap::const_iterator TArtistConstIterator;

typedef std::map<char, int> TLetterMap;
typedef std::pair<char, int> TLetterItem;
typedef TLetterMap::iterator TLetterIterator;
typedef TLetterMap::const_iterator TLetterConstIterator;

using std::vector<const music::TArtist*> TConstArtistList;

#endif


typedef struct CArtists {
	TArtistMap all;
	TArtistMap cd;
	TArtistMap hdcd;
	TArtistMap dsd;
	TArtistMap dvd;
	TArtistMap bd;
	TArtistMap hr;
} TArtists;

typedef struct CAplhabet {
	TLetterMap all;
	TLetterMap cd;
	TLetterMap hdcd;
	TLetterMap dsd;
	TLetterMap dvd;
	TLetterMap bd;
	TLetterMap hr;
} TAplhabet;

typedef struct CSongs {
	TTracks tracks;
	TArtists artists;
	TAplhabet letters;
	THashedMap albums;
	TAlbumMap ordered;
	TAlbumList recent;
} TSongs;


enum ECodecType {
	EFT_UNKNOWN = 0,
	EFT_FLAC = 10,
	EFT_OGG = 11,
	EFT_DSF = 20,
	EFT_DFF = 21,
	EFT_WAV = 30,
	EFT_AIFF = 31,
	EFT_ALAC = 40,
	EFT_AAC = 41,
	EFT_MP3 = 99
};

enum EMediaType {
	EMT_UNKNOWN,
	EMT_CD,
	EMT_HDCD,
	EMT_DSD,
	EMT_DVD,
	EMT_BD,
	EMT_HR,
	EMT_ALL
};

enum EPcmType {
	EP_PCM_RAW,
	EP_PCM_WAVE
};

enum ESampleSize {
	ES_DSD_NE = 1, // Newest endian
	ES_DSD_OE = 2, // Oldest endian
	ES_CD = 16,
	ES_HR = 24
};

enum ESampleRate {
	SR0K = 0,
	SR44K = 44100,
	SR48K = 48000,
	SR88K = 88200,
	SR96K = 96000,
	SR176K = 176400,
	SR192K = 192000,
	SR352K = 352800,
	SR384K = 384000
};

enum EFilterType {
	FT_STRING,
	FT_ARTIST,
	FT_ALBUM,
	FT_DEFAULT = FT_STRING
};

enum EFilterDomain {
	FD_UNKNOWN,
	FD_ARTIST,
	FD_ALBUMARTIST,
	FD_COMPOSER,
	FD_CONDUCTOR,
	FD_ALBUM,
	FD_TITLE,
	FD_ALL,
	FD_DEFAULT = FD_ALL
};

enum EArtistFilter {
	AF_FILTER_FULL,
	AF_FILTER_PARTIAL
};

enum EPlayListAction {
	EPA_INSERT,
	EPA_APPEND,
	EPA_DEFAULT = EPA_APPEND
};

enum EContainerType {
	ECT_NATIVE,
	ECT_FLAC,
	ECT_OGG,
	ECT_DEFAULT = ECT_NATIVE
};

struct TCodecType {
	ECodecType type;
	const char* extension;
	const char* shortcut;
	const char* description;
};

static const struct TCodecType codecs[] =
{
	{ ECodecType::EFT_FLAC,   "flac", "FLAC",     "Free Lossless Audio Codec" },
	{ ECodecType::EFT_DSF,    "dsf",  "DSD/DSF",  "Sony DSD File" },
	{ ECodecType::EFT_DFF,    "dff",  "DSD/DFF",  "Philips DSD File" },
	{ ECodecType::EFT_WAV,    "wav",  "PCM/WAVE", "Microsoft/IBM RIFF/Wave Audio" },
	{ ECodecType::EFT_MP3,    "mp3",  "MP3",      "Frauenhofer MPEG3 Audio Codec" },
	{ ECodecType::EFT_AIFF,   "aif",  "AIFF",     "Apple Audio Interchange File Format" },
	{ ECodecType::EFT_AIFF,   "aiff", "AIFF",     "Apple Audio Interchange File Format" },
	{ ECodecType::EFT_ALAC,   "m4a",  "MP4/ALAC", "Apple Lossless Audio Codec" },
	{ ECodecType::EFT_ALAC,   "mp4",  "MP4/ALAC", "Apple Lossless Audio Codec" },
	{ ECodecType::EFT_AAC,    "aac",  "MP4/AAC",  "MPEG Advanced Audio Codec" },
	{ ECodecType::EFT_UNKNOWN, nil, nil, nil }
};

struct TMediaType {
	EMediaType media;
	const char* name;
	const char* shortcut;
	const char* description;
};

static const struct TMediaType formats[] =
{
	{ EMediaType::EMT_CD,     "cd-audio",   "CD",      "Compact Disk Audio" },
	{ EMediaType::EMT_HDCD,   "hdcd-audio", "HDCD",    "High Definition Compatible Digital" },
	{ EMediaType::EMT_DSD,    "dsd-audio",  "SACD",    "Direct Stream Digital Audio" },
	{ EMediaType::EMT_DVD,    "dvd-audio",  "DVD",     "Digital Versatile Disk Audio" },
	{ EMediaType::EMT_BD,     "bd-audio",   "Blu-ray", "Blu-ray Audio" },
	{ EMediaType::EMT_HR,     "hr-audio",   "Hi-Res",  "High Resolution Audio" },
	{ EMediaType::EMT_UNKNOWN, nil, nil, nil }
};

enum EScannerType {
	EST_UNKNOWN = 0,
	EST_GROUP_NAME_SWAP        = (1 << 0),
	EST_ARTIST_NAME_RESTORE    = (1 << 1),
	EST_FULL_NAME_SWAP         = (1 << 2),
	EST_THE_BAND_PREFIX_SWAP   = (1 << 3),
	EST_DEEP_NAME_INSPECTION   = (1 << 4),
	EST_VARIOUS_ARTISTS_RENAME = (1 << 5),
	EST_MOVE_PREAMBLE          = (1 << 6),
	EST_URL_ENCODED            = (1 << 9)
};

typedef struct CDecoderParams {
	std::string decoder;
	bool paranoid;
	bool gapless;
	bool debug;
	bool valid;
	size_t metaint;
	size_t sampleRate;
	size_t bitRate;
	size_t channels;
	size_t bytesPerSample;
	size_t bitsPerSample;
    size_t sampleCount;
    size_t sampleSize;
    util::TTimePart duration; // Milliseconds
    util::TTimePart seconds;  // Seconds
	EContainerType container;

	void clear() {
		decoder.clear();
		paranoid = false;
		gapless = false;
		debug = false;
		valid = false;
		metaint = 0;
		bitRate = 0;
		sampleRate = 0;
		channels = 0;
		bytesPerSample = 0;
		bitsPerSample = 0;
	    sampleCount = 0;
	    sampleSize = 0;
		duration = 0;
		seconds = 0;
		container = ECT_DEFAULT;
	}

	bool isValid() const {
	    return bitsPerSample > 8 &&
	    	bytesPerSample > 2 &&
			channels > 0 &&
			sampleRate > 0 &&
			sampleCount > 0 &&
			sampleSize > 0 &&
			duration > 0 &&
			seconds > 0;
	}

	CDecoderParams& operator = (const CDecoderParams& value) {
		valid = value.valid;
		decoder = value.decoder;
		paranoid = value.paranoid;
		gapless = value.gapless;
		debug = value.debug;
		metaint = value.metaint;
		bitRate = value.bitRate;
		sampleRate = value.sampleRate;
		channels = value.channels;
		bytesPerSample = value.bytesPerSample;
		bitsPerSample = value.bitsPerSample;
		sampleCount = value.sampleCount;
		sampleSize = value.sampleSize;
		duration = value.duration;
		seconds = value.seconds;
		container = value.container;
		return *this;
	}

	CDecoderParams() {
		clear();
	}
} TDecoderParams;

} /* namespace music */

#endif /* AUDIOTYPES_H_ */
