/*
 * alac.cpp
 *
 *  Created on: 01.10.2016
 *      Author: Dirk Brinkmeier
 */

#include "gcc.h"
#include "endian.h"
#include "bitmap.h"
#include "compare.h"
#include "exception.h"
#include "fileutils.h"
#include "fileconsts.h"
#include "audiobuffer.h"
#include "audioconsts.h"
#include "audiotypes.h"
#include "audiofile.h"
#include "id3genres.h"
#include "alac.h"

#include "alac/ALACDecoder.h"
#include "alac/aglib.h"

namespace music {


TALACAtomScanner::TALACAtomScanner() {
	prime();
}

TALACAtomScanner::~TALACAtomScanner() {
}

void TALACAtomScanner::prime() {
	defaultSize = util::SCAN_BUFFER_SIZE;
	bufferSize = defaultSize;
	debug = false;
	defItem.ID = "<invalid>";
	defItem.offset = std::string::npos;
	defItem.data  = std::string::npos;
	defItem.size = std::string::npos;
	seekPos = 0;
	codec = EFT_UNKNOWN;
}

void TALACAtomScanner::clear() {
	atoms.clear();
	tags.clear();
	artwork.clear();
	bufferSize = defaultSize;
	seekPos = 0;
}


const TALACAtom& TALACAtomScanner::addAtom(const std::string& ID, size_t offset, size_t size) {
	const_iterator it = atoms.find(ID);
	if (it == end()) {
		TALACAtom atom;
		atom.ID = ID;
		atom.offset = offset;
		atom.data = offset + sizeof(TChunkSize) + sizeof(TChunkID);
		atom.size = size;
		atom.hash = 0;
		if (ID.size() > 3)
			atom.hash = MAKE_ATOM(ID[0], ID[1], ID[2], ID[3]);
		if (debug) {
			std::cout << "TAlacAtomScanner::addAtom() ID \"" << atom.ID << "\"" << std::endl;
			std::cout << "TAlacAtomScanner::addAtom() Offset = " << atom.offset << std::endl;
			std::cout << "TAlacAtomScanner::addAtom() Data   = " << atom.data << std::endl;
			std::cout << "TAlacAtomScanner::addAtom() Size   = " << atom.size << std::endl;
			std::cout << "TAlacAtomScanner::addAtom() Hash   = " << atom.hash << std::endl;
		}
		atoms[ID] = atom;
		return find(ID);
	}
	return defItem;
}

bool TALACAtomScanner::addTag(const DWORD hash, const std::string& value) {
	TTagMap::const_iterator it = tags.find(hash);
	if (it == tags.end()) {
		if (debug) {
			std::cout << "TAlacAtomScanner::addTag() Hash = " << hash << std::endl;
			std::cout << "TAlacAtomScanner::addTag() Value = \"" << value << "\"" << std::endl;
		}
		tags[hash] = value;
		return true;
	}
	return false;
}

bool TALACAtomScanner::addAtoms(const util::TFile& file, const std::string& ID, size_t offset, size_t size) {
//	std::cout << "TAlacAtomScanner::addAtoms() ID \"" << ID << "\"" << std::endl;
	TALACAtom atom = addAtom(ID, offset, size);
	if (hasChilds(atom))
		scanSubChunks(file, atom);
	return (atom.offset != std::string::npos);
}


bool TALACAtomScanner::hasChilds(const TALACAtom& atom) const {
	return hasChilds(atom.hash);
}

bool TALACAtomScanner::hasChilds(const DWORD hash) const {
	DWORD h;
	size_t i = 0;
	do {
		h = ALAC_HAS_CHILD_ATOMS[i];
		if (h == hash)
			return true;
		++i;
	} while (h > 0);
	return false;
}

bool TALACAtomScanner::validTagID(const DWORD hash) const {
	DWORD h;
	size_t i = 0;
	do {
		h = ALAC_TAG_ATOMS[i];
		if (h == hash)
			return true;
		++i;
	} while (h > 0);
	return false;
}

void TALACAtomScanner::getFlavour(const util::TFile& file, ECodecType& type) {
	type = EFT_UNKNOWN;
	const_iterator it = atoms.find("stsd");
	if (it != end()) {
		if (it->second.size > 16) {
			util::TBuffer data(4);
			ssize_t r = file.read(data.data(), data.size(), it->second.data + 12);
			if (r == (ssize_t)data.size()) {
				if (0 == util::strncasecmp(data.data(), "alac", data.size())) {
					type = EFT_ALAC;
				} else {
					if (0 == util::strncasecmp(data.data(), "mp4a", data.size())) {
						type = EFT_AAC;
					} else {
						if (debug) std::cout << "TALACAtomScanner::getFlavour() Invalid type identifier \"" << util::TBinaryConvert::binToAsciiA(data.data(), data.size()) << "\"" << std::endl;
					}
				}
			} else {
				if (debug) std::cout << "TALACAtomScanner::getFlavour() Atom STSD could not be read." << std::endl;
			}
		} else {
			if (debug) std::cout << "TALACAtomScanner::getFlavour() Atom STSD too small." << std::endl;
		}
	} else {
		if (debug) std::cout << "TALACAtomScanner::getFlavour() Atom STSD not found." << std::endl;
	}
}

std::string TALACAtomScanner::hashTagToStr(const DWORD hash) const {
	return std::string((char*)&hash, sizeof(DWORD));
}


const TALACAtom& TALACAtomScanner::find(const std::string& ID) const {
	const_iterator it = atoms.find(ID);
	if (it != end())
		return it->second;
	return defItem;
}

const TALACAtom& TALACAtomScanner::operator[] (const std::string& ID) const {
	return find(ID);
}


ssize_t TALACAtomScanner::scan(const std::string& fileName, const ETagLoaderType mode) {
	if (!util::fileExists(fileName))
		return (ssize_t)-3;

	util::TFile file(fileName);
	file.open(O_RDONLY);
	return scan(file, mode);
}

ssize_t TALACAtomScanner::scan(util::TFile& file, const ETagLoaderType mode) {
	clear();
	return scanner(file, mode);
}

ssize_t TALACAtomScanner::scanner(util::TFile& file, const ETagLoaderType mode) {
	TAlacFileHeader header;
	size_t hsize = sizeof(TAlacFileHeader);
	size_t msize = sizeof(TAlacAtomHeader);
	size_t csize = sizeof(TChunkSize);

	if (!file.isOpen())
		return (ssize_t)-4;

	if (file.getSize() <= hsize)
		return (ssize_t)-5;

	// Read ALAC/M4A type header
	if (!readHeader(file, 0, &header, hsize))
		return (ssize_t)-6;

	// Check for valid file header
	header.alChunkSize = convertFromBigEndian32(header.alChunkSize);
	if(!isAlacFile(header))
		return (ssize_t)-7;

	// Find next "moov" chunk after ALAC file header
	util::TFileScanner parser;
	app::TStringVector pattern = {"moov"};
	parser.scan(file, header.alChunkSize, pattern, 512);

	// "moov" header found?
	size_t offset = parser["moov"];
	if (std::string::npos == offset)
		return (ssize_t)-8;
	if (offset < csize + header.alChunkSize)
		return (ssize_t)-9;

	// Read "moov" header
	offset -= csize;
	TAlacAtomHeader moov;
	if (!readHeader(file, offset, &moov, msize))
		return (ssize_t)-19;

	// Check for valid "moov" header
	moov.alChunkSize = convertFromBigEndian32(moov.alChunkSize);
	if (moov.alChunkSize <= 0)
		return (ssize_t)-11;
	if (!isChunkID(moov.alChunkID, "moov"))
		return (ssize_t)-12;

	// Add 'moov' chunk and sub atoms to list
	addAtoms(file, "moov", offset, moov.alChunkSize);

	// Iterate through chunks/atoms up from 'moov'
	ssize_t r = scanChunks(file, offset + moov.alChunkSize);
	if (r < 0)
		return r;

	// Scan sub chunks/atoms, obsolete by recursive scan!
	//  scanSubChunks(file, "moov");
	//  scanSubChunks(file, "trak");
	//  scanSubChunks(file, "mdia");
	//  scanSubChunks(file, "minf");
	//  scanSubChunks(file, "stbl");
	//  scanSubChunks(file, "udta");

	// Check for ALAC vs. AAC
	if (codec != EFT_AAC) {
		ECodecType type;
		getFlavour(file, type);
		if (type == EFT_ALAC || type == EFT_AAC)
			codec = type;
		if (codec == EFT_AAC)
			std::cout << "TALACAtomScanner::scanner() Lossy AAC file detected <" << file.getName() << ">" << std::endl;
	}

	// Scan for metadat tags
	r = scanMetaData(file, mode);
	if (r < 0)
		return r;

	return (ssize_t)atoms.size();
}


ssize_t TALACAtomScanner::scanChunks(const util::TFile& file, const size_t offset) {
	ssize_t retVal = 0;
	bool eof = offset <= 0;
	size_t position = offset;
	TAlacAtomHeader header;
	size_t hsize = sizeof(TAlacAtomHeader);
	size_t csize = sizeof(TChunkID);

	while (!eof) {

		// Read header at given file position
		if (!readHeader(file, position, &header, hsize))
			return (ssize_t)-21;

		// Add header at current position to map
		header.alChunkSize = convertFromBigEndian32(header.alChunkSize);
		if (header.alChunkID[0] != '\0' && header.alChunkSize > 0) {

			std::string s(header.alChunkID, csize);
			addAtoms(file, s, position, header.alChunkSize);
			++retVal;

			// Goto next atom chunk
			position += header.alChunkSize;

			// Check for EOF
			eof = (position + 1) > file.getSize();

		} else {
			// Invalid header
			eof = true;
		}

	}
	return retVal;
}


ssize_t TALACAtomScanner::scanSubChunks(const util::TFile& file, const std::string& ID) {

	// Find atom in map
	TALACAtom atom = find(ID);
	if (atom.offset == std::string::npos)
		return (ssize_t)-31;

	return scanSubChunks(file, atom);
}

ssize_t TALACAtomScanner::scanSubChunks(const util::TFile& file, const TALACAtom& atom) {
	ssize_t retVal = 0;
	if (debug) {
		std::cout << "TAlacAtomScanner::scanSubChunks() ID \"" << atom.ID << "\"" << std::endl;
		std::cout << "TAlacAtomScanner::scanSubChunks() Offset = " << atom.offset << std::endl;
		std::cout << "TAlacAtomScanner::scanSubChunks() Data   = " << atom.data << std::endl;
		std::cout << "TAlacAtomScanner::scanSubChunks() Size   = " << atom.size << std::endl;
		std::cout << "TAlacAtomScanner::scanSubChunks() Hash   = " << atom.hash << std::endl;
	}
	TAlacAtomHeader header;
	//size_t csize = sizeof(TChunkSize) + sizeof(TChunkID);
	size_t hsize = sizeof(TAlacAtomHeader);
	size_t position = atom.data; // atom.offset + csize;
	size_t last = atom.offset + atom.size - 1;
	bool eof = false;

	while (!eof) {

		// Read header at given file position
		if (!readHeader(file, position, &header, hsize))
			return (ssize_t)-22;

		// Add header at current position to map
		header.alChunkSize = convertFromBigEndian32(header.alChunkSize);
		if (header.alChunkID[0] != '\0' && header.alChunkSize > 0) {

			std::string s(header.alChunkID, sizeof(TChunkSize));
			addAtoms(file, s, position, header.alChunkSize);
			++retVal;

			// Goto next atom chunk
			position += header.alChunkSize;

			// Check for EOF
			eof = position > last;

		} else {
			// Invalid header
			eof = true;
		}

	}
	return retVal;
}


ssize_t TALACAtomScanner::scanMetaData(const util::TFile& file, const ETagLoaderType mode) {
	ssize_t retVal = 0;

	// Find meta information atom in map
	TALACAtom atom = find("meta");
	if (atom.offset == std::string::npos)
		return (ssize_t)-41;

	// Read complete metadata chunk from file
	//size_t offset = atom.offset + sizeof(TChunkSize) + sizeof(TChunkID);
	size_t offset = atom.data;
	util::TBuffer buffer(atom.size + 1);
	if (!seek(file, offset))
		return (ssize_t)-42;
	if (!read(file, buffer.data(), atom.size))
		return (ssize_t)-43;

	// Find start of meta data up from atom 'ilst' in chunk
	// Return 0 on missing tag metadata!
	char* p = findAtom("ilst", buffer.data(), buffer.size());
	if (!util::assigned(p))
		return (ssize_t)0;

	// Metadata starts behind 'ilst'
	p += sizeof(TChunkID);
	retVal = scanTags(p, buffer.size() - (p - buffer.data()) - 1, mode);

	return retVal;
}

ssize_t TALACAtomScanner::scanTags(const char *const buffer, const size_t size, const ETagLoaderType mode) {
	ssize_t retVal = 0;
	size_t offset = 0;
	ssize_t ssize;
	size_t length;
	size_t hsize = sizeof(TAlacTagChunk);
	size_t dsize = 4 * sizeof(DWORD);
	TAlacTagChunk header;
	bool finished = false;
	const char* p;
	bool readTags = mode == ETL_ALL || mode == ETL_METADATA;
	bool readPics = mode == ETL_ALL || mode == ETL_PICTURE;

//	std::cout << "TAlacAtomScanner::scanTags() Size = " << size << std::endl;

	do {
		// Copy tag header from buffer
		copyHeader(buffer, offset, &header, hsize);
		header.alChunkSize = convertFromBigEndian32(header.alChunkSize);
		header.alDataSize = convertFromBigEndian32(header.alDataSize);

//		std::cout << "TAlacAtomScanner::scanTags() ID = " << header.alChunkID << std::endl;
//		std::cout << "TAlacAtomScanner::scanTags() Offset = " << offset << std::endl;
//		std::cout << "TAlacAtomScanner::scanTags() Chunk size = " << header.alChunkSize << std::endl;
//		std::cout << "TAlacAtomScanner::scanTags() Data size  = " << header.alDataSize << std::endl;

		// Check for valid data chunk
		if ((util::isMemberOf(header.alDataID, ALAC_TAG_ATOM_1,ALAC_TAG_ATOM_2)) && \
			header.alDataSize > 0 && header.alChunkSize > header.alDataSize && header.alDataSize > dsize) {

			// Is tag type supported?
			if (validTagID(header.alChunkID)) {

				// Set correct size for tag value
				ssize = header.alDataSize - dsize;
				if (ssize > 0) {
					length = (size_t)ssize;

					// Read tag ID
					p = buffer + offset + sizeof(TChunkSize);

					if (readTags) {
						std::string value;
						BYTE b;

						// Check entry types
						switch (header.alChunkID) {
							case ALAC_GENRE_ATOM:
								// Read last byte in buffer as ID3 genre
								// ALAC uses genre values up from 1, not from 0 as native ID3 genres
								b = buffer[offset + hsize + length - 1];
								value = ID3_GENRE_NAME(b - 1);
								break;

							case ALAC_COMPILATION_ATOM:
								// Read last byte in buffer as numerical value
								b = buffer[offset + hsize + length - 1];
								value = util::csnprintf("%", (int)b);
								break;

							case ALAC_TRACK_ATOM:
							case ALAC_DISK_ATOM:
								// Read last byte in buffer as numerical value
								b = buffer[offset + hsize - 1 + 4];
								value = util::csnprintf("%", (int)b);
								break;

							case ALAC_COVER_ART_ATOM:
								// Ignore cover art here...
								break;

							default:
								p = buffer + offset + hsize;
								trim(p, length);
								if (isBOM(p, length)) {
									// Read tag value as UTF-16 string
									value = UTF16ToMultiByteStr(p, length);
								} else {
									// Read tag value as UTF-8 string
									value = UTF8ToMultiByteStr(p, length);
								}
								break;
						}

						// Add entry to map of tags
						if (!value.empty()) {
							addTag(header.alChunkID, value);
							++retVal;
						}
					}

					if (readPics) {
						if (header.alChunkID == ALAC_COVER_ART_ATOM) {
							// Read picture buffer
							artwork.assign(p, length);
							++retVal;
							if (mode == ETL_PICTURE) {
								finished = true;
								break;
							}
						}
					}

				}
			}
		} else {
			finished = true;
		}

		// Goto next tag entry...
		offset += header.alChunkSize;

		// All tags parsed?
		if (!finished)
			finished = offset >= size;

	} while (!finished);

	return retVal;
}

ssize_t TALACAtomScanner::getTags(CMetaData& tag) {
	ssize_t retVal = 0;
	if (!tags.empty()) {
		TTagMap::const_iterator it = tags.begin();
		music::CTagValues r;
		bool found;
		do {
			found = false;

			if (!found && it->first == ALAC_ARTIST_ATOM) {
				tag.text.artist = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_ALBUMARTIST_ATOM) {
				tag.text.albumartist = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_PERFORMER_ATOM) {
				tag.text.conductor = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_COMPOSER_ATOM) {
				tag.text.composer = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_ALBUM_ATOM) {
				tag.text.album = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_TITLE_ATOM) {
				tag.text.title = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_GENRE_ATOM) {
				tag.text.genre = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_CUST_GENRE_ATOM && tag.text.genre.empty()) {
				tag.text.genre = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_DAY_ATOM) {
				int year = util::strToInt(it->second, 1900);
				if (year > 0)
					tag.text.year = util::cprintf("%04.4d", year);
				if (tag.text.year.empty())
					tag.text.year = "1900";
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_TRACK_ATOM) {
				r = tagToValues(it->second, 1);
				tag.track.tracknumber = r.value;
				tag.track.trackcount = r.count;
				tag.text.track = util::cprintf("%02.2d", tag.track.tracknumber);
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_DISK_ATOM) {
				r = tagToValues(it->second, 0);
				tag.track.disknumber = r.value;
				tag.track.diskcount = r.count;
				tag.text.disk = util::cprintf("%02.2d", tag.track.disknumber);
				found = true;
				++retVal;
			}

			if (!found && it->first == ALAC_COMPILATION_ATOM) {
				int val = util::strToInt(it->second);
				if (val > 0)
					tag.track.compilation = true;
				found = true;
				++retVal;
			}

			// All tags read?
			if (retVal >= 9)
				break;

			// Next in list....
			++it;

		} while (it != tags.end());

		// Check for Album Artist a.k.a. Group
		if (tag.text.albumartist.empty() && !tag.text.artist.empty())
			tag.text.albumartist = tag.text.artist;

	}
    return retVal;
}


bool TALACAtomScanner::getPicture(CCoverData& cover) {
	cover.artwork.clear();
	if (!artwork.empty()) {
		cover.artwork = artwork;
	}
	return !cover.artwork.empty();
}


char* TALACAtomScanner::findAtom(const std::string& ID, char* buffer, size_t size) const {
	if (ID.size() > 0) {
		char* p;
		for (size_t i=0; i<size; ++i) {
			if (ID[0] == buffer[i]) {
				p = util::strnstr(buffer + i, ID.c_str(), ID.size());
				if (util::assigned(p)) {
					return p;
				}
			}
		}
	}
	return nil;
}

const char *const TALACAtomScanner::copyHeader(const char *const buffer, const size_t offset, void *const header, const size_t size) const {
	const char *const src = buffer + offset;
	memcpy(header, src, size);
	return src;
}


bool TALACAtomScanner::readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size) {
	if (seek(file, offset)) {
		return read(file, buffer, size);
	}
	return false;
}

size_t TALACAtomScanner::getBlockLength(const void *const buffer) {
	uint8_t const *p = (uint8_t const *)buffer;
	size_t length = 0;
	size_t size = 0;
	uint8_t b;
    do {
		b = *p++;
		length = (length << 7) | (length & 0x7F);
		if (++size > (ssize_t)ALAC_MAX_BER_SIZE) {
			return (size_t)0;
		}
	} while(((b & 0x80) != 0) && (size <= 4));

    // Return block length
	return length;
}

bool TALACAtomScanner::seek(const util::TFile& file, const size_t offset) {
	if (offset != std::string::npos && offset < file.getSize()) {
		if (offset != seekPos) {
			ssize_t r = file.seek(offset, util::SO_FROM_START);
			if (r == (ssize_t)offset) {
				seekPos = offset;
				return true;
			}
			seekPos = 0;
		} else {
			// Same offset as before!
			return true;
		}
	}
	return false;
}

bool TALACAtomScanner::read(const util::TFile& file, void *const data, const size_t size) {
	ssize_t r = file.read((char*)data, size, 0, util::SO_FROM_START);
	if (r == (ssize_t)size) {
		seekPos += size;
		return true;
	}
	return false;
}


bool TALACAtomScanner::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

bool TALACAtomScanner::isAlacFile(const TAlacFileHeader& header) const {

	// Check "magic" header bytes
	if (!isChunkID(header.alChunkID, "ftyp"))
		return false;
	if (!isChunkID(header.alTypeID, "M4A "))
		return false;

	// Check for valid chunk sizes
	if (header.alChunkSize <= 0)
		return false;

	return true;
}

void TALACAtomScanner::debugOutputChunks(const std::string& preamble) const {
	const_iterator it = begin();
	if (!atoms.empty()) {
		while (it != end()) {
			std::cout << preamble << "ID \"" << it->second.ID << "\"" << std::endl;
			std::cout << preamble << "Offset = " << it->second.offset << std::endl;
			std::cout << preamble << "Data   = " << it->second.data << std::endl;
			std::cout << preamble << "Size   = " << it->second.size << std::endl;
			std::cout << std::endl;
			++it;
		}
	} else
		std::cout << preamble << "No pattern found." << std::endl;
}

void TALACAtomScanner::debugOutputTags(const std::string& preamble) const {
	TTagMap::const_iterator it = tags.begin();
	if (!tags.empty()) {
		while (it != tags.end()) {
			std::cout << preamble << "ID \"" << hashTagToStr(it->first) << "\"" << std::endl;
			std::cout << preamble << "Hash = " << it->first << std::endl;
			std::cout << preamble << "Value \"" << it->second << "\"" << std::endl;
			std::cout << std::endl;
			++it;
		}
	} else
		std::cout << preamble << "No tags found." << std::endl;
}



TALACStream::TALACStream() {
}

TALACStream::~TALACStream() {
}

void TALACStream::setFileHeaderEndian(TAlacFileHeader& header) const {
	//	typedef struct CAlacFileHeader {
	//		TChunkSize	alChunkSize;	// Offset 0
	//		TChunkID	alChunkID;		// Offset 4  "ftyp"
	//		TTypeID		alTypeID;		// Offset 8  "M4A "
	//	} PACKED TAlacFileHeader;
#ifdef TARGET_LITTLE_ENDIAN
	header.alChunkSize = convertFromBigEndian32(header.alChunkSize);
#endif
}

void TALACStream::setStreamChunkEndian(TAlacStreamChunk& stream) const {
	//	typedef struct CAlacStreamChunk {
	//		TChunkSize	alChunkSize;		// Offset 0
	//		TChunkID	alChunkID;			// Offset 4  "stsd"
	//		DWORD		alVersionFlag;		// Offset 8  = 0
	//		DWORD		alEntryCount;		// Offset 12 = 1
	//		DWORD		alSampleEntrySize; 	// Offset 16
	//		TTypeID		alEntryType;  		// Offset 20 "alac"
	//		BYTE		alReserved1[7];		// Offset 24
	//		BYTE		alReferenceIndex;	// Offset 31 = 1
	//		BYTE		alReserved2[8];		// Offset 32
	//		WORD		alNumChannels;		// Offset 40
	//		WORD		alSampleSize;		// Offset 42
	//		BYTE		alReserved3[2];		// Offset 44
	//		DWORD		alSampleRate;		// Offset 48
	//	} PACKED TAlacStreamChunk;
#ifdef TARGET_LITTLE_ENDIAN
	stream.alChunkSize = convertFromBigEndian32(stream.alChunkSize);
	stream.alVersionFlag = convertFromBigEndian32(stream.alVersionFlag);
	stream.alEntryCount = convertFromBigEndian32(stream.alEntryCount);
	stream.alSampleEntrySize = convertFromBigEndian32(stream.alSampleEntrySize);
	stream.alNumChannels = convertFromBigEndian16(stream.alNumChannels);
	stream.alSampleSize = convertFromBigEndian16(stream.alSampleSize);
	stream.alSampleRate = convertFromBigEndian32(stream.alSampleRate);
#endif
}

void TALACStream::setMediaChunkEndian(TAlacMediaChunk& media) const {
	//	typedef struct CAlacMediaChunk {
	//		TChunkSize	alChunkSize;		// Offset 0
	//		TChunkID	alChunkID;			// Offset 4  "mdhd"
	//		DWORD		alVersionFlag;		// Offset 8  = 0
	//		BYTE		alReserved[8];		// Offset 12
	//		DWORD		alSampleRate;		// Offset 20
	//		DWORD		alNumFrames;		// Offset 24 --> Duration = alNumFrames / alSampleRate
	//	} PACKED TAlacMediaChunk;
#ifdef TARGET_LITTLE_ENDIAN
	media.alChunkSize = convertFromBigEndian32(media.alChunkSize);
	media.alVersionFlag = convertFromBigEndian32(media.alVersionFlag);
	media.alSampleRate = convertFromBigEndian32(media.alSampleRate);
	media.alNumFrames = convertFromBigEndian32(media.alNumFrames);
#endif
}




TAACDecoder::TAACDecoder() {
	prime();
}

TAACDecoder::~TAACDecoder() {
	freeDecoder();
}

void TAACDecoder::prime() {
	TAudioStream::prime();
	TAudioStream::clear();
	debug = true;
	decoder = nil;
	clear();
}

void TAACDecoder::clear() {
	memset(&info, 0, sizeof(NeAACDecFrameInfo));
	inputBuffer.clear();
	fileOffset = 0;
	lastOffset = 0;
	readOffset = 0;
	bufferSize = 0;
	decodedSize = 0;
}

bool TAACDecoder::createDecoder() {
	freeDecoder();

	// Calculate buffer sizes
	stream.chunkSize = 16536;
	readBufferSize   = stream.chunkSize;
	inputBufferSize  = stream.chunkSize / 16;

	// Create new decoder
	decoder = NeAACDecOpen();
	if (!util::assigned(decoder))
		return false;

	// Get the current config
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoder);

	// Set default decoder parameters
	conf->defObjectType = LC;
	switch (stream.bitsPerSample) {
		case 32:
			if (debug) std::cout << "TAACDecoder::createDecoder() Decode 32 bit." << std::endl;
			conf->outputFormat = FAAD_FMT_32BIT;
			break;
		case 24:
			if (debug) std::cout << "TAACDecoder::createDecoder() Decode 24 bit." << std::endl;
			conf->outputFormat = FAAD_FMT_24BIT;
			break;
		case 16:
			if (debug) std::cout << "TAACDecoder::createDecoder() Decode 16 bit." << std::endl;
			conf->outputFormat = FAAD_FMT_16BIT;
			break;
		default:
			if (debug) std::cout << "TAACDecoder::createDecoder() Decode 16 bit (" << stream.bitsPerSample << ")" << std::endl;
			conf->outputFormat = FAAD_FMT_16BIT;
			break;
	}
	conf->dontUpSampleImplicitSBR = 0;
	conf->useOldADTSFormat = 0;
	conf->downMatrix = 0;

	// Set the new configuration
	NeAACDecSetConfiguration(decoder, conf);

	// Setup buffers
	inputBuffer.resize(inputBufferSize);
	if (debug) {
		std::cout << "TAACDecoder::createDecoder() Stream chunk size = " << stream.chunkSize << std::endl;
		std::cout << "TAACDecoder::createDecoder() Input buffer size = " << inputBufferSize << std::endl;
	}

	return true;
}

void TAACDecoder::freeDecoder() {
	if (util::assigned(decoder))
		NeAACDecClose(decoder);
	decoder = nil;
	clear();
}


bool TAACDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TAACDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TAACDecoder::open(const PSong song) {
	if (util::assigned(song))
		return open(*song);
	return false;
}

bool TAACDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TAACDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {

				// Initialize decoder
				setStream(properties);
				if (createDecoder()) {

					// Parse file to find data chunk
					size_t offset = 0;
					fileOffset = 0;
					TALACAtomScanner parser;
					parser.setCodec(EFT_AAC);
					parser.setDebug(debug);
					ssize_t r = parser.scan(file, ETL_METADATA);
					if (r > 0) {
						// Set offset to first data byte after data chunk file header
						TALACAtom atom = parser["mdat"];
						if (atom.data != std::string::npos) {
							// Skip 'mdat' header + DWORD for version flags and 4 reserved bytes
							offset = atom.offset + sizeof(TChunkSize) + sizeof(TChunkID);
							//offset = atom.data;
						}

//						// Read MP4 specific configuration
//						mp4AudioSpecificConfig mp4;
//						int8_t res1 = NeAACDecAudioSpecificConfig(input, bufferSize, &mp4);
//						if (EXIT_SUCCESS != res1) {
//							if (debug) std::cout << util::csnprintf("TAACDecoder::update() Reading MP4 specific configuration failed (%)", res1) << std::endl;
//							throw util::app_error_fmt("TAACDecoder::update() Reading MP4 specific configuration failed (%)", res1);
//						}
//
						// Seek to first byte after 'mdat' header
						if (offset > 0) {
							r = file.seek(offset);
							if (r == (ssize_t)offset) {

								// Set stream properties
								std::string s;
								unsigned long samplerate = 0; //stream.sampleRate;
								unsigned char channels = 0; //stream.channels;

								// Read first AAC block of data
								size_t readSize = inputBufferSize;
								r = read(file, inputBuffer, readSize);
								if (r == (ssize_t)readSize) {

									// Remember bytes read from file in buffer
									bufferSize = r;

									// Initialize decoder
									ssize_t bytesRead = NeAACDecInit(decoder, inputBuffer.data(), r, &samplerate, &channels);
									if (bytesRead >= 0) {

										// Check for valid decoder parameters
										if (samplerate != (unsigned long)stream.sampleRate) {
											s = util::csnprintf("TAACDecoder::open() AAC decoder reported wrong samplerate (%/%)", samplerate, stream.sampleRate);
											error = true;
										}
										if (channels != (unsigned char)stream.channels) {
											s = util::csnprintf("TAACDecoder::open() AAC decoder reported wrong channel count (%/%)", channels, stream.channels);
											error = true;
										}
										if (debug) {
											std::cout << "TAACDecoder::open() AAC samplerate = " << (int)samplerate << std::endl;
											std::cout << "TAACDecoder::open() AAC channels   = " << (int)channels << std::endl;
										}

										// Set file offset to next read chunk
										fileOffset = offset + bytesRead;
										readOffset = bytesRead;
										if ((ssize_t)bufferSize > bytesRead)
											bufferSize -= bytesRead;

										if (debug) {
											util::hexout("TAACDecoder::open() File offset = ", fileOffset);
											std::cout << "TAACDecoder::open() Bytes read  = " << bytesRead << std::endl;
											std::cout << "TAACDecoder::open() Read offset = " << readOffset << std::endl;
											std::cout << "TAACDecoder::open() Buffer size = " << bufferSize << std::endl;
										}

									} else {
										s = util::csnprintf("TAACDecoder::open() AAC decoder initialization failed (%)", bytesRead);
										error = true;
									}
								} else {
									s = util::csnprintf("TAACDecoder::open() Insufficient file size (%/%)", r, readBufferSize);
									error = true;
								}
								if (error) {
									// Be responsive...
									std::cout << s << std::endl;
								}

							}
						}

					}

				} else {
					error = true;
				}

				if (fileOffset <= 0 || fileOffset > file.getSize())
					error = true;

				// Close file on error!
				if (error) {
					opened = false;
					file.close();
				}
			}
			if (!opened) {
				freeDecoder();
			}
			if (debug) {
				stream.debugOutput();
			}
			lastOffset = fileOffset;
			return opened;
		}
	}
	setRead(0);
	lastOffset = fileOffset;
	return false;
}


bool TAACDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			PSample input = inputBuffer.data() + readOffset;
			void* samples = (void*)buffer->writer();
			ssize_t bytesRead;
			size_t bytesDecoded;
			ssize_t r;

			// Decoder may throw exception...
			try {
				if (debug) {
					std::cout << "TAACDecoder::open() Buffer : " << util::TBinaryConvert::binToHexA(input, 16) << std::endl;
					std::cout << "TAACDecoder::open() Read offset = " << readOffset << std::endl;
					std::cout << "TAACDecoder::open() Input buffer size = " << bufferSize << std::endl;
					std::cout << "TAACDecoder::open() Read buffer size  = " << readBufferSize << std::endl;
				}

				// Decode next frame in buffer
				void* res3 = NeAACDecDecode2(decoder, &info, input, bufferSize, &samples, readBufferSize);
				if (!util::assigned(res3) || EXIT_SUCCESS != info.error) {
					std::string s = "Unexpected decoder error";
					if (info.error > 0) {
						const char* p = NeAACDecGetErrorMessage(info.error);
						if (util::assigned(p))
							s = std::string(p);
					}
					if (debug) std::cout << util::csnprintf("TAACDecoder::update() Decoding input buffer failed: $", s) << std::endl;
					throw util::app_error_fmt("TAACDecoder::update() Decoding input buffer failed: $", s);
				}

				// Check for valid decoded bytes
				bytesDecoded = info.bytesconsumed;
				if (info.samples == 0) {
					if (debug) std::cout << "TAACDecoder::update() Decoding input buffer failed, no samples decoded." << std::endl;
					throw util::app_error("TAACDecoder::update() Decoding input buffer failed, no samples decoded.");
				}

				// Check if all bytes decoded
				if (debug) {
					std::cout << "TAACDecoder::update() Consumed " << bytesDecoded << std::endl;
					std::cout << "TAACDecoder::update() Decoded " << decodedSize << " of " << stream.sampleSize << std::endl;
					std::cout << "TAACDecoder::update() Remaining " << stream.sampleSize - decodedSize << std::endl;
					std::cout << "TAACDecoder::update() Progress " << (decodedSize * 100 / stream.sampleSize) << " %" << std::endl;
				}

				// Set decoded size in current decoder buffer
				buffer->write(bytesDecoded);
				decodedSize += bytesDecoded;
				read = bytesDecoded;

				// Check if all expected bytes decoded
				if (!eof) {
					eof = (decodedSize >= stream.sampleSize);
					if (debug && eof) std::cout << "TAACDecoder::update() EOF on decode succeeded." << std::endl;
				}

				// Set file offset to next read chunk
				bool consumed = false;
				fileOffset += bytesDecoded;
				readOffset += bytesDecoded;
				if (bufferSize > bytesDecoded) {
					bufferSize -= bytesDecoded;
				}
				if (bufferSize < (2 * bytesDecoded)) {
					consumed = true;
				}

				// Read next chunk from file
				if (consumed) {
					// Start reading at first position of input buffer
					readOffset = 0;

					// Go to next file position
					if (debug) util::hexout("TAACDecoder::update() File offset = ", fileOffset);
					r = file.seek(fileOffset);
					if (r != (ssize_t)fileOffset)
						throw util::app_error_fmt("TAACDecoder::update() Seek to next chunk data failed (@/@)", r, fileOffset);

					// Read input buffer size from file
					bytesRead = this->read(file, inputBuffer, readBufferSize);
					if (debug) std::cout << "TAACDecoder::update() Read from file = " << bytesRead << std::endl;
					if (bytesRead <= 0) {
						if (buffer->getWritten() <= 0)
							throw util::app_error_fmt("TAACDecoder::update() Reading input buffer failed (%/%)", bytesRead, readBufferSize);
						eof = true;
						if (debug) std::cout << "TAACDecoder::update() EOF on read detected." << std::endl;
					}
				}

			} catch (const std::exception& e) {
				std::cout << "TAACDecoder::update() Exception: \"" << e.what() << "\"" << std::endl;
				error = true;
			}

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				std::cout << "TAACDecoder::update() EOF detected." << std::endl;
			}

			if (eof && stream.sampleSize != decodedSize) {
				std::cout << "TAACDecoder::update() Decoded " << decodedSize << " of " << stream.sampleSize << std::endl;
				std::cout << "TAACDecoder::update() Missing " << stream.sampleSize - decodedSize << std::endl;
			}

			// Check for error...
			lastOffset = fileOffset;
			if (!error)
				return true;
		}
	}

	// Create internal error
	error = true;
	setBuffer(nil);
	close();
	return false;
}


bool TAACDecoder::seek(const util::TFile& file, const size_t offset) {
	if (offset != std::string::npos && offset < file.getSize()) {
		return ((ssize_t)offset == file.seek(offset, util::SO_FROM_START));
	}
	return false;
}

ssize_t TAACDecoder::read(const util::TFile& file, TDecoderBuffer& buffer, const size_t size) {
	return file.read((char*)buffer.data(), size, 0, util::SO_FROM_START);
}


void TAACDecoder::close() {
	bool r = error;
	clear();
	file.close();
	freeDecoder();
	if (debug)
		 std::cout << "TAACDecoder::close()" << std::endl;
	if (r)
		throw util::app_error("TAACDecoder::close() Close on error");
}



TALACDecoder::TALACDecoder() {
	prime();
}

TALACDecoder::~TALACDecoder() {
	util::freeAndNil(decoder);
}

void TALACDecoder::prime() {
	TAudioStream::prime();
	debug = false;
	decoder = nil;
	readBufferSize = 0;
	inputBufferSize = 0;
    inputPacketSize = 0;
    outputPacketSize = 0;
    decodedSize = 0;
    fileOffset = 0;
    lastOffset = 0;
}


bool TALACDecoder::createDecoder() {
	freeDecoder();

	// Set decoder configuration for current file:
	//
	//	struct	ALACSpecificConfig (defined in ALACAudioTypes.h)
	//
	//	abstract
	//	This struct is used to describe codec provided information about the encoded Apple Lossless bitstream.
	//	It must accompany the encoded stream in the containing audio file and be provided to the decoder.
	//
	//	field      	frameLength 		uint32_t	indicating the frames per packet when no explicit frames per packet setting is
	//												present in the packet header. The encoder frames per packet can be explicitly set
	//												but for maximum compatibility, the default encoder setting of 4096 should be used.
	//
	//	field      	compatibleVersion 	uint8_t 	indicating compatible version,
	//												value must be set to 0
	//
	//	field      	bitDepth 			uint8_t 	describes the bit depth of the source PCM data (maximum value = 32)
	//
	//	field      	pb 					uint8_t 	currently unused tuning parameter.
	//												value should be set to 40
	//
	//	field      	mb 					uint8_t 	currently unused tuning parameter.
	//												value should be set to 14 <-- crap info from Apple: #define MB0 10
	//
	//	field      	kb					uint8_t 	currently unused tuning parameter.
	//												value should be set to 10 <-- crap info from Apple: #define KB0 14
	//
	//	field      	numChannels 		uint8_t 	describes the channel count (1 = mono, 2 = stereo, etc...)
	//												when channel layout info is not provided in the 'magic cookie', a channel count > 2
	//												describes a set of discreet channels with no specific ordering
	//
	//	field      	maxRun				uint16_t 	currently unused.
	//												value should be set to 255
	//
	//	field      	maxFrameBytes 		uint32_t 	the maximum size of an Apple Lossless packet within the encoded stream.
	//												value of 0 indicates unknown <-- crap info from Apple (as we know them): must be at least 4096!
	//
	//	field      	avgBitRate 			uint32_t	the average bit rate in bits per second of the Apple Lossless stream.
	//												value of 0 indicates unknown
	//
	//	field      	sampleRate 			uint32_t	sample rate of the encoded stream
	//
	ALACSpecificConfig config;
	config.frameLength 			= (uint32_t)kALACDefaultFrameSize;
	config.compatibleVersion 	= (uint8_t)kALACCompatibleVersion;
	config.bitDepth 			= stream.bitsPerSample;
	config.pb 					= (uint8_t)PB0;
	config.mb 					= (uint8_t)MB0;
	config.kb 					= (uint8_t)KB0;
	config.numChannels 			= stream.channels;
	config.maxRun 				= (uint16_t)MAX_RUN_DEFAULT;
	config.maxFrameBytes 		= (uint16_t)MAX_FRAME_BYTES;
	config.avgBitRate 			= (uint32_t)0;
	config.sampleRate 			= stream.sampleRate;

	// Calculate buffer sizes
	outputPacketSize = stream.chunkSize; // = stream.bytesPerSample * stream.channels * kALACDefaultFrameSize (4096)
	inputPacketSize  = outputPacketSize + kALACMaxEscapeHeaderBytes;
	inputBufferSize  = 2 * (size_t)inputPacketSize;
	readBufferSize   = inputBufferSize;

	// Setup buffers
	inputBuffer.resize(inputBufferSize + 2);
    BitBufferInit(&readBuffer, inputBuffer.data(), inputBufferSize);

	// Create new decoder
	decoder = new ALACDecoder;
	int32_t r = decoder->Init(config);

    return (r == ALAC_noErr);
}

void TALACDecoder::freeDecoder() {
	util::freeAndNil(decoder);
	inputBuffer.clear();
	fileOffset = 0;
	lastOffset = 0;
	decodedSize = 0;
}


bool TALACDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TALACDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TALACDecoder::open(const PSong song) {
	if (util::assigned(song))
		return open(*song);
	return false;
}

bool TALACDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TAlacDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {

				// Initialize decoder
				setStream(properties);
				if (createDecoder()) {

					// Parse file to find data chunk
					size_t offset = 0;
					fileOffset = 0;
					TALACAtomScanner parser;
					parser.setCodec(EFT_ALAC);
					parser.setDebug(debug);
					ssize_t r = parser.scan(file, ETL_METADATA);
					if (r > 0) {
						// Set offset to first data byte after data chunk file header
						TALACAtom atom = parser["mdat"];
						if (atom.data != std::string::npos) {
							// Skip 'mdat' header + DWORD for version flags and 4 reserved bytes
							//offset = atom.offset + sizeof(TChunkSize) + sizeof(TChunkID);
							offset = atom.data;
						}

						// Seek to first byte after 'mdat' header
						if (offset > 0) {
							r = file.seek(offset);
							if (r == (ssize_t)offset) {
								// Read first ALAC block header
								size_t hsize = sizeof(TAlacBlockHeader);
								r = read(file, inputBuffer, hsize);
								if (r == (ssize_t)hsize) {
									if (getBlockHeader(inputBuffer.data(), hsize, blockHeader)) {
										fileOffset = offset;
									} else {
										std::string b = util::TBinaryConvert::TBinaryConvert::binToHexA(inputBuffer.data(), r);
										std::string s = util::csnprintf("TAlacDecoder::open() Invalid block header (%/%) found at offset = @ [%]", hsize, r, offset, b);
										std::cout << s << std::endl;
										error = true;
									}
								}
							}
						}

					} else {
						error = true;
					}

				}
				if (fileOffset <= 0 || fileOffset > file.getSize())
					error = true;

				// Close file on error!
				if (error) {
					fileOffset = 0;
					opened = false;
					file.close();
				}
			}
			if (!opened) {
				freeDecoder();
				clear();
			}
			if (debug) {
				stream.debugOutput();
			}
			lastOffset = fileOffset;
			return opened;
		}
	}
	setRead(0);
	lastOffset = fileOffset;
	return false;
}


bool TALACDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			PSample samples = buffer->writer();
			size_t offset;
			ssize_t bytesRead;
			size_t bytesDecoded;
			uint32_t frames;
			uint8_t* bitBuffer;
			ssize_t r;

			// File reader may throw exception...
			try {

				// Go to file position
				if (debug) util::hexout("TAlacDecoder::update() File offset = ", fileOffset);
				r = file.seek(fileOffset);
				if (r != (ssize_t)fileOffset)
					throw util::app_error_fmt("TAlacDecoder::update() Seek to next header position failed (@/@)", r, fileOffset);

				// Read input buffer size from file
				// inputBuffer.fillchar(0);
				bytesRead = this->read(file, inputBuffer, readBufferSize);
				if (debug) std::cout << "TAlacDecoder::update() Read from file = " << bytesRead << std::endl;
				if (bytesRead > 0) {

					// Check for block header ID (simplified check!!!)
					if (!compareBlockID(inputBuffer.data(), blockHeader))
						throw util::app_error("TAlacDecoder::update() Block header check failed.");

					/* Find next block header
					offset = findNextBlockHeader(inputBuffer.data(), bytesRead, blockHeader);
					if (offset == std::string::npos) {
						eof = true;
						fileOffset = 0;
						if (debug) std::cout << "TAlacDecoder::update() EOF on next block header." << std::endl;
					} else {
						// fileOffset += offset;
						if (debug) std::cout << std::endl << "TAlacDecoder::update() Bit buffer offs = " << offset << std::endl;
					}
					if (debug || (offset != std::string::npos && offset >= MAX_FRAME_BYTES)) {
						std::cout << "TAlacDecoder::update() Block size = " << (ssize_t)offset << std::endl;
						util::hexout("TAlacDecoder::update() Last file offset = ", lastOffset);
						util::hexout("TAlacDecoder::update() Next file offset = ", fileOffset);
					}
					*/

					// Decode data chunk
					bitBuffer = readBuffer.cur;
					int32_t result = decoder->Decode(&readBuffer, (uint8_t*)samples, kALACDefaultFrameSize, stream.channels, &frames);
					if (result != ALAC_noErr)
						throw util::app_error_fmt("TAlacDecoder::update() Decoder failed failed on error (%)", result);
					if (frames <= 0)
						throw util::app_error_fmt("TAlacDecoder::update() Decoder returned no data (%)", frames);

					// Decoded data size in bytes
					bytesDecoded = frames * stream.bytesPerSample * stream.channels;
					if (debug) {
						std::cout << "TAlacDecoder::update() Frames decoded = " << frames << std::endl;
						std::cout << "TAlacDecoder::update() Bytes  decoded = " << bytesDecoded << std::endl;
					}

					// Set decoded size in current decoder buffer
					buffer->write(bytesDecoded);
					decodedSize += bytesDecoded;
					read = bytesDecoded;

					// Check if all bytes decoded
					if (debug) {
						std::cout << "TAlacDecoder::update() Decoded " << decodedSize << " of " << stream.sampleSize << std::endl;
						std::cout << "TAlacDecoder::update() Remaining " << stream.sampleSize - decodedSize << std::endl;
						std::cout << "TAlacDecoder::update() Progress " << (decodedSize * 100 / stream.sampleSize) << " %" << std::endl;
					}
					if (!eof) {
						eof = (decodedSize >= stream.sampleSize);
						if (debug && eof) std::cout << "TAlacDecoder::update() EOF on decode succeeded." << std::endl;
					}
					if (!eof) {
						// Get decoded bytes from bit buffer pointer
						size_t bitSize = readBuffer.cur - bitBuffer + 1;
						if (ALAC_BLOCK_ID == inputBuffer[bitSize+1]) {
							// Next byte is valid block header ID
							++bitSize;
						} else {
							if (ALAC_BLOCK_ID != inputBuffer[bitSize]) {
								// Ignore trailing chunks...
								offset = findNextBlockHeader(inputBuffer.data(), bytesRead, blockHeader);
								if (offset == std::string::npos) {
									eof = true;
									if (debug && eof) std::cout << "TAlacDecoder::update() EOF on trailing chunk." << std::endl;
								} else {
									// Offset to next block is valid
									bitSize = offset;
								}
							}
						}
						fileOffset += bitSize;
						if (debug) {
							std::cout << "TAlacDecoder::update() Bit buffer read = " << bitSize << std::endl;
							std::cout << util::csnprintf("TAlacDecoder::update() 0/1/2/3 (@/@/@/@)", (int)inputBuffer[bitSize], (int)inputBuffer[bitSize+1], (int)inputBuffer[bitSize+2], (int)inputBuffer[bitSize+3]) << std::endl;
						}
					}
					if (!eof && fileOffset >= file.getSize()) {
						// EOF reached when reading last chunk
						eof = true;
						if (debug && eof) std::cout << "TAlacDecoder::update() EOF for bit buffer offset detected." << std::endl;
					}

					// Reset internal buffer
					BitBufferReset(&readBuffer);

				} else {
					if (buffer->getWritten() <= 0)
						throw util::app_error_fmt("TAlacDecoder::update() Reading input buffer failed (%/%)", bytesRead, stream.chunkSize);
					eof = true;
					if (debug) std::cout << "TAlacDecoder::update() EOF on read detected." << std::endl;
				}

			} catch (const std::exception& e) {
				std::cout << "TAlacDecoder::update() Exception: \"" << e.what() << "\"" << std::endl;
				error = true;
			}

			// Detect End Of File
			//if (!error && !eof && (bytesDecoded < song->getChunkSize())) {
			//	eof = true;
			//	if (debug) std::cout << "TAlacDecoder::update() EOF on last block (Decoded " << bytesDecoded << " of " << song->getChunkSize() << " bytes)" << std::endl;
			//}

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				std::cout << "TAlacDecoder::update() EOF detected." << std::endl;
			}

			if (eof && stream.sampleSize != decodedSize) {
				std::cout << "TAlacDecoder::update() Decoded " << decodedSize << " of " << stream.sampleSize << std::endl;
				std::cout << "TAlacDecoder::update() Missing " << stream.sampleSize - decodedSize << std::endl;
			}

			// Check for error...
			lastOffset = fileOffset;
			if (!error)
				return true;
		}
	}

	// Create internal error
	error = true;
	setBuffer(nil);
	close();
	return false;
}


bool TALACDecoder::getBlockHeader(const void *const buffer, const size_t size, TAlacBlockHeader& header) const {
	size_t hsize = sizeof(TAlacBlockHeader);
	/* std::cout << util::csnprintf("TAlacDecoder::getBlockHeader() Header size (%/%)", size, hsize) << std::endl; */
	if (size >= hsize) {
		memcpy(&header, buffer, hsize);
		return isBlockHeader(header);
	}
	return false;
}

size_t TALACDecoder::findNextBlockHeader(const void *const buffer, const size_t size, const TAlacBlockHeader& header) const {
	size_t hsize = sizeof(TAlacBlockHeader);
	size_t max = size - hsize;
	uint8_t* p = (uint8_t*)buffer + hsize;
	if (debug) std::cout << util::csnprintf("TALACDecoder::findNextBlockHeader() Find next block header from @ in range of % bytes", (uint64_t)p, size) << std::endl;
	for (size_t i=hsize; i<max; ++i) {
		if (*p == ALAC_BLOCK_ID) {
			if (compareBlockHeader(p, header)) {
				if (debug || i >= MAX_FRAME_BYTES) std::cout << util::csnprintf("TALACDecoder::findNextBlockHeader() Block header found for offset % at @", i, (uint64_t)p) << std::endl;
				return i;
			}
		}
		++p;
	}
	return std::string::npos;
}

bool TALACDecoder::compareBlockID(const void *const buffer, const TAlacBlockHeader& header) const {
	TAlacBlockHeader ab;
	memcpy(&ab, buffer, sizeof(TAlacBlockHeader));
	return (ab.alBlockID == header.alBlockID);
}

bool TALACDecoder::compareBlockHeader(const void *const buffer, const TAlacBlockHeader& header) const {
	/*
	 * Examples:
	 * 0x20/0x00/0x14/0x00/0xa0 cobham
	 * 0x20/0x00/0x14/0x00/0xa0 sara k
	 * 0x20/0x00/0x00/0x13/0x08 alan parsons
	 * 0x20/0x00/0x00/0x13/0x08 little axe <-- ignore alReserved[3]
	*/
	TAlacBlockHeader ab;
	memcpy(&ab, buffer, sizeof(TAlacBlockHeader));
	return ((ab.alByte1 == header.alByte1) && \
			(ab.alReserved[0] == header.alReserved[0]) && \
			((ab.alReserved[1] == header.alReserved[1]) || ((header.alReserved[1] == 0x14) && (ab.alReserved[1] == 0x12))) && \
			(ab.alReserved[2] == header.alReserved[2]) && \
			((ab.alReserved[3] == header.alReserved[3]) || (header.alByte1 == 0x13)));
}

bool TALACDecoder::isBlockHeader(const TAlacBlockHeader& header) const {
	bool r = ((header.alBlockID == ALAC_BLOCK_ID) && \
			 ((header.alReserved[0] == 0x00)) && \
			 ((header.alReserved[1] == 0x14) || (header.alReserved[1] == 0x00)) && \
			 ((header.alByte1 == 0x13) || (header.alByte1 == 0x03) || (header.alByte1 == 0x02) || (header.alByte1 == 0x00)));
	std::cout << util::csnprintf("TAlacDecoder::isBlockHeader() Header BlockID/Reserved[0]/Reserved[1]/Byte1/Byte2 (@/@/@/@/@) Valid = %", \
			(int)header.alBlockID, (int)header.alReserved[0], (int)header.alReserved[1], (int)header.alByte1, (int)header.alByte2, r) << std::endl;
	return r;
}


bool TALACDecoder::seek(const util::TFile& file, const size_t offset) {
	if (offset != std::string::npos && offset < file.getSize()) {
		return ((ssize_t)offset == file.seek(offset, util::SO_FROM_START));
	}
	return false;
}

ssize_t TALACDecoder::read(const util::TFile& file, TDecoderBuffer& buffer, const size_t size) {
	return file.read((char*)buffer.data(), size, 0, util::SO_FROM_START);
}


size_t TALACDecoder::getBlockLength(const TDecoderBuffer& buffer, size_t& length) {
	if (length > 0) {
		size_t r = 0;
		size_t size = 0;
		uint8_t c;
		do {
			c = buffer[size];
			r = (r << 7) | (c & 0x7F);
			if (++size > (size_t)ALAC_MAX_BER_SIZE) {
				length = (size_t)0;
				return (size_t)0;
			}
		} while(((c & 0x80) != 0) && (size <= length));

		// Return block length and set length to entry size
		length = size;
		return r;
	}
	return (size_t)0;
}


void TALACDecoder::close() {
	clear();
	file.close();
	freeDecoder();
	if (debug)
		 std::cout << "TAlacDecoder::close()" << std::endl;
}



TALACFile::TALACFile() : TSong(EFT_ALAC) {
	prime();
}

TALACFile::TALACFile(const ECodecType type) : TSong(type) {
	prime();
}

TALACFile::TALACFile(const std::string& fileName) : TSong(fileName, EFT_ALAC) {
	prime();
}

TALACFile::~TALACFile() {
}

void TALACFile::prime() {
}

void TALACFile::decoderNeeded() {
	if (!util::assigned(stream)) {
		if (EFT_AAC == getType()) {
			stream = new TAACDecoder;
		} else {
			stream = new TALACDecoder;
		}
	}
}

bool TALACFile::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

bool TALACFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Scan file for ALAC chunks
		TALACAtomScanner parser;
		ssize_t r = parser.scan(file, ETL_METADATA);
		if (r > 0) {
			TALACAtom atom;
			util::TTimePart sec, ms;

			// Content of ALAC data chunks
			// ================================================
			//
			//	typedef struct CAlacStreamChunk {
			//		TChunkSize	alChunkSize;		// Offset 0
			//		TChunkID	alChunkID;			// Offset 4  "stsd"
			//		DWORD		alVersionFlag;		// Offset 8  = 0
			//		DWORD		alEntryCount;		// Offset 12 = 1
			//		DWORD		alSampleEntrySize; 	// Offset 16
			//		TTypeID		alEntryType;  		// Offset 20 "alac"
			//		BYTE		alReserved1[7];		// Offset 24
			//		BYTE		alReferenceIndex;	// Offset 31 = 1
			//		BYTE		alReserved2[8];		// Offset 32
			//		WORD		alNumChannels;		// Offset 40
			//		WORD		alSampleSize;		// Offset 42
			//		BYTE		alReserved3[2];		// Offset 44
			//		DWORD		alSampleRate;		// Offset 48
			//	} PACKED TAlacStreamChunk;
			//
			//	typedef struct CAlacMediaChunk {
			//		TChunkSize	alChunkSize;		// Offset 0
			//		TChunkID	alChunkID;			// Offset 4  "mdhd"
			//		DWORD		alVersionFlag;		// Offset 8  = 0
			//		BYTE		alReserved[8];		// Offset 12
			//		DWORD		alSampleRate;		// Offset 20
			//		DWORD		alNumFrames;		// Offset 24 --> Duration = alNumFrames / alSampleRate
			//	} PACKED TAlacMediaChunk;
			//

			TAlacStreamChunk stream;
			TAlacMediaChunk media;
			size_t ssize = sizeof(TAlacStreamChunk);
			size_t msize = sizeof(TAlacMediaChunk);

			// Read stream chunk
			atom = parser["stsd"];
			if (atom.offset == std::string::npos) {
				tag.error = "Corrupt file header, STSD chunk not found";
				return false;
			}
			if (!readHeader(file, atom.offset, &stream, ssize)) {
				tag.error = "Invalid offset for STSD chunk";
				return false;
			}
			setStreamChunkEndian(stream);

			// Read media chunk
			atom = parser["mdhd"];
			if (atom.offset == std::string::npos) {
				tag.error = "Corrupt file header, MHDH chunk not found";
				return false;
			}
			if (!readHeader(file, atom.offset, &media, msize)) {
				tag.error = "Invalid offset for MHDH chunk";
				return false;
			}
			setMediaChunkEndian(media);

			// Try to read samplerate from media chunk
			int sampleRate = 0;
			if (media.alSampleRate > 0) {
				sampleRate = media.alSampleRate;
			} else {
				if (stream.alSampleRate > 0) {
					sampleRate = stream.alSampleRate;
				}
			}

			// Info is crap, escape divide-by-zero
			if (media.alNumFrames <= 0 || sampleRate <= 0) {
				std::cout << "TALACFile::readMetaData() Invalid metadata (" << media.alNumFrames << "/" << sampleRate << std::endl;
				tag.error = util::csnprintf("Missing or zeroed sound properties: NumFrames=%, SampleRate=%", media.alNumFrames, sampleRate);
				return false;
			}

			// Get duration from media header
			ms = (util::TTimePart)((size_t)media.alNumFrames * (size_t)1000 / sampleRate);
			sec = ms / 1000;

			// Time is crap, escape divide-by-zero
			if (ms <= 0 || sec <= 0) {
				tag.error = "Invalid song duration in metadata";
				return false;
			}

			// Get stream information
			tag.stream.duration = ms;
			tag.stream.seconds = sec;
			tag.stream.channels = stream.alNumChannels;
			tag.stream.sampleRate = sampleRate;
			tag.stream.bitsPerSample = normalizeSampleSize(stream.alSampleSize);
			tag.stream.bytesPerSample = tag.stream.bitsPerSample / 8;
			tag.stream.sampleCount = media.alNumFrames;
			tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.bytesPerSample * tag.stream.channels; // Storage size for all samples in bytes
			tag.stream.bitRate = bitRateFromTags(tag);

			// Fallback on crap sample rate info
			if (!util::isMemberOf((ESampleRate)tag.stream.sampleRate, SR44K,SR48K,SR88K,SR96K,SR176K,SR192K,SR352K,SR384K))
				tag.stream.sampleRate = media.alSampleRate;

			// Estimated buffer size for frame decoder
			tag.stream.chunkSize = tag.stream.bytesPerSample * tag.stream.channels * kALACDefaultFrameSize;

			// Get metadata from parser
			if (parser.hasTags()) {
				parser.getTags(tag.meta);
			}

			// Check if codec has to be changed
			if (EFT_AAC == parser.getCodec()) {
				type = parser.getCodec();
			}

        	if (!tag.stream.isValid()) {
				tag.stream.debugOutput(" ALAC: ");
				tag.meta.debugOutput(" META: ");
				tag.error = "Invalid stream format, e.g. Bitrate, channels, samplerate, etc. invalid";
				return false;
        	}

        	return true;
		}
		tag.error = "Corrupt ALAC file, no valid metadata found";
	}
	tag.error = "ALAC file not readable";
	return false;
}

bool TALACFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Scan file for ALAC chunks
		TALACAtomScanner parser;
		ssize_t r = parser.scan(file, ETL_PICTURE);
		if (r > 0) {

			// Get picture metadata from file
			parser.getPicture(cover);
			return cover.isValid();

		}
	}
	return false;
}


bool TALACFile::readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size) {
	if (seek(file, offset)) {
		return ((ssize_t)size == read(file, buffer, size));
	}
	return false;
}

bool TALACFile::seek(const util::TFile& file, const size_t offset) {
	if (offset != std::string::npos && offset < file.getSize()) {
		return ((ssize_t)offset == file.seek(offset, util::SO_FROM_START));
	}
	return false;
}

ssize_t TALACFile::read(const util::TFile& file, void *const data, const size_t size) {
	return file.read((char*)data, size, 0, util::SO_FROM_START);
}



} /* namespace music */
