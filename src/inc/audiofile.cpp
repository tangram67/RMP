/*
 * audiofile.cpp
 *
 *  Created on: 14.08.2016
 *      Author: Dirk Brinkmeier
 */

#include "audiofile.h"
#include "templates.h"
#include "numlimits.h"
#include "charconsts.h"
#include "audiotypes.h"
#include "htmlutils.h"
#include "sysconsts.h"
#include "compare.h"
#include "encoding.h"
#include "convert.h"
#include "json.h"
#include "flac.h"
#include "ssl.h"

namespace music {

TTrack::TTrack() {
	prime();
}

TTrack::~TTrack() {
}

void TTrack::prime() {
	index = std::string::npos;
	deleted = false;
	removed = false;
	deferred = false;
	randomized = false;
	streamable = false;
	song = nil;
}

void TTrack::setSong(const PSong value) {
	song = value;
	setStreamable(util::assigned(value) ? value->isStreamed() : false);
}


TAudioFile::TAudioFile() {
}

TAudioFile::TAudioFile(const std::string& fileName, TFileTag& tag) {
	setFileProperties(fileName, tag);
}

TAudioFile::~TAudioFile() {
}


void TAudioFile::setFileProperties(const std::string& fileName, TFileTag& tag) {
	tag.file.filename  = fileName;
	tag.file.basename  = util::fileBaseName(fileName);
	tag.file.extension = util::fileExt(fileName);
	tag.file.folder    = util::filePath(fileName);
	tag.file.url       = util::TURL::encode(tag.file.folder);

	// Using lower case to detect file type!
	util::tolower(tag.file.extension);

	// Read file size and date
	if (readFileProperties(tag)) {
		tag.file.timestamp = util::dateTimeToStr(tag.file.time);
		tag.file.hash = createFileHash(tag);
	}
}

void TAudioFile::setDefaultProperties(const std::string& fileName, TFileTag& tag) {
	tag.file.filename  = fileName;
	tag.file.basename  = util::fileBaseName(fileName);
	tag.file.extension = util::fileExt(fileName);
	tag.file.folder    = util::filePath(fileName);
	tag.file.url       = util::TURL::encode(tag.file.folder);

	// Using lower case to detect file type!
	util::tolower(tag.file.extension);

	// Read file size and date
	tag.file.time = util::now();
	tag.file.timestamp = util::dateTimeToStr(tag.file.time);
	tag.file.hash = createFileHash(tag);
}

bool TAudioFile::readFileProperties(TFileTag& tag) {
    bool retVal = false;
	if (!tag.file.filename.empty()) {
		// Read file properties (follow links to target file)
#ifdef GCC_HAS_LARGEFILE64
		struct stat64 buf;
		if (EXIT_ERROR != stat64(tag.file.filename.c_str(), &buf)) {
#else
		struct stat buf;
		if (EXIT_ERROR != stat(tag.file.filename.c_str(), &buf)) {
#endif
			tag.file.size = buf.st_size;
			tag.file.time = buf.st_mtim.tv_sec; // st_mtime;
			retVal  = true;
		}
	}
	return retVal;
}

std::string TAudioFile::createFileHash(TFileTag& tag) {
	util::TDigest MD5(util::EDT_MD5);
	return MD5(tag.file.filename);
}


TAudioConvert::TAudioConvert() {
}

TAudioConvert::~TAudioConvert() {
}

int TAudioConvert::strToInt(const char *const value, char ** next, const int defValue) {
	if (util::assigned(next))
		*next = nil;

	if (!util::assigned(value))
		return defValue;

	long int retVal;
	char* q;
	errno = EXIT_SUCCESS;
	retVal = strtol(value, &q, 10);
	if (EXIT_SUCCESS != errno || value == q)
		return defValue;

	// Return pointer to next character for further processing
	if (util::assigned(next) && *q != util::NUL && value != q)
		*next = q;

	if (retVal > (long int)util::TLimits::LIMIT_INT32_MAX ||
		retVal < (long int)util::TLimits::LIMIT_INT32_MIN)
		retVal = defValue;

	if (retVal < 1)
		return defValue;

	return retVal;
}

int TAudioConvert::tagToInt(const std::string& value, const int defValue) {
	if (value.empty())
		return defValue;

	const char *const p = value.c_str();
	return strToInt(p, nil, defValue);
}

CTagValues TAudioConvert::tagToValues(const std::string& value, const int defValue) {
	CTagValues r;

	if (value.empty()) {
		r.value = defValue;
		return r;
	}

	const char *const p = value.c_str();
	char* q = nil;
	r.value = strToInt(p, &q, defValue);
	if (util::assigned(q)) {
		++q;
		if (*q != util::NUL)
			r.count = strToInt(q, nil, 0);
	}
	return r;
}

int TAudioConvert::normalizeSampleSize(const int bits) {
	// e.g. HDCD ripped by dbPoweramp pretends to be 20 bits...
	if (bits > 24)
		return 32;
	if (bits > 16)
		return 24;
	if (bits > 8)
		return 16;
	return 8;
}

size_t TAudioConvert::bitRateFromTags(const TFileTag& tag) {
	if (tag.stream.sampleSize > 0 && tag.stream.duration > 0)
		return tag.stream.sampleSize * 1000 / tag.stream.duration / 128; // in kiBit/sec (1024 for kiBit / 8 Bit in sampleSize --> divide by 128)
	if (tag.stream.sampleRate > 0 && tag.stream.bitsPerSample > 0 && tag.stream.channels > 0)
		return tag.stream.sampleRate * tag.stream.bitsPerSample * tag.stream.channels / 1024;
	return (size_t)0;
}


TSong::TSong(const ECodecType type) {
	prime();
	setType(type);
}

TSong::TSong(const std::string& fileName) {
	prime();
	type = getFileType(fileName);
	streamable = getStreamable(type);
	setFileProperties(fileName);
}

TSong::TSong(const std::string& fileName, const ECodecType type) {
	prime();
	this->type = type;
	streamable = getStreamable(type);
	setFileProperties(fileName);
}

TSong::~TSong() {
	TAudioStreamAdapter::clear();
}

void TSong::prime() {
	valid = false;
	changed = false;
	deleted = false;
	loaded = true;
	buffered = false;
	streamed = false;
	streamable = false;
	index = std::string::npos;
	tags.stream.codec = "<unknown>";
	media = EMT_UNKNOWN;
	params = -1;
	c_activated = false;
	c_extended = false;
}


bool TSong::isChanged(const bool reset) {
	bool r = changed;
	if (reset)
		changed = false;
	return r;
}


bool TSong::isSupported(std::string& hint) const {
	// Stereo files only
	if (tags.stream.channels != 2) {
		hint = util::csnprintf("Invalid channel count (%)", tags.stream.channels);
		return false;
	}

	// 1 (DSD), 16 and 24 Bit files are supported
	if (!util::isMemberOf((ESampleSize)tags.stream.bitsPerSample, ES_DSD_OE,ES_DSD_NE,ES_CD,ES_HR)) {
		hint = util::csnprintf("Invalid bits per sample (%)", tags.stream.bitsPerSample);
		return false;
	}

	// 44100, 48000, 88200, 96000, 176400, 192000, 352800 and 384000 Samples/sec supported
	if (!util::isMemberOf((ESampleRate)tags.stream.sampleRate, SR44K,SR48K,SR88K,SR96K,SR176K,SR192K,SR352K,SR384K)) {
		hint = util::csnprintf("Invalid sample rate (%)", tags.stream.sampleRate);
		return false;
	}

	// Check for supported type
	switch (type) {
		case EFT_WAV:
		case EFT_DSF:
		case EFT_DFF:
		case EFT_FLAC:
		case EFT_AIFF:
#ifdef SUPPORT_ALAC_FILES
		case EFT_ALAC:
#endif
#ifdef SUPPORT_AAC_FILES
		case EFT_AAC:
#endif
			return true;
			break;
		case EFT_MP3:
			if (tags.stream.bitsPerSample != (int)ES_CD) {
				hint = util::csnprintf("Invalid word size for MP3 decoder: % (16 Bit only!)", tags.stream.bitsPerSample);
				return false;
			} else {
				return true;
			}
			break;
		default:
			break;
	}

	hint = util::csnprintf("Unsupported file type (%)", getCodec());
	return false;
}


size_t TSong::getBytesPerSample() const {
	if (isDSD()) {
		return 2;
	}
	return getBitsPerSample() / 8;
}


bool TSong::getStreamable(const ECodecType type) const {
	return type == EFT_FLAC || type == EFT_MP3 || type == EFT_OGG || type == EFT_ALAC;
}


int TSong::readMetadataTags(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
		const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble) {
	int retVal = EXIT_SUCCESS;

	// Read tags from file
	bool ok = false;
	try {
		// Metatag readers my throw exceptions on file access, etc.
		ECodecType t = type;
		ok = readMetaData(tags.file.filename, tags, t);
		if (ok) {
			if (type != t)
				setType(t);
		} else {
			retVal = -1;
		}
	} catch (const std::exception& e) {
		retVal = -10;
		tags.error = e.what();
	} catch (...) {
		retVal = -10;
		tags.error = "Unknown exception";
	}

	// Process supported files only
	if (ok) {
		std::string hint;
		ok = isSupported(hint);
		if (!ok) {
			if (!tags.error.empty())
				tags.error += ", ";
			tags.error += hint;
			retVal = -2;
		}
	}

	// Set default values on import of new file
	tags.meta.track.compilation = false;

	// Modify tags as configured
	if (ok) {
		setScannerParams(allowArtistNameRestore, allowFullNameSwap, allowGroupNameSwap,
				allowTheBandPrefixSwap, allowVariousArtistsRename, allowMovePreamble);
		normalizeTags(allowGroupNameSwap);
		if (tags.meta.isValid()) {
			util::trim(tags.meta.text.artist);
			util::trim(tags.meta.text.composer);
			util::trim(tags.meta.text.conductor);
			util::trim(tags.meta.text.albumartist);
			if (allowFullNameSwap || allowArtistNameRestore || allowTheBandPrefixSwap) {
				size_t ca = findArtistSeparator(tags.meta.text.artist);
				size_t co = findArtistSeparator(tags.meta.text.composer);
				size_t cn = findArtistSeparator(tags.meta.text.conductor);
				size_t cb = findArtistSeparator(tags.meta.text.albumartist);
				if (allowFullNameSwap) {
					swapArtistName(tags.meta.text.artist, ca);
					swapArtistName(tags.meta.text.composer, co);
					swapArtistName(tags.meta.text.conductor, cn);
					swapArtistName(tags.meta.text.albumartist, cb);
				} else {
					if (allowArtistNameRestore) {
						restoreArtistName(tags.meta.text.artist, ca);
						restoreArtistName(tags.meta.text.composer, co);
						restoreArtistName(tags.meta.text.conductor, cn);
						restoreArtistName(tags.meta.text.albumartist, cb);
						ca = co = cn = cb = std::string::npos;
					}
					if (allowTheBandPrefixSwap) {
						if (tags.meta.text.originalartist.empty())
							tags.meta.text.originalartist = tags.meta.text.artist;
						if (tags.meta.text.originalalbumartist.empty())
							tags.meta.text.originalalbumartist = tags.meta.text.albumartist;
						swapTheBandPrefix(tags.meta.text.artist, ca);
						swapTheBandPrefix(tags.meta.text.composer, co);
						swapTheBandPrefix(tags.meta.text.conductor, cn);
						swapTheBandPrefix(tags.meta.text.albumartist, cb);
					}
				}
			}
			if (allowMovePreamble) {
				moveAlbumPreamble(tags.meta.text.album);
			}
		} else {
			setDefaultTags(tags.file.filename);
		}
		if (allowVariousArtistsRename) {
			renameVariousArtists();
		} else {
			setCompilationTags(VARIOUS_ARTISTS_NAME);
		}
		sanitizeTags();
		setSortTags();
		setDisplayTags();
	}

	return retVal;
}


void TSong::setScannerParams(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
		const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble) {
	params = evalScannerParams(allowArtistNameRestore, allowFullNameSwap, allowGroupNameSwap,
			allowTheBandPrefixSwap, allowVariousArtistsRename, allowMovePreamble);
}

std::string TSong::getAlbumSortName() const {
	return tags.meta.text.album + "/" + tags.meta.text.albumartist;
}

void TSong::setDisplayTags() {
	// Sanitize HTML display text
	tags.meta.display.artist = html::THTML::encode(tags.meta.text.artist);
	tags.meta.display.originalartist = html::THTML::encode(tags.meta.text.originalartist);
	tags.meta.display.albumartist = html::THTML::encode(tags.meta.text.albumartist);
	tags.meta.display.originalalbumartist = html::THTML::encode(tags.meta.text.originalalbumartist);
	tags.meta.display.album = html::THTML::encode(tags.meta.text.album);
	tags.meta.display.title = html::THTML::encode(tags.meta.text.title);
	tags.meta.display.track = html::THTML::encode(tags.meta.text.track);
	tags.meta.display.disk = html::THTML::encode(tags.meta.text.disk);
	tags.meta.display.year = html::THTML::encode(tags.meta.text.year);
	tags.meta.display.date = html::THTML::encode(tags.meta.text.date);
	tags.meta.display.genre = html::THTML::encode(tags.meta.text.genre);
	tags.meta.display.composer = html::THTML::encode(tags.meta.text.composer);
	tags.meta.display.conductor = html::THTML::encode(tags.meta.text.conductor);
	tags.meta.display.comment = html::THTML::encode(tags.meta.text.comment);
	tags.meta.display.description = html::THTML::encode(tags.meta.text.description);

	// Set default extended album display info
	setExtendedTags(tags.meta.display);
}

void TSong::setSortTags() {
	// Album must be made unique:
	// --> Prevent "Greatest Hits" (e.g.) get mixed up between artists
	std::string album = getAlbumSortName();

	// Sort tags case insensitive
	tags.sort.album = util::tolower(album);
	tags.sort.artist = util::tolower(tags.meta.text.artist);
	tags.sort.albumartist = util::tolower(tags.meta.text.albumartist);
	tags.sort.albumHash = hash(tags.sort.album);
	tags.sort.artistHash = hash(tags.sort.artist);
	tags.sort.albumartistHash = hash(tags.sort.albumartist);
	tags.sort.year = tagToInt(tags.meta.text.year, 1900);

	// Create unique md5 hash strings
	tags.meta.hash.title = md5(tags.file.filename + album + tags.meta.text.title);
	tags.meta.hash.album = md5(album);
	tags.meta.hash.artist = md5(tags.meta.text.artist);
	tags.meta.hash.albumartist = md5(tags.meta.text.albumartist);

	// Set compare tags
	setCompareTags();
}

void TSong::setCompareTags() {
	// Set compare tags
	if (!tags.meta.text.originalartist.empty()) {
		if (!tags.meta.text.albumartist.empty() && ((tags.meta.text.originalartist == tags.meta.text.albumartist)    || (tags.meta.text.artist == tags.meta.text.albumartist)))    tags.meta.compare.albumIsArtist = true;
		if (!tags.meta.text.composer.empty()    && ((tags.meta.text.originalartist == tags.meta.text.composer)       || (tags.meta.text.artist == tags.meta.text.composer)))       tags.meta.compare.composerIsArtist = true;
		if (!tags.meta.text.conductor.empty()   && ((tags.meta.text.originalartist == tags.meta.text.conductor)      || (tags.meta.text.artist == tags.meta.text.conductor)))      tags.meta.compare.conductorIsArtist = true;
		if (!tags.meta.text.composer.empty()    && ((tags.meta.text.originalalbumartist == tags.meta.text.composer)  || (tags.meta.text.albumartist == tags.meta.text.composer)))  tags.meta.compare.composerIsAlbumArtist = true;
		if (!tags.meta.text.conductor.empty()   && ((tags.meta.text.originalalbumartist == tags.meta.text.conductor) || (tags.meta.text.albumartist == tags.meta.text.conductor))) tags.meta.compare.conductorIsAlbumArtist = true;
	}

	// Set default extended album text info
	setExtendedTags(tags.meta.text);
}


void TSong::setExtendedTags(CTextMetaData& info) {
	// Set default extended album info
	info.extendedalbum = info.album;
	info.extendedinfo = info.extendedalbum;

	// Set extended album information:
	// --> Album + Orchestra + Conductor + Composer
	// --> Ignore compilations!
	if (!tags.meta.track.compilation) {
		int count = 0;
		bool first = true;
		if (!(info.conductor.empty() || tags.meta.compare.conductorIsArtist)) {
			info.extendedinfo += (first ? "<small><br/>" : " · ") + info.conductor;
			first = false;
			++count;
		}
		if (count > 0) {
			info.extendedinfo += "</small>";
		}
	}
}


void TSong::setCompilationTags(const std::string& artist) {
	if (tags.meta.track.compilation) {

		// Save artist name as original artist name
		if (tags.meta.text.originalartist.empty())
			tags.meta.text.originalartist = tags.meta.text.artist;
		if (tags.meta.text.originalalbumartist.empty())
			tags.meta.text.originalalbumartist = tags.meta.text.albumartist;

		// Overwrite artist name with "Various Artists"
		tags.meta.text.artist = artist;
		tags.meta.text.albumartist = artist;

		// Set changed sort tags
		setSortTags();
	} else {
		setCompareTags();
	}
}


void TSong::setCompilation(const std::string& artist) {
	tags.meta.track.compilation = true;
	if (!artist.empty()) {
		setCompilationTags(artist);
	}
};


void TSong::setInsertedTime(const util::TTimePart time) {
	tags.file.inserted = time;
	tags.file.insertstamp = util::dateTimeToStr(time);
}


bool TSong::isVariousArtistName(const std::string& name) const {
	return util::strcasestr(name, "sampler") ||
		   util::strcasestr(name, "various") ||
		   util::strcasestr(name, "soundtrack") ||
		   util::strcasestr(name, "compilation") ||
		   util::strcasestr(name, "divers");
}

void TSong::renameVariousArtists() {
	if (tags.meta.track.compilation || isVariousArtistName(tags.file.folder)) {
		setCompilation(VARIOUS_ARTISTS_NAME);
	}
}

void TSong::normalizeTags(const bool allowGroupNameSwap) {
	// Check if artist is album artist
	if (tags.meta.text.artist.empty() && !tags.meta.text.albumartist.empty())
		tags.meta.text.artist = tags.meta.text.albumartist;

	// Check for Album Artist a.k.a. Group a.k.a. orchestra
	if (tags.meta.text.albumartist.empty() && !tags.meta.text.artist.empty())
		tags.meta.text.albumartist = tags.meta.text.artist;

	// Check if album artist is single artist
	// --> String is shorter than artist, this is a raw assumption!
	if (false) { // allowGroupNameSwap) {
		if (!tags.meta.text.artist.empty() && !tags.meta.text.albumartist.empty() && (tags.meta.text.artist.size() > (7 * tags.meta.text.albumartist.size() / 4)) \
			&& (!util::strcasestr(tags.meta.text.albumartist, "various"))) {
			std::string a = tags.meta.text.artist;
			tags.meta.text.artist = tags.meta.text.albumartist;
			tags.meta.text.albumartist = a;
		}
	}
}

size_t TSong::findArtistSeparator(const std::string& name) {
	return name.find(", ");
}

bool TSong::restoreArtistName(std::string& name, const size_t separator) {
	// Order artist by full name:
    //  - Convert "Beethoven, Ludwig van" back to original name "Ludwig van Beethoven"
    if (separator != std::string::npos) {
    	std::string lastname = name.substr(0, separator);
    	std::string givenname = name.substr(separator + 2, std::string::npos);
    	if (!givenname.empty() && !lastname.empty()) {
    		if (givenname.size() < 15) {
    			name = givenname + " " + lastname;
    			return true;
    		}
    	}
    }
    return false;
}

bool TSong::restoreArtistName(std::string& name) {
	// Order artist by full name:
    //  - Convert "Beethoven, Ludwig van" back to original name "Ludwig van Beethoven"
	size_t separator = findArtistSeparator(name);
	return restoreArtistName(name, separator);
}


bool isPreamble(char c) {
	if ((unsigned char)c > UINT8_C(0x7F))
		return false;
	return c != '.' && c != '\"' && c != '\'' && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9');
}

bool TSong::moveAlbumPreamble(std::string& name) {
	// Remove trailing preamble (first "word" until SPACE) like "[...] Title of Album" --> "Title of Album"
    //  - Whitespace is lower than 'A' like "[](){}+-#..." except numerics "0..9"
	//  - First valid char is the one after SPACE
	size_t idx = std::string::npos;
	if (name.size() > 2) {
		if (isPreamble(name[0])) {
			for (size_t i=0; i<name.size(); i++) {
				if (' ' == name[i]) {
					idx = i;
					break;
				}
			}
			if (std::string::npos != idx) {
				std::string preamble = name.substr(0, idx);
				name = util::trim(name.substr(util::succ(idx), std::string::npos)) + " " + preamble;
				return true;
			}
		}
	}
	return false;
}


bool TSong::swapTheBandPrefix(std::string& name, const size_t separator) {
	// Order artist by name (not prefix!):
    //  - Convert "The Motors" to "Motors, The"
	//  - This conversion is also included in swapArtistName()
    if (separator == std::string::npos && name.size() > 4) {
    	if (0 == util::strncasecmp(name, "The ", 4)) {
			std::string bandname = name.substr(4, std::string::npos);
			if (!bandname.empty()) {
				util::trimLeft(bandname);
				name = bandname + ", The";
				return true;
			}
		}
    }
    return false;
}

bool TSong::swapTheBandPrefix(std::string& name) {
	// Order artist by name (not prefix!):
    //  - Convert "The Motors" to "Motors, The"
	//  - This conversion is also included in swapArtistName()
	size_t separator = findArtistSeparator(name);
	return swapTheBandPrefix(name, separator);
}


size_t TSong::findArtistLastName(const std::string& name) {
	if (name.size() > 1) {
		size_t len = name.size() - 1;
		const char* p = name.c_str() + len;
		for (ssize_t i=len; i>0; --i) {
			if (util::isWhiteSpaceA(*p)) {
				return i + 1;
			}
			--p;
		}
	}
	return std::string::npos;
}

void TSong::swapArtistName(std::string& name, const size_t separator) {
	// Order artist by lastname:
    //  - Convert "Ludwig van Beethoven" to "Beethoven, Ludwig van"
    //  - Includes swapping "The Motors" to "Motors, The" (!)
	if (separator == std::string::npos) {
    	size_t pos = findArtistLastName(name);
        if (pos != std::string::npos && pos > 1) {
        	std::string lastname = name.substr(pos);
        	std::string givenname = name.substr(0, pos - 1);
			name = lastname + ", " + givenname;
        }
    }
}

void TSong::sanitizeTags() {
	if (tags.meta.text.originalartist.empty())
		tags.meta.text.originalartist = tags.meta.text.artist;
	if (tags.meta.text.originalalbumartist.empty())
		tags.meta.text.originalalbumartist = tags.meta.text.albumartist;
	if (tags.meta.text.genre.empty())
		tags.meta.text.genre = "Unknown";
	if (util::strToInt(tags.meta.text.year, 0) < 1900)
		tags.meta.text.year = "1900";
//	music::TFileTag::sanitize(tags.meta.text.artist);
//	music::TFileTag::sanitize(tags.meta.text.originalartist);
//	music::TFileTag::sanitize(tags.meta.text.albumartist);
//	music::TFileTag::sanitize(tags.meta.text.originalalbumartist);
//	music::TFileTag::sanitize(tags.meta.text.album);
//	music::TFileTag::sanitize(tags.meta.text.title);
//	music::TFileTag::sanitize(tags.meta.text.genre);
//	music::TFileTag::sanitize(tags.meta.text.composer);
//	music::TFileTag::sanitize(tags.meta.text.conductor);
//	music::TFileTag::sanitize(tags.meta.text.comment);
//	music::TFileTag::sanitize(tags.meta.text.description);
}

void TSong::setDefaultTags(const std::string& fileName, const int defaultTrack) {
	// Set pseudo tags:
	//  - Assuming storage like: /music/Van Halen/1984/01 - 1984.wav
	size_t offset = 0;
	std::string base = util::fileBaseName(fileName);
	std::string path = util::filePath(fileName);

	// Extract artist and album from path
	// Assuming storage like: /music/Van Halen/1984/
	std::string album;
	std::string artist;
	extractAlbumArtist(path, album, artist);

	// Extract track number from file name
	// Assuming file starts with valid number like: 01 - 1984.wav
	if (tags.meta.track.tracknumber <= 0) {
		tags.meta.track.tracknumber = extractTrackNumber(base, offset, defaultTrack);
		tags.meta.text.track        = util::cprintf("%02d", tags.meta.track.tracknumber);
	}

	// Fill missing tags...
	if (tags.meta.text.track.empty())                  tags.meta.text.track       = util::cprintf("%02d", tags.meta.track.tracknumber);
	if (tags.meta.text.genre.empty())                  tags.meta.text.genre       = "Unknown";
	if (util::strToInt(tags.meta.text.year, 0) < 1900) tags.meta.text.year        = "1900";
	if (tags.meta.text.album.empty())                  tags.meta.text.album       = album.empty() ? path : album;
	if (tags.meta.text.artist.empty())                 tags.meta.text.artist      = artist.empty() ? "Unknown" : artist;
	if (tags.meta.text.albumartist.empty())            tags.meta.text.albumartist = tags.meta.text.artist;
	if (tags.meta.text.title.empty())                  tags.meta.text.title       = extractTrackTitle(base, offset);

	// Original artist is set to artist tag by default
	tags.meta.text.originalartist = tags.meta.text.artist;
	tags.meta.text.originalalbumartist = tags.meta.text.albumartist;
}

int TSong::extractTrackNumber(const std::string& name, size_t& offset, const int defaultTrack) {
	offset = 0;

	// Invalid tag...
	if (name.empty())
		return defaultTrack;

	const char *const p = name.c_str();
	long int retVal;
	char* q;

	errno = EXIT_SUCCESS;
	retVal = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q )
		return defaultTrack;

	if (retVal > 99 || retVal < 1)
		return defaultTrack;

	// Return offset to next non numeric character
	if (q > p)
		offset = q - p;

	// Return tracknumber
	return retVal;
}

std::string TSong::extractTrackTitle(const std::string& name, size_t offset) {
	std::string s;

	// Find first letter
	size_t pos = 0;
	for (size_t i=offset; i<name.size(); ++i) {
		if (util::isAlphaA(name[i])) {
			pos = i;
			break;
		}
	}

	// Return substring from first letter
	if (pos > 0) {
		s = name.substr(pos);
	}
	if (s.empty())
		s = name;

	return s;
}

bool TSong::extractAlbumArtist(const std::string& path, std::string& album, std::string& artist) {
	int found = 0;
	util::TStringList sl;
	sl.split(util::stripLastPathSeparator(path), sysutil::PATH_SEPERATOR);
	if (sl.size() > 1) {
		size_t idx = util::pred(sl.size());
		if (!sl[idx].empty()) {
			album = util::trim(sl[idx]);
			found |= 1;
		}
		if (!sl[--idx].empty()) {
			artist = util::trim(sl[idx]);
			found |= 2;
		}
	}
	return (found == 3);
}

void TSong::updateProperties() {

	// Set valid codec name
	if (!hasCodec()) {
		setType(getType());
	}

	// Set default insert time as file creation date
	if (tags.file.inserted <= util::epoch()) {
		tags.file.inserted = tags.file.time;
		tags.file.insertstamp = tags.file.timestamp;
	}

	// Set media icon
	if (media == EMT_UNKNOWN || icon.empty()) {

		// Known file types
		if (isDSD()) {
			media = EMT_DSD;
			if (tags.stream.sampleRate == (int)SR176K) {
				icon = "sacd-audio";
				return;
			}
			icon = "dsd-audio";
			return;
		}
		if (isHDCD()) {
			media = EMT_HDCD;
			icon = "hdcd-audio";
			return;
		}

		// Guess file types...
		if (tags.stream.sampleRate == (int)SR88K) {
			media = EMT_HR;
			icon = "hr-audio";
			return;
		}
		if (tags.stream.sampleRate < (int)SR48K) {
			media = EMT_CD;
			icon = "cd-audio";
			return;
		}
		if (tags.stream.sampleRate < (int)SR96K) {
			media = EMT_DVD;
			icon = "dvd-audio";
			return;
		}
		if (tags.stream.sampleRate < (int)SR176K) {
			if (tags.stream.bitsPerSample < (int)ES_HR) {
				media = EMT_DVD;
				icon = "dvd-audio";
				return;
			}
			media = EMT_BD;
			icon = "bd-audio";
			return;
		}

		// Higher rates are always HR audio files
		media = EMT_HR;
		icon = "hr-audio";
	}
}


util::hash_type TSong::hash(const std::string& value) {
	util::hash_type hash = 0;
	if (!value.empty()) {
		for (size_t i=0; i<value.size(); ++i) {
			hash = (hash << 5) - hash + (util::hash_type)::tolower(value[i]);
		}
	}
	return hash;
}

std::string TSong::md5(const std::string& value) {
	util::TDigest MD5(util::EDT_MD5);
	return MD5(value);
}

void TSong::setDefaultFileProperties(const std::string& fileName, const ESampleRate sampleRate, int bitsPerSample) {
	TAudioFile file;
	file.setDefaultProperties(fileName, tags);

	// Set default metadata
	tags.meta.text.title = tags.file.basename;
	tags.meta.display.title = tags.file.basename;

	// Set default stream properties
	setDefaultStreamProperties(sampleRate, bitsPerSample);

	// Create default md5 hash strings
	tags.meta.hash.title = md5(tags.file.filename + "X" + tags.file.extension);
	tags.meta.hash.album = md5(tags.file.basename);
	tags.meta.hash.artist = md5(tags.meta.hash.title);
	tags.meta.hash.albumartist = md5(tags.meta.hash.album);

	changed = false;
	valid = false;
}

void TSong::setDefaultStreamProperties(const ESampleRate sampleRate, int bitsPerSample) {

	// Set default stream properties
	tags.stream.bitsPerSample = bitsPerSample;
	tags.stream.bytesPerSample = bitsPerSample / 8;
	tags.stream.channels = 2;
	tags.stream.sampleRate = sampleRate;
	tags.stream.sampleCount = (size_t)-1;
	tags.stream.sampleSize = (size_t)-1;
	tags.stream.bitRate = 1024;
	tags.stream.chunkSize = MP3_OUTPUT_CHUNK_SIZE;
	tags.stream.duration = 1000000;
	tags.stream.seconds = 1000000;

	valid = false;
}

void TSong::updateStreamProperties(const ESampleRate sampleRate, int bitsPerSample) {

	// Detect property changes
	if (tags.stream.bitsPerSample != bitsPerSample || tags.stream.sampleRate != sampleRate) {
		changed = true;
	}

	// Set default stream properties
	setDefaultStreamProperties(sampleRate, bitsPerSample);
}

void TSong::setFileProperties(const std::string& fileName) {
	TAudioFile file(fileName, tags);
	valid = tags.file.isValid();
}

void TSong::setFileProperties(const TFileTag tag) {
	tags.file = tag.file;
	valid = tags.file.isValid();
}

bool TSong::compareByTitleHash(const std::string& hash) const {
	return getTitleHash() == hash;
}

bool TSong::compareByTitleHash(const TSong* song) const {
	if (util::assigned(song)) {
		return compareByTitleHash(song->getTitleHash());
	}
	return false;
}

bool TSong::compareByTitleHash(const TSong& song) const {
	return compareByTitleHash(song.getTitleHash());
}

bool TSong::compareByFileHash(const std::string& hash) const {
	return getFileHash() == hash;
}

bool TSong::compareByFileHash(const TSong* song) const {
	if (util::assigned(song)) {
		return compareByFileHash(song->getFileHash());
	}
	return false;
}

bool TSong::compareByFileHash(const TSong& song) const {
	return compareByFileHash(song.getFileHash());
}

ECodecType TSong::getFileType(const std::string& fileName) {
	if (!fileName.empty()) {
		std::string extension = util::fileExt(fileName);
		if (!extension.empty()) {
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			const struct TCodecType *it;
			for (it = codecs; util::assigned(it->extension); ++it) {
				if (util::assigned(strstr(extension.c_str(), it->extension))) {
					return it->type;
				}
			}
		}
	}
	return EFT_UNKNOWN;
}

std::string TSong::typeAsString(const ECodecType value) const {
	const struct TCodecType *it;
	for (it = codecs; util::assigned(it->extension); ++it) {
		if (it->type == type) {
			return it->shortcut;
		}
	}
	return "<unknown>";
}

void TSong::setType(const ECodecType value) {
	type = value;
	streamable = getStreamable(type);
	tags.stream.codec = typeAsString(type);
}

int TSong::evalScannerParams(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
	const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble) {
	// EST_GROUP_NAME_SWAP        = (1 << 0)
	// EST_ARTIST_NAME_RESTORE    = (1 << 1)
	// EST_FULL_NAME_SWAP         = (1 << 2)
	// EST_THE_BAND_PREFIX_SWAP   = (1 << 3)
	// EST_DEEP_NAME_INSPECTION   = (1 << 4)
	// EST_VARIOUS_ARTISTS_RENAME = (1 << 5)
	// EST_SUPPRESS_PREAMBLE      = (1 << 6)
	int r = EST_UNKNOWN;
	// if (allowGroupNameSwap)        r |= EST_GROUP_NAME_SWAP;
	if (allowArtistNameRestore)    r |= EST_ARTIST_NAME_RESTORE;
	if (allowFullNameSwap)         r |= EST_FULL_NAME_SWAP;
	if (allowTheBandPrefixSwap)    r |= EST_THE_BAND_PREFIX_SWAP;
	// if (allowDeepNameInspection)   r |= EST_DEEP_NAME_INSPECTION;
	if (allowVariousArtistsRename) r |= EST_VARIOUS_ARTISTS_RENAME;
	if (allowMovePreamble)         r |= EST_MOVE_PREAMBLE;
	return r;
}

std::string TSong::paramsAsString(int param) const {
	std::string s;
	s.reserve(16);
	s.insert(s.begin(), 16, '0');
	if (param > 0) {
		if (param & EST_GROUP_NAME_SWAP)        s[0] = '1';
		if (param & EST_ARTIST_NAME_RESTORE)    s[1] = '1';
		if (param & EST_FULL_NAME_SWAP)         s[2] = '1';
		if (param & EST_THE_BAND_PREFIX_SWAP)   s[3] = '1';
		if (param & EST_DEEP_NAME_INSPECTION)   s[4] = '1';
		if (param & EST_VARIOUS_ARTISTS_RENAME) s[5] = '1';
		if (param & EST_MOVE_PREAMBLE)          s[6] = '1';
		if (param & EST_URL_ENCODED)            s[9] = '1';
	}
	return s;
}

int TSong::paramsFromString(const std::string& param) const {
	int r = 0;
	if (param.size() >= 16) {
		if (param[0] == '1') r |= EST_GROUP_NAME_SWAP;
		if (param[1] == '1') r |= EST_ARTIST_NAME_RESTORE;
		if (param[2] == '1') r |= EST_FULL_NAME_SWAP;
		if (param[3] == '1') r |= EST_THE_BAND_PREFIX_SWAP;
		if (param[4] == '1') r |= EST_DEEP_NAME_INSPECTION;
		if (param[5] == '1') r |= EST_VARIOUS_ARTISTS_RENAME;
		if (param[6] == '1') r |= EST_MOVE_PREAMBLE;
		if (param[9] == '1') r |= EST_URL_ENCODED;
		return r;
	}
	return -1;
}

std::string TSong::timeToStr(const util::TTimePart seconds) {
	if (seconds < 3600)
		return util::timeToHuman(seconds, 2, app::ELocale::cloc);
	return util::timeToHuman(seconds, 3, app::ELocale::cloc);
}

std::string TSong::encode(const std::string& text, const bool encoded) {
	return encoded ? util::TURL::encode(text, util::TURL::URL_EXTENDED, util::TURL::UST_REPLACED) : text;
}

std::string TSong::decode(const std::string& text, const bool encoded) {
	return encoded ? util::TURL::decode(util::unquote(text)) : util::unquote(text);
}

std::string TSong::text(const char delimiter, const bool encoded) {
	std::string d(&delimiter, 1);
	std::string r("");
	size_t size = 2 +
			tags.meta.text.artist.size() +
			tags.meta.text.albumartist.size() +
			tags.meta.text.album.size() +
			tags.meta.text.title.size() +
			tags.meta.text.genre.size() +
			tags.meta.text.composer.size() +
			tags.meta.text.conductor.size() +
			tags.meta.text.originalalbumartist.size() +
			tags.meta.hash.title.size() +
			tags.meta.hash.album.size() +
			tags.meta.hash.artist.size() +
			tags.meta.hash.albumartist.size() +
			tags.meta.text.track.size() +
			tags.meta.text.disk.size() +
			tags.meta.text.year.size() +
			tags.meta.text.date.size() +
			tags.file.url.size() +
			tags.file.timestamp.size();
			tags.file.hash.size();
	r.reserve(2*size);

	// Write file codec type
	r += util::cprintf("%d", type).append(d); // Index 0

	// Tag meta data (quote all strings)
	r += util::quote(encode(tags.meta.text.artist, encoded)).append(d);					// Index 1
	r += util::quote(encode(tags.meta.text.originalartist, encoded)).append(d);			// Index 2
	r += util::quote(encode(tags.meta.text.albumartist, encoded)).append(d);			// Index 3
	r += util::quote(encode(tags.meta.text.album, encoded)).append(d);					// Index 4
	r += util::quote(encode(tags.meta.text.title, encoded)).append(d);					// Index 5
	r += util::quote(encode(tags.meta.text.genre, encoded)).append(d);					// Index 6
	r += util::quote(encode(tags.meta.text.composer, encoded)).append(d);				// Index 7
	r += util::quote(encode(tags.meta.text.conductor, encoded)).append(d);				// Index 8
	r += util::quote(encode(tags.meta.text.originalalbumartist, encoded)).append(d);	// Index 9


	// Tag meta data (quote hasches)
	r += util::quote(tags.meta.hash.title).append(d); // Index 10
	r += util::quote(tags.meta.hash.album).append(d); // Index 11

	// Track meta data
	std::string track = tags.meta.text.track.empty() ? "01" : tags.meta.text.track;
	if (tags.meta.track.trackcount > 0) {
		r += track + "/" + util::cprintf("%02.2d", tags.meta.track.trackcount) + d;	// Index 12
	} else {
		r += encode(track, encoded) + d;
	}

	std::string disk = tags.meta.text.disk.empty() ? "0" : tags.meta.text.disk;
	if (tags.meta.track.diskcount > 0) {
		r += disk + "/" + util::cprintf("%02.2d", tags.meta.track.diskcount) + d; // Index 12
	} else {
		r += encode(disk, encoded) + d;
	}

	// Track date data
	r += encode(tags.meta.text.year, encoded) + d; // Index 14
	r += encode(tags.meta.text.date, encoded) + d; // Index 15

    // Stream data
	r += util::cprintf("%ld", tags.stream.sampleCount).append(d);		// Index 16
	r += util::cprintf("%ld", tags.stream.sampleSize).append(d);		// Index 17
	r += util::cprintf("%ld", tags.stream.sampleRate).append(d);		// Index 18
	r += util::cprintf("%ld", tags.stream.bitsPerSample).append(d);		// Index 19
	r += util::cprintf("%ld", tags.stream.bytesPerSample).append(d);	// Index 20
	r += util::cprintf("%ld", tags.stream.channels).append(d);			// Index 21
	r += util::cprintf("%ld", tags.stream.bitRate).append(d);			// Index 22
	r += util::cprintf("%ld", tags.stream.chunkSize).append(d);			// Index 23
	r += util::cprintf("%ld", tags.stream.duration).append(d);			// Index 24
	r += util::cprintf("%ld", tags.stream.seconds).append(d);			// Index 25
	r += timeToStr(tags.stream.seconds).append(d);						// Index 26

	// File properties
	r += util::TURL::encode(tags.file.filename) + d;		// Index 27
	r += tags.file.timestamp + d;							// Index 28
	r += util::cprintf("%ld", tags.file.size).append(d);	// Index 29
	r += tags.file.hash + d;								// Index 30

	// Scanner configuration and string URL encoding parameter
	int config = params;
	if (encoded)
		config |= EST_URL_ENCODED;
	r += paramsAsString(config) + d; // Index 31

	// Song inserted timestamp
	r += tags.file.insertstamp; // Index 32

	return r;
}

bool TSong::assign(const std::string& text, const char delimiter) {
	if (text.empty())
		return false;

	CTagValues r;
	util::TStringList csv;
	csv.split(text, delimiter);
	if (csv.size() < 31)
		return false;

	// Read params
	bool encoded = false;
	if (csv.size() > 31) {
		params = paramsFromString(csv[31]);
		if (params & EST_URL_ENCODED) {
			encoded = true;
		}
	}

	for (size_t i=0; i<csv.size(); ++i) {
		switch (i) {
			case 1:
				tags.meta.text.artist = decode(csv[i], encoded);
				break;
			case 2:
				tags.meta.text.originalartist = decode(csv[i], encoded);
				break;
			case 3:
				tags.meta.text.albumartist = decode(csv[i], encoded);
				break;
			case 4:
				tags.meta.text.album = decode(csv[i], encoded);
				break;
			case 5:
				tags.meta.text.title = decode(csv[i], encoded);
				break;
			case 6:
				tags.meta.text.genre = decode(csv[i], encoded);
				break;
			case 7:
				tags.meta.text.composer = decode(csv[i], encoded);
				break;
			case 8:
				tags.meta.text.conductor = decode(csv[i], encoded);
				//tags.meta.comment = util::unquote(csv[i]);
				break;
			case 9:
				tags.meta.text.originalalbumartist = decode(csv[i], encoded);
				//tags.meta.description = util::unquote(csv[i]);
				break;
			case 10:
				tags.meta.hash.title = util::unquote(csv[i]);
				break;
			case 11:
				tags.meta.hash.album = util::unquote(csv[i]);
				break;
			case 12:
				r = tagToValues(csv[i], 1);
				tags.meta.track.tracknumber = r.value;
				tags.meta.track.trackcount = r.count;
				tags.meta.text.track = util::cprintf("%02.2d", tags.meta.track.tracknumber);
				break;
			case 13:
				r = tagToValues(csv[i], 0);
				tags.meta.track.disknumber = r.value;
				tags.meta.track.diskcount = r.count;
				tags.meta.text.disk = util::cprintf("%02.2d", tags.meta.track.disknumber);
				break;
			case 14:
				tags.meta.text.year = decode(csv[i], encoded);
				break;
			case 15:
				tags.meta.text.date = decode(csv[i], encoded);
				break;
			case 16:
				tags.stream.sampleCount = (size_t)util::strToInt64(csv[i]);
				break;
			case 17:
				tags.stream.sampleSize = (size_t)util::strToInt64(csv[i]);
				break;
			case 18:
				tags.stream.sampleRate = (size_t)util::strToInt64(csv[i]);
				break;
			case 19:
				tags.stream.bitsPerSample = (size_t)util::strToInt64(csv[i]);
				break;
			case 20:
				tags.stream.bytesPerSample = (size_t)util::strToInt64(csv[i]);
				break;
			case 21:
				tags.stream.channels = (size_t)util::strToInt64(csv[i]);
				break;
			case 22:
				tags.stream.bitRate = (size_t)util::strToInt64(csv[i]);
				break;
			case 23:
				tags.stream.chunkSize = (size_t)util::strToInt64(csv[i]);
				break;
			case 24:
				tags.stream.duration = (size_t)util::strToInt64(csv[i]);
				break;
			case 25:
				tags.stream.seconds = (size_t)util::strToInt64(csv[i]);
				break;
			case 26:
				// Ignore timeToStr(tags.stream.seconds).append(d);
				break;
			case 27:
				tags.file.filename = util::TURL::decode(csv[i]);
				tags.file.basename = util::fileBaseName(tags.file.filename);
				tags.file.extension = util::fileExt(tags.file.filename);
				tags.file.folder = util::filePath(tags.file.filename);
				tags.file.url = util::TURL::encode(tags.file.folder);
				break;
			case 28:
				tags.file.timestamp = csv[i];
				tags.file.time = util::strToDateTime(tags.file.timestamp);
				break;
			case 29:
				tags.file.size = (size_t)util::strToInt64(csv[i]);
				break;
			case 30:
				tags.file.hash = csv[i];
				break;
			case 31:
				params = paramsFromString(csv[i]);
				break;
			case 32:
				tags.file.insertstamp = csv[i];
				tags.file.inserted = util::strToDateTime(tags.file.insertstamp);
				break;
			default:
				break;
		}
	}

	if (tags.stream.isValid() && tags.meta.isValid() && tags.file.isValid()) {
		setSortTags();
		setDisplayTags();
		return true;
	}

	// Invalid tags read from file
	if (!tags.meta.isValid())   std::cout << ">>> Invalid metadata: <" << text << ">" << std::endl;
	if (!tags.stream.isValid()) std::cout << ">>> Invalid stream properties: <" << text << ">" << std::endl;
	if (!tags.file.isValid())   std::cout << ">>> Invalid file properties: <" << text << ">" << std::endl;
	csv.debugOutput();
	tags.file.debugOutput();
	tags.stream.debugOutput();
	tags.meta.debugOutput();

	return false;
}


void TSong::addKeyValue(const std::string& preamble, const std::string& key, const std::string& value, const bool quote, const bool last) {
	std::string separator(",");
	if (last)
		separator.clear();
	if (!value.empty())
		if (quote)
			json.add(preamble + "\"" + key + "\": " + util::quote(value) + separator);
		else
			json.add(preamble + "\"" + key + "\": " + value + separator);
	else
		json.add(preamble + "\"" + key + "\": " + util::JSON_NULL + separator);
}


bool TSong::compare(const std::string& s1, const std::string& s2) const {
	if (s1.size() == s2.size())
		if (s1 == s2)
			return true;
	return false;
}

util::TStringList& TSong::asJSON(std::string preamble, const bool active, const std::string& playlist, const bool extended) {
	if (!json.empty() && active == c_activated && extended == c_extended && compare(playlist, c_playlist))
		return json;

	const CMetaData& meta = tags.meta;
	const CStreamData& stream = tags.stream;
	const CFileData& file = tags.file;

	// Store last state
	json.clear();
	c_activated = active;
	c_playlist = playlist;
	c_extended = extended;

	// Begin new JSON object
	json.add(preamble + "{");
	std::string offs = preamble + "  ";

	// Add artist information
	addKeyValue(offs, "Artist", util::TJsonValue::escape(meta.text.artist), true);
	addKeyValue(offs, "Originalartist", util::TJsonValue::escape(meta.text.originalartist), true);
	addKeyValue(offs, "Albumartist", util::TJsonValue::escape(meta.text.albumartist), true);
	addKeyValue(offs, "Originalalbumartist", util::TJsonValue::escape(meta.text.originalalbumartist), true);

	// Add extended artist information?
	std::string value = meta.display.originalartist;
	if (extended) {
		bool first = true;
		int count = 0;
		if (!(meta.track.compilation || meta.display.albumartist.empty() || meta.compare.albumIsArtist || meta.compare.conductorIsAlbumArtist)) {
			value += (first ? "<i><small><br/>(" : " · ") + meta.display.albumartist;
			first = false;
			++count;
		}
		if (!(meta.display.conductor.empty() || meta.compare.conductorIsArtist)) {
			value += (first ? "<i><small><br/>(" : " · ") + meta.display.conductor;
			first = false;
			++count;
		}
		if (count > 0) {
			value +=  ")</small></i>";
		}
	}
	addKeyValue(offs, "Displayartist", util::TJsonValue::escape(value), true);
	
	// Add title information
	addKeyValue(offs, "Title", util::TJsonValue::escape(meta.display.title), true);
	addKeyValue(offs, "Originaltitle", util::TJsonValue::escape(meta.text.title), true);

	// Add extended title information?
	value = meta.display.title;
	if (extended) {
		if (!(meta.display.composer.empty() || meta.compare.composerIsArtist)) {
			value += "<i><small><br/>(" + meta.display.composer + ")</small></i>";
		}
	}
	addKeyValue(offs, "Displaytitle", util::TJsonValue::escape(value), true);

	// Tag meta data (escape and quote all strings)
	addKeyValue(offs, "Album", util::TJsonValue::escape(meta.display.album), true);
	addKeyValue(offs, "Originalalbum", util::TJsonValue::escape(meta.text.album), true);
	addKeyValue(offs, "Genre", util::TJsonValue::escape(meta.display.genre), true);
	addKeyValue(offs, "Originalgenre", util::TJsonValue::escape(meta.text.genre), true);
	addKeyValue(offs, "Composer", util::TJsonValue::escape(meta.display.composer), true);
	addKeyValue(offs, "Originalcomposer", util::TJsonValue::escape(meta.text.composer), true);
	addKeyValue(offs, "Conductor", util::TJsonValue::escape(meta.display.conductor), true);
	addKeyValue(offs, "Originalconductor", util::TJsonValue::escape(meta.text.conductor), true);

	// Track and album hash data (escape and quote all strings)
	addKeyValue(offs, "Titlehash", util::TJsonValue::escape(meta.hash.title), true);
	addKeyValue(offs, "Albumhash", util::TJsonValue::escape(meta.hash.album), true);

	// Track meta data
	std::string track = meta.display.track;
	if (meta.track.trackcount > 1)
		track += util::cprintf("/%02.2d", meta.track.trackcount);
	if (meta.track.diskcount > 1)
		track += util::cprintf("/%02.2d", meta.track.disknumber);
	addKeyValue(offs, "Track", track, true);
	addKeyValue(offs, "Tracks", util::cprintf("%ld", meta.track.trackcount));
	addKeyValue(offs, "Disk", meta.display.disk, true);
	addKeyValue(offs, "Disks", util::cprintf("%ld", meta.track.diskcount));
	addKeyValue(offs, "Year", meta.display.year, true);
	addKeyValue(offs, "Date", meta.display.date, true);
	addKeyValue(offs, "Compilation", meta.track.compilation ? "true" : "false", false);

    // Stream data
	addKeyValue(offs, "Codec", getCodec(), true);
	addKeyValue(offs, "SampleCount", util::cprintf("%ld", stream.sampleCount));
	addKeyValue(offs, "SampleSize", util::cprintf("%ld", stream.sampleSize));
	addKeyValue(offs, "SampleRate", util::cprintf("%ld", stream.sampleRate));
	addKeyValue(offs, "BitsPerSample", util::cprintf("%ld", stream.bitsPerSample));
	addKeyValue(offs, "BytesPerSample", util::cprintf("%ld", stream.bytesPerSample));
	addKeyValue(offs, "Channels", util::cprintf("%ld", stream.channels));
	addKeyValue(offs, "BitRate", util::cprintf("%ld", stream.bitRate));
	addKeyValue(offs, "ChunkSize", util::cprintf("%ld", stream.chunkSize));
	addKeyValue(offs, "Duration", util::cprintf("%ld", stream.duration));
	addKeyValue(offs, "Seconds", util::cprintf("%ld", stream.seconds));
	addKeyValue(offs, "Time", timeToStr(stream.seconds), true);

	// File properties
	addKeyValue(offs, "Filename", file.filename, true);
	addKeyValue(offs, "Basename", file.basename, true);
	addKeyValue(offs, "Extension", file.extension, true);
	addKeyValue(offs, "Folder", file.folder, true);
	addKeyValue(offs, "URL", file.url, true);
	addKeyValue(offs, "Filetime", file.timestamp, true);
	addKeyValue(offs, "Filesize", util::cprintf("%ld", file.size));
	addKeyValue(offs, "Filehash", file.hash, true);

	// File scanner configuration data
	addKeyValue(offs, "Configuration", paramsAsString(params), true);

	// Name of playlist where JSON data is requested for (or "null")
	addKeyValue(offs, "Playlist", playlist, true);

	// Is streamable MIME type
	addKeyValue(offs, "Streamable", isStreamable() ? "true" : "false", false);

	// Is song active or currently played song
	addKeyValue(offs, "Active", active ? "true" : "false", false, true);

	// Close JSON object
	json.add(preamble + "}");

	return json;
}

util::TStringList& TSong::asM3U(const std::string& webroot, const bool isHTTP) {
	if (!m3u.empty())
		return m3u;

	// Add song metadata and URL
	// #EXTINF:221,Queen - Bohemian Rhapsody
	m3u.add("#EXTINF:" + util::cprintf("%ld", tags.stream.seconds) + "," + tags.meta.text.artist + " - " + tags.meta.text.title + " - " + tags.meta.text.album);
	if (isHTTP) {
		m3u.add(webroot + "fs0" + tags.file.url + util::TURL::encode(util::fileExtName(tags.file.filename)));
	} else {
		m3u.add(webroot + tags.file.filename);
	}
	m3u.add("");

	return m3u;
}

void TSong::reset() {
	clearStatsistics();
	setBuffered(false);
}

void TSong::clear() {
	tags.clear();
	valid = false;
	invalidate();
}

void TSong::cleanup() {
	// Cleanup song to be played again
	resetStatsistics();
	//setBuffered(true);
}

void TSong::invalidate() {
	if (!json.empty()) json.clear();
}

bool TSong::filter(const std::string& filter, EFilterType type) {
	switch (type) {
		case FT_ALBUM:
			return isAlbum(filter);
			break;
		case FT_ARTIST:
			return isArtist(filter);
			break;
		case FT_STRING:
		default:
			return isString(filter);
			break;
	}
	return false;
}

bool TSong::isString(const std::string& filter) {
	if (tags.meta.track.compilation) {
		return util::strcasestr(tags.meta.text.originalalbumartist, filter) ||
				util::strcasestr(tags.meta.text.album, filter) ||
				util::strcasestr(tags.meta.text.title, filter);
	} else {
		return util::strcasestr(tags.meta.text.albumartist, filter) ||
				util::strcasestr(tags.meta.text.album, filter) ||
				util::strcasestr(tags.meta.text.title, filter);
	}
}

bool TSong::isArtist(const std::string& artist) {
	if (tags.meta.track.compilation) {
		return util::strcasestr(tags.meta.text.originalalbumartist, artist);
	} else {
		return util::strcasestr(tags.meta.text.albumartist, artist);
	}
}

bool TSong::isAlbum(const std::string& hash) {
	return (hash == tags.meta.hash.album);
}

TSong& TSong::operator = (const TSong& song) {
	invalidate();

	type    = song.type;
	media   = song.media;
	icon    = song.icon;
	modtime = song.modtime;
	params  = song.params;
	streamable = song.streamable;

	index      = -1;
	deleted    = false;
	loaded     = false;
	buffered   = false;

	c_activated  = false;
	c_playlist.clear();

	tags  = song.tags;
	valid = tags.isValid();

	return *this;
}

} /* namespace music */
