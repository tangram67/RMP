/*
 * pcm.cpp
 *
 *  Created on: 21.08.2016
 *      Author: Dirk Brinkmeier
 */

#include <cstring>
#include "fileutils.h"
#include "templates.h"
#include "stringutils.h"
#include "audiobuffer.h"
#include "audioconsts.h"
#include "pcmtypes.h"
#include "compare.h"
#include "gcc.h"
#include "pcm.h"

namespace music {


TRIFFAtomScanner::TRIFFAtomScanner() {
	prime();
}

TRIFFAtomScanner::~TRIFFAtomScanner() {
}

void TRIFFAtomScanner::prime() {
	debug = false;
	defItem.ID = "<invalid>";
	defItem.offset = std::string::npos;
	defItem.size = std::string::npos;
	seekPos = 0;
}

void TRIFFAtomScanner::clear() {
	atoms.clear();
	tags.clear();
	seekPos = 0;
}


const TRIFFAtom& TRIFFAtomScanner::addAtom(const std::string& ID, size_t offset, size_t size) {
	const_iterator it = atoms.find(ID);
	if (it == end()) {
		TRIFFAtom atom;
		atom.ID = ID;
		atom.offset = offset;
		atom.size = size;
		atom.hash = 0;
		if (ID.size() > 3)
			atom.hash = MAKE_ATOM(ID[0], ID[1], ID[2], ID[3]);
		if (debug) {
			std::cout << "TRIFFAtomScanner::addAtom() ID \"" << ID << "\"" << std::endl;
			std::cout << "TRIFFAtomScanner::addAtom() Offset = " << offset << std::endl;
			std::cout << "TRIFFAtomScanner::addAtom() Size   = " << size << std::endl;
			std::cout << "TRIFFAtomScanner::addAtom() Hash   = " << atom.hash << std::endl;
		}
		atoms[ID] = atom;
		return find(ID);
	}
	return defItem;
}

bool TRIFFAtomScanner::addTag(const DWORD hash, const std::string& value) {
	TTagMap::const_iterator it = tags.find(hash);
	if (it == tags.end()) {
		if (debug) {
			std::cout << "TRIFFAtomScanner::addTag() Hash = " << hash << std::endl;
			std::cout << "TRIFFAtomScanner::addTag() Value = \"" << value << "\"" << std::endl;
		}
		tags[hash] = value;
		return true;
	}
	return false;
}

bool TRIFFAtomScanner::validTagID(const DWORD hash) const {
	DWORD h;
	size_t i = 0;
	do {
		h = RIFF_TAG_ATOMS[i];
		if (h == hash)
			return true;
		++i;
	} while (h > 0);
	return false;
}

std::string TRIFFAtomScanner::hashTagToStr(const DWORD hash) const {
	return std::string((char*)&hash, sizeof(DWORD));
}


const TRIFFAtom& TRIFFAtomScanner::find(const std::string& ID) const {
	const_iterator it = atoms.find(ID);
	if (it != end())
		return it->second;
	return defItem;
}

const TRIFFAtom& TRIFFAtomScanner::operator[] (const std::string& ID) const {
	return find(ID);
}


ssize_t TRIFFAtomScanner::scan(util::TFile& file, size_t offset) {
	clear();

	// Find start of RIFF atoms after LIST....INFO header
	util::TFileScanner parser;
	app::TStringVector pattern = {"LIST"};
	parser.scan(file, offset, pattern, 512);
	offset = parser["LIST"];
	if (std::string::npos == offset)
		return -10;

	// Read INFO header
	TPCMInfoHeader header;
	size_t hsize = sizeof(TPCMInfoHeader);
	if (!readHeader(file, offset, &header, hsize))
		return -11;
	if (!isChunkID(header.pfChunkID, "LIST    "))
		return -12;
	if (!isChunkID(header.pfInfoID, "INFO    "))
		return -13;

	// Start of RIFF metadata found
	offset += hsize;
	scanAtoms(file, offset);

	// Read complete atom metadata chunk from file
	header.pfChunkSize = convertFromLittleEndian32(header.pfChunkSize);
	util::TBuffer buffer(header.pfChunkSize + 1);
	if (!seek(file, offset))
		return (ssize_t)-20;
	if (!read(file, buffer.data(), header.pfChunkSize))
		return (ssize_t)-21;

	// Read atom values from buffer
	scanTags(buffer.data(), header.pfChunkSize);

	// Return atom count
	return (ssize_t)tags.size();
}


ssize_t TRIFFAtomScanner::scanAtoms(const util::TFile& file, const size_t offset) {
	ssize_t retVal = 0;
	bool eof = offset <= 0;
	size_t position = offset;
	CPCMChunkHeader header;
	size_t hsize = sizeof(CPCMChunkHeader);
	size_t csize = sizeof(TChunkID);

	if (debug) util::hexout("TRIFFAtomScanner::scanAtoms() RIFF metadata starts at offset = ", offset);

	while (!eof) {

		// Read header at given file position
		if (!readHeader(file, position, &header, hsize))
			return (ssize_t)-40;

		// Add header at current position to map
		header.pfChunkSize = convertFromLittleEndian32(header.pfChunkSize);
		if (header.pfChunkID[0] != '\0' && header.pfChunkSize > 0) {

			std::string s(header.pfChunkID, csize);
			addAtom(s, position - offset + hsize, header.pfChunkSize);
			++retVal;

			// Goto next atom chunk at even (!) position
			position += hsize + header.pfChunkSize + (header.pfChunkSize % 2);

			// Check for EOF
			eof = (position + 1) > file.getSize();

		} else {
			// Invalid header
			eof = true;
		}

	}
	return retVal;
}

ssize_t TRIFFAtomScanner::scanTags(const char *const buffer, const size_t size) {
	ssize_t retVal = 0;

	if (debug) std::cout << "TRIFFAtomScanner::scanTags() RIFF metadata size = " << size << std::endl;

	// Iterate through atoms in buffer
	TAtomMap::const_iterator it = atoms.begin();
	do {
		if (validTagID(it->second.hash)) {
			size_t offset = it->second.offset;
			size_t length = it->second.size;
			if ((offset + length) < size) {
				char* p = (char*)buffer + offset;
				trim(p, length);
				if (size > 0) {
					std::string s;
					if (isBOM(p, length)) {
						// Read tag value as UTF-16 string
						s = UTF16ToMultiByteStr(p, length);
					} else {
						// Read tag value as ANSI string
						s = AnsiToMultiByteStr(p, length);
					}
					if (!s.empty())
						addTag(it->second.hash, s);
				}
			}
		}
		++it;
	} while (it != atoms.end());

	return retVal;
}


ssize_t TRIFFAtomScanner::getTags(CMetaData& tag) {
	ssize_t retVal = 0;
	if (!tags.empty()) {
		TTagMap::const_iterator it = tags.begin();
		bool found;
		do {
			found = false;

			if (!found && it->first == RIFF_IART_ATOM) {
				tag.text.artist = it->second;
				tag.text.albumartist = tag.text.artist;
				found = true;
				++retVal;
			}

			if (!found && it->first == RIFF_IPRD_ATOM) {
				tag.text.album = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == RIFF_INAM_ATOM) {
				tag.text.title = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == RIFF_IGNR_ATOM) {
				tag.text.genre = it->second;
				found = true;
				++retVal;
			}

			if (!found && it->first == RIFF_ICRD_ATOM) {
				int year = tagToInt(it->second, 1900);
				if (year > 0)
					tag.text.year = util::cprintf("%04.4d", year);
				if (tag.text.year.empty())
					tag.text.year = "1900";
				found = true;
				++retVal;
			}

			if (!found && it->first == RIFF_ITRK_ATOM) {
				tag.track.tracknumber = tagToInt(it->second, 1);
				tag.text.track = util::cprintf("%02.2d", tag.track.tracknumber);
				found = true;
				++retVal;
			}

			// All tags read?
			if (retVal >= 6)
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

char* TRIFFAtomScanner::findAtom(const std::string& ID, char* buffer, size_t size) const {
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

bool TRIFFAtomScanner::readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size) {
	if (seek(file, offset)) {
		return read(file, buffer, size);
	}
	return false;
}

const char *const TRIFFAtomScanner::copyHeader(const char *const buffer, const size_t offset, void *const header, const size_t size) const {
	const char *const src = buffer + offset;
	memcpy(header, src, size);
	return src;
}

bool TRIFFAtomScanner::seek(const util::TFile& file, const size_t offset) {
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

bool TRIFFAtomScanner::read(const util::TFile& file, void *const data, const size_t size) {
	ssize_t r = file.read((char*)data, size, 0, util::SO_FROM_START);
	if (r == (ssize_t)size) {
		seekPos += size;
		return true;
	}
	return false;
}


bool TRIFFAtomScanner::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

void TRIFFAtomScanner::debugOutputChunks(const std::string& preamble) const {
	const_iterator it = begin();
	if (!atoms.empty()) {
		while (it != end()) {
			std::cout << preamble << "ID \"" << it->second.ID << "\"" << std::endl;
			std::cout << preamble << "Offset = " << it->second.offset << std::endl;
			std::cout << preamble << "Size   = " << it->second.size << std::endl;
			std::cout << std::endl;
			++it;
		}
	} else
		std::cout << preamble << "No pattern found." << std::endl;
}

void TRIFFAtomScanner::debugOutputTags(const std::string& preamble) const {
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





TPCMStream::TPCMStream() {
}

TPCMStream::~TPCMStream() {
}

void TPCMStream::setHeaderEndian(TPCMFileHeader& header) const {
#ifdef TARGET_BIG_ENDIAN
	header.pfChunkSize     = convertFromLittleEndian32(header.pfChunkSize);
	header.pfSubchunk1Size = convertFromLittleEndian32(header.pfSubchunk1Size);
	header.pfAudioFormat   = convertFromLittleEndian16(header.pfAudioFormat);
	header.pfNumChannels   = convertFromLittleEndian16(header.pfNumChannels);
	header.pfSampleRate    = convertFromLittleEndian32(header.pfSampleRate);
	header.pfByteRate      = convertFromLittleEndian32(header.pfByteRate);
	header.pfBlockAlign    = convertFromLittleEndian16(header.pfBlockAlign);
	header.pfBitsPerSample = convertFromLittleEndian16(header.pfBitsPerSample);
	header.pfSubchunk2Size = convertFromLittleEndian32(header.pfSubchunk2Size);
#endif
}

void TPCMStream::setHeaderEndian(TPCMBaseHeader& header) const {
#ifdef TARGET_BIG_ENDIAN
	header.pfChunkSize     = convertFromLittleEndian32(header.pfChunkSize);
	header.pfSubchunk1Size = convertFromLittleEndian32(header.pfSubchunk1Size);
	header.pfAudioFormat   = convertFromLittleEndian16(header.pfAudioFormat);
	header.pfNumChannels   = convertFromLittleEndian16(header.pfNumChannels);
	header.pfSampleRate    = convertFromLittleEndian32(header.pfSampleRate);
	header.pfByteRate      = convertFromLittleEndian32(header.pfByteRate);
	header.pfBlockAlign    = convertFromLittleEndian16(header.pfBlockAlign);
	header.pfBitsPerSample = convertFromLittleEndian16(header.pfBitsPerSample);
#endif
}

void TPCMStream::setChunkEndian(TPCMChunkHeader& header) const {
#ifdef TARGET_BIG_ENDIAN
	header.pfChunkSize     = convertFromLittleEndian32(header.pfChunkSize);
#endif
}

bool TPCMStream::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

size_t TPCMStream::readChunkHeader(const util::TBuffer& buffer, const size_t offset, TPCMChunkHeader& header) const {
	size_t hsize = sizeof(TPCMChunkHeader);
	if ((offset + hsize) >= buffer.size())
		return std::string::npos;

	const char *const p = buffer.data() + offset;
	memcpy((void*)&header, p, hsize);
	setChunkEndian(header);

	// Return offset of next possible header position
	return offset + hsize + header.pfChunkSize;
}

size_t TPCMStream::parseWaveHeader(const util::TFile& file, TPCMBaseHeader& header, TPCMChunkHeader& data) const {
	size_t hsize = sizeof(TPCMBaseHeader);
	size_t offset, start, next;
	util::TBuffer chunk(AUDIO_CHUNK_SIZE);
	ssize_t r;

	// Read PCM base header
	r = file.read((char*)&header, hsize);
	if (r != (ssize_t)hsize)
		return std::string::npos;
	setHeaderEndian(header);

	// Check PCM base header
	if (!isWave(header))
		return std::string::npos;

	// Seek to first position after sub chunk header:
	// TChunkID	pfChunkID;			// Offset 0  "RIFF"  +4
	// DWORD	pfChunkSize;		// Offset 4          +4
	// TChunkID	pfFormat;			// Offset 8  "WAVE"  +4
	// TChunkID	pfSubchunk1ID;		// Offset 12 "fmt "  +4
	// DWORD	pfSubchunk1Size;	// Offset 16 + DWORD +4 = 5 * 4 = 20
	start = offset = 20 + header.pfSubchunk1Size;

	// Read header chunk (at least 16kByte + some data)
	r = file.read(chunk.data(), chunk.size(), offset);
	if (r != (ssize_t)chunk.size())
		return std::string::npos;

	// Walk through chunks to find 'data'
	// --> Starts at offset 0 in buffer read from file after header
	offset = 0;
	while (offset != std::string::npos && offset < chunk.size()) {
		next = readChunkHeader(chunk, offset, data);
		if (isChunkID(data.pfChunkID, "data    ")) {
			// Return absolute offset to PCM data in file stream
			return offset + start + sizeof(TPCMChunkHeader);
		}
		offset = next;
	}

	return std::string::npos;
}

bool TPCMStream::isWave(const TPCMBaseHeader& header) const {
	// Check "magic" header bytes
	if (!isChunkID(header.pfChunkID, "RIFF    "))
		return false;
	if (!isChunkID(header.pfFormat, "WAVE    "))
		return false;
	if (!isChunkID(header.pfSubchunk1ID, "fmt     "))
		return false;
	if ((header.pfAudioFormat != WAVE_FORMAT_PCM) && (header.pfAudioFormat != WAVE_FORMAT_EXTENSIBLE))
		return false;
	if (header.pfChunkSize <= 0)
		return false;
	return true;
}

bool TPCMStream::isWave(const TPCMFileHeader& header) const {
	// Check "magic" header bytes
	if (!isChunkID(header.pfChunkID, "RIFF    "))
		return false;
	if (!isChunkID(header.pfFormat, "WAVE    "))
		return false;
	if (!isChunkID(header.pfSubchunk1ID, "fmt     "))
		return false;
	if (!isChunkID(header.pfSubchunk2ID, "data    "))
		return false;
	if (header.pfChunkSize <= 0)
		return false;
	if ((header.pfAudioFormat != WAVE_FORMAT_PCM) && (header.pfAudioFormat != WAVE_FORMAT_EXTENSIBLE))
		return false;
	return true;
}



TPCMWriter::TPCMWriter() {
}

TPCMWriter::~TPCMWriter() {
}

bool TPCMWriter::saveRawFile(const std::string& fileName, const TAudioBuffer* buffer, const TAudioBufferList* buffers) const {
	// Delete file...
	if (fileName.empty())
		return false;
	util::deleteFile(fileName);

	// Check prerequisites
	if (!util::assigned(buffer))
		return false;

	// Read sample size if song is given...
	size_t fileSize = 0;
	PSong song = buffer->getSong();
	if (util::assigned(song)) {
		CStreamData stream = song->getStreamData();
		if (stream.isValid()) {
			fileSize = stream.sampleSize;
		}
	}

	// Write raw PCM data to file
	size_t written = 0;
	util::TFile file(fileName);
	if (fileSize > 0)
		file.create(fileSize);
	file.open(O_WRONLY | O_CREAT);

	// Write PCM data for all buffers in list for the same song
	if (file.isOpen())
		written = writeRawData(file, buffer, buffers);

	return (fileSize > 0) ? written == fileSize : written == buffer->getWritten();
}


size_t TPCMWriter::writeRawData(util::TFile& file, const TAudioBuffer* buffer, const TAudioBufferList* buffers) const {
	size_t written = 0;
	if (file.isOpen() && util::assigned(buffer)) {

		// Write PCM data for all buffers in list for the same song
		// Hint: The songs are ordered ascending in buffer list by sort() method
		bool found = false;
		PSong q, p = buffer->getSong();
		if (util::assigned(buffers) && util::assigned(p)) {
			if (buffers->count() > 0) {
				PAudioBuffer o;
				for (size_t i=0; i<buffers->count(); ++i) {
					o = buffers->at(i);
					if (util::assigned(o)) {
						q = o->getSong();
						if (util::assigned(q)) {
							if (p->compareByTitleHash(q)) {
								// Song in buffer belongs to given buffer
								written += file.write((char*)o->data(), o->getWritten());
								found = true;
							}
						}
					}
				}
			}
		}

		// Write only current buffer to file, no list or empty list given
		if (!found)
			written += file.write((char*)buffer->data(), buffer->getWritten());
	}
	return written;
}



TPCMEncoder::TPCMEncoder() {
}

TPCMEncoder::~TPCMEncoder() {
}

bool TPCMEncoder::saveToFile(const std::string& fileName, const TAudioBuffer* buffer, const TAudioBufferList* buffers) const {
	TPCMFileHeader header;
	CStreamData stream;

	// Delete file...
	if (fileName.empty())
		return false;
	util::deleteFile(fileName);

	// Check prerequisites
	if (!util::assigned(buffer))
		return false;

	if (!util::assigned(buffer->getSong()))
		return false;

	// Read stream properties
	stream = buffer->getSong()->getStreamData();
	if (!stream.isValid())
		return false;

	// Fill "magic" header bytes
	memcpy(&header.pfChunkID,     "RIFF    ", sizeof(TChunkID));
	memcpy(&header.pfFormat,      "WAVE    ", sizeof(TChunkID));
	memcpy(&header.pfSubchunk1ID, "fmt     ", sizeof(TChunkID));
	memcpy(&header.pfSubchunk2ID, "data    ", sizeof(TChunkID));

	// Fill header values
    // header.pfChunkID		= "RIFF"
	header.pfChunkSize 		= (DWORD)(36 + stream.sampleSize);
    // header.pfFormat		= "WAVE"

	// header.pfSubchunk1ID = "fmt "
	header.pfSubchunk1Size 	= (DWORD)16; // PCM without ExtraParams
	header.pfAudioFormat 	= (WORD)WAVE_FORMAT_PCM;
	header.pfNumChannels	= (WORD)stream.channels;
	header.pfSampleRate		= (DWORD)stream.sampleRate;
	header.pfByteRate		= (DWORD)(stream.sampleRate * stream.channels * stream.bytesPerSample);
	header.pfBlockAlign		= (WORD)(stream.channels * stream.bytesPerSample);
	header.pfBitsPerSample	= (WORD)stream.bitsPerSample;

	// header.pfSubchunk2ID	= "data"
	header.pfSubchunk2Size	= stream.sampleSize;

	// Set correct endian for file header
	setHeaderEndian(header);

	// Create file for given size + header
	size_t written = 0;
	size_t headerSize = sizeof(header);
	size_t fileSize = stream.sampleSize + headerSize;
	util::TFile file(fileName);
	file.create(fileSize);
	file.open(O_WRONLY);
	if (file.isOpen()) {

		// Write header
		written = file.write((char*)&header, headerSize);

		// Write PCM data for all buffers in list for the same song
		if (written == headerSize)
			written += writeRawData(file, buffer, buffers);

	}

	return written == fileSize;
}




TPCMDecoder::TPCMDecoder() {
	debug = false;
}

TPCMDecoder::~TPCMDecoder() {
}

bool TPCMDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TPCMDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TPCMDecoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TPCMDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TPCMDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {

				// Read PCM header(s) from file
				TPCMBaseHeader header;
				TPCMChunkHeader data;
				size_t offset = parseWaveHeader(file, header, data);

				if (offset == std::string::npos || offset > file.getSize())
					error = true;

				// Seek to position and check resulting position
				if (!error) {
					if (debug) util::hexout("TPCMDecoder::open() PCM data offset for file \"" + fileName + "\" = ", offset);
					ssize_t r = file.seek(offset);
					if (r != (ssize_t)offset)
						error = true;
				}

				// Close file on error!
				if (error) {
					opened = false;
					file.close();
				}
			}
			if (opened)
				setStream(properties);
			if (debug) {
				stream.debugOutput();
				std::cout << "  Decoder opened    : " << opened << std::endl;
			}
			return opened;
		}
	}
	setRead(0);
	return false;
}

bool TPCMDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			size_t r = 0;
			PSample samples = buffer->writer();

			// File reader may throw exception...
			try {
				r = file.read((char*)samples, song->getChunkSize());
				if (r > 0) {
					//std::cout << "TPCMDecoder::update() " << r << " bytes of " << song->getChunkSize() << " read." << std::endl;

					// Check if all audio data is read
					// --> Ignore chunk at end of file...
					if ((getRead() + r) >= song->getSampleSize()) {
						r = song->getSampleSize() - getRead();
						if (debug) std::cout << "TPCMDecoder::update() EOF on read finished (" << r << " bytes)" << std::endl;
						eof = true;
					}

					// Set write buffer pointer to current position after successful read
					read = r;
					addRead(read);
					buffer->write(read);

				} else {
					if (debug) std::cout << "TPCMDecoder::update() EOF on read underrun (" << r << " bytes)" << std::endl;
					eof = true;
				}
			} catch (...) {
				error = true;
			}

			// Detect End Of File
			if (!error && !eof && r < song->getChunkSize()) {
				if (debug) std::cout << "TPCMDecoder::update() EOF on chunk size underrun (" << r << " bytes)" << std::endl;
				eof = true;
			}

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				 std::cout << "TPCMDecoder::update() EOF detected." << std::endl;
			}

			// Check for error...
			if (!error)
				return true;
		}
	}

	// Create internal error
	error = true;
	setBuffer(nil);
	file.close();
	return false;
}

void TPCMDecoder::close() {
	clear();
	file.close();
	if (debug) std::cout << "TPCMDecoder::close()" << std::endl;
}



TPCMFile::TPCMFile() : TSong(EFT_WAV) {
	prime();
}

TPCMFile::TPCMFile(const std::string& fileName) : TSong(fileName, EFT_WAV) {
	prime();
}

TPCMFile::~TPCMFile() {
}

void TPCMFile::prime() {
}

void TPCMFile::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TPCMDecoder;
}

bool TPCMFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read PCM header from file
		TPCMBaseHeader header;
		TPCMChunkHeader data;

		// Complete header read from file?
		size_t offset = parseWaveHeader(file, header, data);
		if (offset != std::string::npos) {
			//util::hexout("TPCMFile::readMetaData() PCM data offset for file <" + fileName + "> = ", offset);

			util::TTimePart sec, ms;
			size_t frameSize, samples;

			// Content of TPCMBaseHeader
			// ================================================
			//
			// DWORD	pfChunkID;			// Offset 0  "RIFF"
			// DWORD	pfChunkSize;		// Offset 4
			// DWORD	pfFormat;			// Offset 8  "WAVE"
			// DWORD	pfSubchunk1ID;		// Offset 12 "fmt "
			// DWORD	pfSubchunk1Size;	// Offset 16
			// WORD		pfAudioFormat;		// Offset 20
			// WORD		pfNumChannels;		// Offset 22
			// DWORD	pfSampleRate;		// Offset 24
			// DWORD	pfByteRate;			// Offset 28
			// WORD		pfBlockAlign;		// Offset 32
			// WORD		pfBitsPerSample;	// Offset 34
			//
			// Content of TPCMChunkHeader
			// ================================================
			//
			// DWORD	pfChunkID;			// Offset 0  "data"
			// DWORD	pfChunkSize;		// Offset 4
			//

			// Info is crap, escape divide-by-zero
			if (header.pfSampleRate <= 0 || data.pfChunkSize <= 0 || header.pfNumChannels <= 0 || header.pfBitsPerSample <= 0) {
				tag.error = util::csnprintf("Missing or zeroed sound properties: NumChannels=%, ChunkSize=%, BitsPerSample=%, SampleRate=%", header.pfNumChannels, data.pfChunkSize, header.pfBitsPerSample, header.pfSampleRate);
				return false;
			}

			// Calculate frame size, sample count and duration
			frameSize = header.pfNumChannels * header.pfBitsPerSample / 8;
			samples = data.pfChunkSize / frameSize;
			ms = (util::TTimePart)(samples * 1000 / header.pfSampleRate);
			sec = ms / 1000;

			// Time is crap, escape divide-by-zero
			if (ms <= 0 || sec <= 0) {
				tag.error = "Invalid song duration in metadata";
				return false;
			}

			// Get stream information
			tag.stream.duration = ms;
			tag.stream.seconds = sec;
			tag.stream.channels = header.pfNumChannels;
			tag.stream.sampleRate = header.pfSampleRate;
			tag.stream.bitsPerSample = normalizeSampleSize(header.pfBitsPerSample);
			tag.stream.bytesPerSample = tag.stream.bitsPerSample / 8;
			tag.stream.sampleCount = samples; // Simple sample count for all channels
			tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.bytesPerSample * tag.stream.channels; // Storage size for all samples in bytes
			tag.stream.bitRate = bitRateFromTags(tag);

			// Estimated buffer size for frame decoder, cut at audio word border
			// tag.stream.chunkSize = AUDIO_CHUNK_SIZE;
			tag.stream.chunkSize = util::align(AUDIO_CHUNK_SIZE, tag.stream.channels * tag.stream.bytesPerSample);

			// Check if ID3 tags appended after sound chunk
			bool found = false;
			size_t metadataoffset = offset + data.pfChunkSize;
			if (offset < file.getSize() ) {
				ssize_t r = 0;
				size_t offsetToID3Tag = 0;

				// Read ID3 chunk header
				int retry = 0;
				util::TFileScanner parser;
				app::TStringVector pattern = {"ID3"};
				offset = metadataoffset;

				// Retry to skip RIFF container tags(s)
				do {
					//util::hexout("TPCMFile::readMetaData() Scan ID3 metadata at offset = ", offset);
					parser.scan(file, offset, pattern, 512);

					// Check if ID3 pattern found
					offsetToID3Tag = parser["ID3"];
					if (offsetToID3Tag != std::string::npos) {
						//util::hexout("TPCMFile::readMetaData() Offset to ID3 tag = ", offsetToID3Tag);
						r = readTags(file, offsetToID3Tag, tag.meta);
						if (r < 0) {
							++retry;
							offset = offsetToID3Tag + 4;
						} else {
							found = true;
						}

					}

				} while (!found && r < 0 && offsetToID3Tag != std::string::npos && retry > 0 && retry < 2);
			}

			// Check for native RIFF metadata
			if (!found) {
				TRIFFAtomScanner parser;
				offset = metadataoffset;
				//util::hexout("TPCMFile::readMetaData() Scan RIFF metadata at offset = ", offset);
				if (parser.scan(file, offset) > 0) {
					parser.getTags(tag.meta);
				}
			}

        	if (!tag.stream.isValid()) {
				tag.stream.addErrorHint(tag.error);
				tag.stream.debugOutput(" PCM: ");
				tag.meta.debugOutput(" META: ");
				return false;
        	}

        	return true;
		}
		tag.error = "Invalid WAVE file, size too small";
	}
	tag.error = "WAVE file not readable";
	return false;
}


bool TPCMFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read PCM header from file
		TPCMBaseHeader header;
		TPCMChunkHeader data;

		// Complete header read from file?
		size_t offset = parseWaveHeader(file, header, data);
		if (offset != std::string::npos) {

			// Content of TPCMBaseHeader
			// ================================================
			//
			// DWORD	pfChunkID;			// Offset 0  "RIFF"
			// DWORD	pfChunkSize;		// Offset 4
			// DWORD	pfFormat;			// Offset 8  "WAVE"
			// DWORD	pfSubchunk1ID;		// Offset 12 "fmt "
			// DWORD	pfSubchunk1Size;	// Offset 16
			// WORD		pfAudioFormat;		// Offset 20
			// WORD		pfNumChannels;		// Offset 22
			// DWORD	pfSampleRate;		// Offset 24
			// DWORD	pfByteRate;			// Offset 28
			// WORD		pfBlockAlign;		// Offset 32
			// WORD		pfBitsPerSample;	// Offset 34
			//
			// Content of TPCMChunkHeader
			// ================================================
			//
			// DWORD	pfChunkID;			// Offset 0  "data"
			// DWORD	pfChunkSize;		// Offset 4
			//

			// Check if ID3 tags appended after sound chunk
			size_t offsetToID3Tag = 0;
			offset += data.pfChunkSize;
			if (offset < file.getSize() ) {
				ssize_t r = 0;
				bool found = false;

				// Read ID3 chunk header
				int retry = 0;
				util::TFileScanner parser;
				app::TStringVector pattern = {"ID3"};

				// Retry to skip RIFF container tags(s)
				do {
					//util::hexout("TPCMFile::readMetaData() Scan ID3 metadata at offset = ", offset);
					parser.scan(file, offset, pattern, 512);

					// Check if ID3 pattern found
					offsetToID3Tag = parser["ID3"];
					if (offsetToID3Tag != std::string::npos) {
						//util::hexout("TPCMFile::readMetaData() Offset to ID3 tag = ", offsetToID3Tag);
						CMetaData meta;
						r = readTags(file, offsetToID3Tag, meta, cover, ETL_PICTURE);
						if (r < 0) {
							++retry;
							offset = offsetToID3Tag + 4;
						} else {
							found = true;
						}

					}

				} while (!found && r < 0 && offsetToID3Tag != std::string::npos && retry > 0 && retry < 2);

			}

			return cover.isValid();

		}
	}

	return false;
}


} /* namespace music */
