/*
 * tagtypes.h
 *
 *  Created on: 30.09.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef TAGTYPES_H_
#define TAGTYPES_H_

#include <iostream>
#include <string>
#include "hash.h"
#include "locale.h"
#include "variant.h"
#include "convert.h"
#include "datetime.h"
#include "audiotypes.h"
#include "jpegtypes.h"


namespace music {


class TArtwork : public util::TBlob {
private:
	void loadFromFile(const std::string fileName);

public:
	TArtwork& operator = (const TArtwork& value) {
		clear();
		assign(value);
		return *this;
	}

	virtual ~TArtwork() = default;
};


struct CTagValues {
	int value;
	int count;

	void prime() {
		value = 0;
		count = 0;
	}

	CTagValues& operator = (const CTagValues& value) {
		this->value = value.value;
		this->count = value.count;
		return *this;
	}

	CTagValues() { prime(); };
	virtual ~CTagValues() = default;
};

struct CSongData {
	std::string artist;
	std::string albumartist;
	std::string album;
	std::string title;

	int track;
	int tracks;
	int disk;
	int disks;

	std::string titleHash;
	std::string albumHash;
	std::string artistHash;
	std::string albumartistHash;
	std::string fileHash;

	util::TTimePart duration;
	util::TTimePart played;
	int progress;

	size_t index;

	bool streamable;
	bool valid;

	void prime() {
		duration = 0;
		played = 0;
		progress = 0;
		track = 0;
		disk = 0;
		tracks = 0;
		disks = 0;
		index = std::string::npos;
		streamable = false;
		valid = false;
	}

	void clear() {
		artist.clear();
		albumartist.clear();
		album.clear();
		title.clear();
		titleHash.clear();
		albumHash.clear();
		artistHash.clear();
		albumartistHash.clear();
		fileHash.clear();
		prime();
	}

	CSongData& operator = (const CSongData& value) {
		artist = value.artist;
		albumartist = value.albumartist;
		album = value.album;
		title = value.title;
		track = value.track;
		tracks = value.tracks;
		disk = value.disk;
		disks = value.disks;
		titleHash = value.titleHash;
		albumHash = value.albumHash;
		artistHash = value.artistHash;
		albumartistHash = value.albumartistHash;
		fileHash = value.fileHash;
		duration = value.duration;
		played = value.played;
		progress = value.progress;
		index = value.index;
		valid = value.valid;
		streamable = value.streamable;
		return *this;
	}

	CSongData() { prime(); };
	virtual ~CSongData() = default;
};


struct CComapreMetaData {
	bool albumIsArtist;
	bool composerIsArtist;
	bool conductorIsArtist;
	bool composerIsAlbumArtist;
	bool conductorIsAlbumArtist;

	void prime() {
		albumIsArtist = false;
		composerIsArtist = false;
		conductorIsArtist = false;
		composerIsAlbumArtist = false;
		conductorIsAlbumArtist = false;
	}

	void clear() {
		prime();
	}

	CComapreMetaData& operator = (const CComapreMetaData &value) {
		albumIsArtist = value.albumIsArtist;
		composerIsArtist = value.composerIsArtist;
		conductorIsArtist = value.conductorIsArtist;
		composerIsAlbumArtist = value.composerIsAlbumArtist;
		conductorIsAlbumArtist = value.conductorIsAlbumArtist;
		return *this;
	}

	CComapreMetaData() {
		prime();
	}
};

struct CTextMetaData {
	std::string artist;
	std::string originalartist;
	std::string albumartist;
	std::string originalalbumartist;
	std::string extendedalbum;
	std::string extendedinfo;
	std::string album;
	std::string title;
	std::string track;
	std::string disk;
	std::string year;
	std::string date;
	std::string genre;
	std::string composer;
	std::string conductor;
	std::string comment;
	std::string description;

	void prime() {}

	void clear() {
		artist.clear();
		originalartist.clear();
		albumartist.clear();
		originalalbumartist.clear();
		extendedalbum.clear();
		extendedinfo.clear();
		album.clear();
		title.clear();
		track.clear();
		disk.clear();
		year.clear();
		date.clear();
		genre.clear();
		composer.clear();
		conductor.clear();
		comment.clear();
		description.clear();
	}

	CTextMetaData& operator = (const music::CTextMetaData &value) {
		artist = value.artist;
		originalartist = value.originalartist;
		albumartist = value.albumartist;
		originalalbumartist = value.originalalbumartist;
		extendedalbum = value.extendedalbum;
		extendedinfo = value.extendedinfo;
		album = value.album;
		title = value.title;
		track = value.track;
		disk = value.disk;
		year = value.year;
		date = value.date;
		genre = value.genre;
		composer = value.composer;
		conductor = value.conductor;
		comment = value.comment;
		description = value.description;
		return *this;
	}
};

struct CHashMetaData {
	std::string title;
	std::string album;
	std::string artist;
	std::string albumartist;

	void prime() {}

	void clear() {
		title.clear();
		album.clear();
		artist.clear();
		albumartist.clear();
	}

	CHashMetaData& operator = (const music::CHashMetaData &value) {
		title = value.title;
		album = value.album;
		artist = value.artist;
		albumartist = value.albumartist;
		return *this;
	}
};

struct CTrackMetaData {
	bool compilation;
	int tracknumber;
	int trackcount;
	int disknumber;
	int diskcount;

	void prime() {
		compilation = false;
		tracknumber = 0;
		trackcount = 0;
		disknumber = 0;
		diskcount = 0;
	}

	void clear() {
		prime();
	}

	CTrackMetaData& operator = (const music::CTrackMetaData &value) {
		compilation = value.compilation;
		tracknumber = value.tracknumber;
		trackcount = value.trackcount;
		disknumber = value.disknumber;
		diskcount = value.diskcount;
		return *this;
	}

	CTrackMetaData() {
		prime();
	}
};

struct CMetaData {
	CTextMetaData display;
	CTextMetaData text;
	CHashMetaData hash;
	CTrackMetaData track;
	CComapreMetaData compare;

	void prime() {}

	void clear() {
		display.clear();
		text.clear();
		hash.clear();
		track.clear();
		compare.clear();
	}

	bool isValid() const {
		return !text.artist.empty() &&
			!text.album.empty() &&
			!text.title.empty() &&
			!text.genre.empty() &&
			!text.track.empty() &&
			!text.year.empty() &&
			track.tracknumber > 0;
	}

	void debugOutput(const std::string& preamble = "") const {
		std::cout << preamble << "  Artist (*)  : \"" << text.artist << "\"" << std::endl;
		std::cout << preamble << "  Albumartist : \"" << text.albumartist << "\"" << std::endl;
		std::cout << preamble << "  Album (*)   : \"" << text.album << "\"" << std::endl;
		std::cout << preamble << "  Title (*)   : \"" << text.title << "\"" << std::endl;
		std::cout << preamble << "  Genre (*)   : \"" << text.genre << "\"" << std::endl;
		std::cout << preamble << "  Composer    : \"" << text.composer << "\"" << std::endl;
		std::cout << preamble << "  Conductor   : \"" << text.conductor << "\"" << std::endl;
		std::cout << preamble << "  Extended    : \"" << text.extendedinfo << "\"" << std::endl;
		std::cout << preamble << "  Comment     : \"" << text.comment << "\"" << std::endl;
		std::cout << preamble << "  Description : \"" << text.description << "\"" << std::endl;
		std::cout << preamble << "  Track (*)   : \"" << text.track << "\"" << std::endl;
		if (track.trackcount > 0)
			std::cout << preamble << "  Tracks      : \"" << track.trackcount << "\"" << std::endl;
		std::cout << preamble << "  Disk        : \"" << text.disk << "\"" << std::endl;
		if (track.diskcount > 0)
			std::cout << preamble << "  Disks       : \"" << track.diskcount << "\"" << std::endl;
		std::cout << preamble << "  Year (*)    : \"" << text.year << "\"" << std::endl;
		std::cout << preamble << "  Date        : \"" << text.date << "\"" << std::endl;
		std::cout << preamble << "  Artist hash : " << (hash.artist.empty() ? "-" : hash.artist) << std::endl;
		std::cout << preamble << "  Album hash  : " << (hash.album.empty() ? "-" : hash.album) << std::endl;
		std::cout << preamble << "  Title hash  : " << (hash.title.empty() ? "-" : hash.title) << std::endl;
		std::cout << preamble << "  Valid       : " << isValid() << std::endl;
	}

	CMetaData& operator = (const music::CMetaData &value) {
		display = value.display;
		text = value.text;
		hash = value.hash;
		track = value.track;
		compare = value.compare;
		return *this;
	}

	CMetaData() { prime(); };
	virtual ~CMetaData() = default;
};

struct CSortData {
	std::string artist;
	std::string albumartist;
	std::string album;

	util::hash_type artistHash;
	util::hash_type albumartistHash;
	util::hash_type albumHash;

	int year;

	void prime() {
		artistHash = 0;
		albumartistHash = 0;
		albumHash = 0;
		year = 1900;
	}

	void clear() {
		artist.clear();
		albumartist.clear();
		album.clear();
		prime();
	}

	bool isValid() const {
		return artistHash > 0 &&
			albumartistHash > 0 &&
			albumHash > 0;
	}

	CSortData& operator = (const music::CSortData &value) {
		artist = value.artist;
		albumartist = value.albumartist;
		album = value.album;
		artistHash = value.artistHash;
		albumartistHash = value.albumartistHash;
		albumHash = value.albumHash;
		return *this;
	}

	CSortData() { prime(); };
	virtual ~CSortData() = default;
};

struct CStatisticsData {
	size_t written;
	size_t read;
	size_t percent;
	size_t promille;
    util::TTimePart played; // Seconds

	void prime() {
		played = (util::TTimePart)0;
		clear();
	}

	void clear() {
		written = 0;
		read = 0;
		percent = 0;
		promille = 0;
	}

	void reset() {
		read = 0;
		percent = 0;
		promille = 0;
	}

	void update(util::TTimePart seconds) {
		if (read > 0 && written > 0) {
			if (read < written) {
				percent = read * 100 / written;
				promille = read * 1000 / written;
				played = read * seconds / written;
			} else {
				percent = 100;
				promille = 1000;
				played = seconds;
			}
		} else {
			percent = 0;
			promille = 0;
			played = 0;
		}
	}

	CStatisticsData& operator = (const music::CStatisticsData &value) {
		written = value.written;
		read = value.read;
		percent = value.percent;
		promille = value.promille;
		played = value.played; // Seconds
		return *this;
	}

	CStatisticsData() { prime(); };
	virtual ~CStatisticsData() = default;
};

struct CStreamData {
    size_t sampleCount;
    size_t sampleSize;
    int sampleRate;
    int bitsPerSample;
    int bytesPerSample;
    int channels;
    size_t bitRate;
    size_t chunkSize;
    util::TTimePart duration; // Milliseconds
    util::TTimePart seconds;  // Seconds
    std::string codec;

	void prime() {
	    sampleCount = 0;
	    sampleSize = 0;
	    sampleRate = 0;
	    bitsPerSample = 0;
	    bytesPerSample = 0;
	    channels = 0;
	    bitRate = 0;
	    chunkSize = 0;
	    duration = 0;
	    seconds = 0;
	}

	void setFixedRate(ESampleRate rate) {
	    sampleCount = rate;
	    sampleSize = 8 * sampleCount;
	    sampleRate = rate;
	    bitsPerSample = 16;
	    bytesPerSample = 2;
	    channels = 2;
	    bitRate = bytesPerSample * channels * 8 * rate;
	    chunkSize = 16536;
	    seconds = sampleRate / sampleCount;
	    duration = seconds * 1000;
	    codec = std::to_string((size_u)((int)rate / 1000)) + "K";
	}

	void clear() {
		prime();
		codec.clear();
	}

	bool isValid() const {
	    return sampleCount > 0 &&
	    	sampleSize > 0 &&
			bitsPerSample > 0 &&
			((bitsPerSample > 2) ? (bytesPerSample > 0) : true) &&
			channels > 0 &&
			sampleRate > 0 &&
			bitRate > 0 &&
			chunkSize > 0 &&
			duration > 0 &&
			seconds > 0;
	}

	bool isInvalid(std::string& hint) const {
	    if (!(sampleCount > 0)) {
	    	hint = util::csnprintf("SampleCount = %", sampleCount);
	    	return true;
	    }
	    if (!(sampleSize > 0)) {
	    	hint = util::csnprintf("SampleSize = %", sampleSize);
	    	return true;
	    }
	    if (!(bitsPerSample > 0)) {
	    	hint = util::csnprintf("BitsPerSample = %", bitsPerSample);
	    	return true;
	    }
	    if (!((bitsPerSample > 2) ? (bytesPerSample > 0) : true)) {
	    	hint = util::csnprintf("BitsPerSample=%, BytesPerSample=%", bitsPerSample, bytesPerSample);
	    	return true;
	    }
	    if (!(channels > 0)) {
	    	hint = util::csnprintf("Channels=%", channels);
	    	return true;
	    }
	    if (!(sampleRate > 0)) {
	    	hint = util::csnprintf("SampleRate=%", sampleRate);
	    	return true;
	    }
	    if (!(bitRate > 0)) {
	    	hint = util::csnprintf("BitRate=%", bitRate);
	    	return true;
	    }
	    if (!(chunkSize > 0)) {
	    	hint = util::csnprintf("ChunkSize=%", chunkSize);
	    	return true;
	    }
	    if (!(duration > 0)) {
	    	hint = util::csnprintf("Duration=%", duration);
	    	return true;
	    }
	    if (!(seconds > 0)) {
	    	hint = util::csnprintf("Seconds=%", seconds);
	    	return true;
	    }
    	hint = "Stream properties are valid";
	    return false;
	}

	void addErrorHint(std::string& error) {
		std::string hint;
		if (isInvalid(hint)) {
			if (!error.empty())
				error += ", ";
			if (!hint.empty()) {
				error += "Invalid stream properties found (" + hint + ")";
			} else {
				error += "Invalid stream properties found (Undefined reason)";
			}
		}
	}

	void debugOutput(const std::string& preamble = "") const {
		std::cout << preamble << "  Codec name        : " << codec << std::endl;
		std::cout << preamble << "  Sample count      : " << sampleCount << " samples" << std::endl;
		std::cout << preamble << "  Sample size       : " << sampleSize << " Byte [" << util::sizeToStr(sampleSize, 1, util::VD_BINARY) << "]" << std::endl;
		std::cout << preamble << "  Bits per sample   : " << bitsPerSample << " Bit" << std::endl;
		std::cout << preamble << "  Bytes per sample  : " << bytesPerSample << " Byte" << std::endl;
		std::cout << preamble << "  Sample rate       : " << util::cprintf("%.1f kS/s", (double)sampleRate / 1000.0) << " (" << sampleRate << ")" << std::endl;
		std::cout << preamble << "  Audio format      : " << channels << " channels" << std::endl;
		std::cout << preamble << "  Buffer chunk size : " << chunkSize << " Byte [" << util::sizeToStr(chunkSize, 1, util::VD_BINARY) << "]" << std::endl;
		std::cout << preamble << "  Bit rate          : " << bitRate << " kBit/s" << std::endl;
		std::cout << preamble << "  Duration          : " << seconds << " seconds [" << util::timeToHuman(seconds, 2, app::ELocale::en_US) << "]" << std::endl;
		std::cout << preamble << "  Valid             : " << isValid() << std::endl;
	}

	bool compare(const CStreamData& data) const {
		return (data.bitsPerSample == bitsPerSample) &&
				(data.channels == channels) &&
				(data.sampleRate == sampleRate);
	}

	inline bool operator == (const CStreamData& data) const { return compare(data); };
	inline bool operator != (const CStreamData& data) const { return !compare(data); };

	CStreamData& operator = (const music::CStreamData &value) {
		sampleCount = value.sampleCount;
		sampleSize = value.sampleSize;
		sampleRate = value.sampleRate;
		bitsPerSample = value.bitsPerSample;
		bytesPerSample = value.bytesPerSample;
		channels = value.channels;
		bitRate = value.bitRate;
		chunkSize = value.chunkSize;
		duration = value.duration; // Milliseconds
		seconds = value.seconds;  // Seconds
		codec = value.codec;
		return *this;
	}

	CStreamData() { prime(); };
	virtual ~CStreamData() = default;
};

struct CFileData {
	std::string filename;
	std::string basename;
	std::string extension;
	std::string folder;
	std::string url;
	util::TTimePart time;
	std::string timestamp;
	std::string insertstamp;
	util::TTimePart inserted;
	std::string hash;
	size_t size;

	void prime() {
		size = 0;
		time = util::epoch();
		inserted = util::epoch();
	}

	void clear() {
		filename.clear();
		basename.clear();
		extension.clear();
		folder.clear();
		url.clear();
		timestamp.clear();
		insertstamp.clear();
		hash.clear();
		prime();
	}

	bool isValid() const {
		return !filename.empty() &&
			!basename.empty() &&
			!extension.empty() &&
			!timestamp.empty() &&
			!folder.empty() &&
			!url.empty() &&
			time > 0 &&
			size > 0;
	}

	void debugOutput(const std::string& preamble = "") const {
		std::cout << preamble << "  Filename  : " << filename << std::endl;
		std::cout << preamble << "  Basename  : " << basename << std::endl;
		std::cout << preamble << "  Extension : " << extension << std::endl;
		std::cout << preamble << "  Folder    : " << folder << std::endl;
		std::cout << preamble << "  URL       : " << url << std::endl;
		std::cout << preamble << "  Time      : " << time << std::endl;
		std::cout << preamble << "  Timestamp : " << timestamp << std::endl;
		std::cout << preamble << "  Inserted  : " << inserted << std::endl;
		std::cout << preamble << "  Hash      : " << hash << std::endl;
		std::cout << preamble << "  Valid     : " << isValid() << std::endl;
	}

	CFileData& operator=(const music::CFileData &value) {
		filename = value.filename;
		basename = value.basename;
		extension = value.extension;
		folder = value.folder;
		url = value.url;
		time = value.time;
		timestamp = value.timestamp;
		insertstamp = value.insertstamp;
		inserted = value.inserted;
		hash = value.hash;
		size = value.size;
		return *this;
	}

	CFileData() { prime(); };
	virtual ~CFileData() = default;
};

typedef struct CCoverData {
	std::string filename;  // Filename of full scale image file
	TArtwork artwork;      // Raw JPEG byte buffer

	bool isValid() const {
		return !artwork.empty();
	}

	void clear() {
		filename.clear();
		artwork.clear();
	}

	CCoverData& operator = (const music::CCoverData &value) {
		filename = value.filename;  // Filename of full scale image file
		artwork = value.artwork;    // Raw JPEG byte buffer
		return *this;
	}

} TCoverData;

typedef struct CTrackData {
	CMetaData meta;
	CFileData file;
	CStreamData stream;

	void clear() {
		meta.clear();
		file.clear();
		stream.clear();
	}

	CTrackData& operator = (const music::CTrackData &value) {
		meta = value.meta;
		file = value.file;
		stream = value.stream;
		return *this;
	}

} TTrackData;


} /* namespace music */

#endif /* TAGTYPES_H_ */
