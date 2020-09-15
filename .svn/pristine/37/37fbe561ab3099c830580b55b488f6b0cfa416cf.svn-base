/*
 * aiff.cpp
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#include "gcc.h"
#include "endian.h"
#include "IEEE754.h"
#include "audiobuffer.h"
#include "audioconsts.h"
#include "audiotypes.h"
#include "audiofile.h"
#include "aiff.h"

namespace music {

TAIFFStream::TAIFFStream() {
}

TAIFFStream::~TAIFFStream() {
}


void TAIFFStream::setFileHeaderEndian(TAIFFFileHeader& header) const {
	// TChunkID	aiChunkID;			// Offset 0  "FORM"
	// DWORD	aiChunkSize;		// Offset 4
	// TChunkID	aiFormType;			// Offset 8  "AIFF"
#ifdef TARGET_LITTLE_ENDIAN
	header.aiChunkSize = convertFromBigEndian32(header.aiChunkSize);
#endif
}

void TAIFFStream::setCommonChunkEndian(TAIFFCommonChunk& common) const {
	// TChunkID			aiChunkID;			// Offset 0  "COMM"
	// DWORD			aiChunkSize;		// Offset 4  = 18
	// INT16			aiNumChannels;		// Offset 6	 2 Byte
	// DWORD			aiNumSampleFrames;	// Offset 10 4 Byte
	// INT16 			aiSampleSize;		// Offset 14 2 Byte
	// util::TIEEE754 	aiSampleRate;		// Offset 16 10 Byte --> 18 Byte
#ifdef TARGET_LITTLE_ENDIAN
	common.aiChunkSize = convertFromBigEndian32(common.aiChunkSize);
	common.aiNumChannels = convertFromBigEndian16(common.aiNumChannels);
	common.aiNumSampleFrames = convertFromBigEndian32(common.aiNumSampleFrames);
	common.aiSampleSize = convertFromBigEndian16(common.aiSampleSize);
#endif
}

void TAIFFStream::setSoundChunkEndian(CAIFFSoundChunk& sound) const {
	// TChunkID		aiChunkID;			// Offset 0  "SSND"
	// DWORD		aiChunkSize;		// Offset 4
	// DWORD		aiOffset;			// Offset 8  4 Byte
	// DWORD		aiBlockSize;		// Offset 12 4 Byte
	// BYTE			aiChunk[];			// Offset 16
#ifdef TARGET_LITTLE_ENDIAN
	sound.aiChunkSize = convertFromBigEndian32(sound.aiChunkSize);
	sound.aiOffset = convertFromBigEndian32(sound.aiOffset);
	sound.aiBlockSize = convertFromBigEndian32(sound.aiBlockSize);
#endif
}



TAIFFDecoder::TAIFFDecoder() {
	debug = false;
}

TAIFFDecoder::~TAIFFDecoder() {
}


bool TAIFFDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TAIFFDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TAIFFDecoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TAIFFDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TAIFFDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {
				// Position file reader to first data byte after Sound chunk file header
				size_t hsize = sizeof(TAIFFFileHeader);
				util::TFileScanner parser;
				app::TStringVector pattern = {"SSND"};
				parser.scan(file, hsize, pattern, 128);

				// Sound chunk header found?
				size_t offset = parser["SSND"];
				if (std::string::npos != offset) {
					// Position to first byte after sound chunk header
					offset += sizeof(TAIFFSoundChunk);
					if (debug) util::hexout("TPCMDecoder::open() Data offset for file \"" + fileName + "\" = ", offset);
				} else {
					error = true;
				}

				// Is file big enough for seek position?
				if (!error) {
					if (offset > file.getSize())
						error = true;
				}

				// Seek to position and check resulting position
				if (!error) {
					size_t r = file.seek(offset);
					if (r != offset)
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
			}
			return opened;
		}
	}
	setRead(0);
	return false;
}


bool TAIFFDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;
	bool warning = false;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			ssize_t r = 0;
			PSample samples = buffer->writer();

			// Resize sample buffer if needed
			if (chunk.size() != song->getChunkSize()) {
				chunk.resize(song->getChunkSize(), false);
				if (debug) std::cout << "TAIFFDecoder::update() Resize chunk to " << song->getChunkSize() << " bytes." << std::endl;
			}

			// File reader may throw exception...
			try {
				r = file.read(chunk.data(), song->getChunkSize());
				if (r > 0) {

					// Check if all audio data is read
					// --> Ignore chunk at end of file (MP3Tags, etc.)
					if ((getRead() + (size_t)r) >= song->getSampleSize()) {
						if (debug) {
							std::cout << "TAIFFDecoder::update() EOF while chunk read:" << std::endl;
							std::cout << "TAIFFDecoder::update()   Read from file : " << r << std::endl;
							std::cout << "TAIFFDecoder::update()   Read until now : " << getRead() << std::endl;
							std::cout << "TAIFFDecoder::update()   Sample size    : " << song->getSampleSize() << std::endl;
							std::cout << "TAIFFDecoder::update()   Total read     : " << getRead() + r << std::endl;
						}
						r = song->getSampleSize() - getRead();
						eof = true;
						if (debug) {
							std::cout << "TAIFFDecoder::update()   Corrected size : " << r << std::endl;
						}
					}

					// Set current file reader position
					read = r;
					addRead(read);

					// Convert big endian chunk to little endian PCM data
					// Remember that ALSA is set to use little endian PCM by application parameters!
					// --> Set write buffer pointer to current position after successful conversion
					size_t written = convertToLittleEndian(chunk.data(), samples, read, song->getBitsPerSample());
					buffer->write(written);

					// Check for successful endian conversion
					if (read != written) {
						if (!warning) {
							if (debug) std::cout << "TAIFFDecoder::update() Expected size " << read << " != written size " << written << " (" << song->getBitsPerSample() << " bits)" << std::endl;
							warning = true;
						}
					} else {
						warning = false;
					}

				} else {
					eof = true;
				}

			} catch (...) {
				error = true;
			}

			// Detect End Of File
			if (!error && !eof && r < (ssize_t)song->getChunkSize())
				eof = true;

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				 std::cout << "TAIFFDecoder::update() EOF detected." << std::endl;
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

size_t TAIFFDecoder::convertToLittleEndian(const TSample* aiff, TSample* pcm, size_t size, int bitsPerSample) {
	size_t i = 0, written = 0;
	TSample L1, L2, L3, R1, R2, R3;
	while (i < size) {

		// Read big endian aligned bytes from AIFF buffer
		switch (bitsPerSample) {
			case 8:
				// Read/write 1 byte
				*pcm++ = *aiff++;

				// Increment read/write counter
				++i;
				++written;

				break;

			case 16:
				// Read 2 bytes for each stereo (!) channel
				L1 = *aiff++;
				L2 = *aiff++;
				R1 = *aiff++;
				R2 = *aiff++;

				// Reverse byte order in PCM stream
				*pcm++ = L2;
				*pcm++ = L1;
				*pcm++ = R2;
				*pcm++ = R1;

				// Increment read/write counter
				i += 2;
				written += 2;

				break;

			case 24:
				// Read 3 bytes for each stereo (!) channel
				L1 = *aiff++;
				L2 = *aiff++;
				L3 = *aiff++;
				R1 = *aiff++;
				R2 = *aiff++;
				R3 = *aiff++;

				// Reverse byte order in PCM stream
				*pcm++ = L3;
				*pcm++ = L2;
				*pcm++ = L1;
				*pcm++ = R3;
				*pcm++ = R2;
				*pcm++ = R1;

				// Increment read/write counter
				i += 3;
				written += 3;

				break;

			default:
				break;
		}

	}
	return written;
}

void TAIFFDecoder::close() {
	clear();
	file.close();
	if (debug)
		 std::cout << "TAIFFDecoder::close()" << std::endl;
}



TAIFFFile::TAIFFFile() : TSong(EFT_AIFF) {
	prime();
}

TAIFFFile::TAIFFFile(const std::string& fileName) : TSong(fileName, EFT_AIFF) {
	prime();
}

TAIFFFile::~TAIFFFile() {
}

void TAIFFFile::prime() {
	offsetToID3Tag = 0;
	sizeOfID3Tag = 0;
}

void TAIFFFile::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TAIFFDecoder;
}

bool TAIFFFile::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

bool TAIFFFile::isAIFFFile(const TAIFFFileHeader& header) const {

	// Check "magic" header bytes
	if (!isChunkID(header.aiChunkID, "FORM    "))
		return false;
	if (!isChunkID(header.aiFormType, "AIFF    "))
		return false;

	// Check for valid chunk sizes
	if (header.aiChunkSize <= 0)
		return false;

	return true;
}

bool TAIFFFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TAIFFFileHeader header;
		TAIFFCommonChunk common;
		TAIFFSoundChunk sound;
		size_t hsize = sizeof(TAIFFFileHeader);
		size_t csize = sizeof(TAIFFCommonChunk);
		size_t ssize = sizeof(TAIFFSoundChunk);
		size_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == hsize) {

			// Check for valid WAVE/PCM file
			if (isAIFFFile(header)) {
				util::TTimePart sec, ms;

				// Correct endianess
				setFileHeaderEndian(header);

				// Content of AIFF data chunks
				// ================================================
				//
				//	typedef struct CAIFFFileHeader {
				//		TChunkID	aiChunkID;			// Offset 0  "FORM"
				//		DWORD		aiChunkSize;		// Offset 4
				//		TChunkID	aiFormType;			// Offset 8  "AIFF"
				//	} PACKED TAIFFFileHeader;
				//
				//
				//	typedef struct CAIFFCommonChunk {
				//		TChunkID		aiChunkID;			// Offset 0  "COMM"
				//		DWORD			aiChunkSize;		// Offset 4  = 18
				//		INT16			aiNumChannels;		// Offset 6	 2 Byte
				//		DWORD			aiNumSampleFrames;	// Offset 10 4 Byte
				//		INT16 			aiSampleSize;		// Offset 14 2 Byte
				//		util::TIEEE754 	aiSampleRate;		// Offset 16 10 Byte --> 18 Byte
				//	} TAIFFCommonChunk;
				//
				//
				//	typedef struct CAIFFSoundChunk {
				//		TChunkID	aiChunkID;			// Offset 0  "SSND"
				//		DWORD		aiChunkSize;		// Offset 4
				//		DWORD		aiOffset;			// Offset 8  4 Byte
				//		DWORD		aiBlockSize;		// Offset 12 4 Byte
				//	} TAIFFSoundChunk;
				//

				// Find chunks in given header chunk size
				// ATTENTION: Order of chunks is not defined, so seek to each position!
				util::TFileScanner parser;
				app::TStringVector pattern = {"COMM", "SSND"};
				if (getDebug()) std::cout << "TAIFFFile::readMetaData() Scan file \"" << file.getFile() << "\"" << std::endl;
				parser.scan(file, hsize, pattern, header.aiChunkSize);

				// Read common chunk header
				size_t offset = parser["COMM"];
				if (!readHeader(file, offset, &common, csize)) {
					tag.error = "No COMM chunk header found";
					return false;
				}
				setCommonChunkEndian(common);

				// Read sound chunk header
				offset = parser["SSND"];
				if (!readHeader(file, offset, &sound, ssize)) {
					tag.error = "No SSND sound description header found";
					return false;
				}
				setSoundChunkEndian(sound);

				// Calculate frame size, sample rate and duration
				double rate;
				size_t sampleRate = 0;
				if (util::convertFromIeeeExtended(common.aiSampleRate, rate)) {
					sampleRate = floor(rate);
				}

				// Info is crap, escape divide-by-zero
				if (common.aiNumChannels <= 0 ||
					common.aiNumSampleFrames <= 0 ||
					common.aiSampleSize <= 0 ||
					sampleRate <= 0) {
					tag.error = util::csnprintf("Missing or zeroed sound properties: NumChannels=%, NumSampleFrames=%, SampleSize=%, SampleRate=%", common.aiNumChannels, common.aiNumSampleFrames, common.aiSampleSize, sampleRate);
					return false;
				}

				ms = (util::TTimePart)((size_t)common.aiNumSampleFrames * (size_t)1000 / sampleRate);
				sec = ms / 1000;

				// Time is crap, escape divide-by-zero
				if (ms <= 0 || sec <= 0) {
					tag.error = "Invalid song duration in metadata";
					return false;
				}

				// Get stream information
				tag.stream.duration = ms;
				tag.stream.seconds = sec;
				tag.stream.channels = common.aiNumChannels;
				tag.stream.sampleRate = sampleRate;
				tag.stream.bitsPerSample = normalizeSampleSize(common.aiSampleSize);
				tag.stream.bytesPerSample = tag.stream.bitsPerSample / 8;
				tag.stream.sampleCount = common.aiNumSampleFrames;
				tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.bytesPerSample * tag.stream.channels; // Storage size for all samples in bytes
				tag.stream.bitRate = bitRateFromTags(tag);

				// Estimated buffer size for frame decoder, cut at audio word limit
				tag.stream.chunkSize = util::align((sound.aiBlockSize > 0) ? sound.aiBlockSize : AUDIO_CHUNK_SIZE, tag.stream.channels * tag.stream.bytesPerSample);

				// Check if ID3 tags appended after sound chunk
				offsetToID3Tag = 0;
				offset = parser["SSND"];
            	if (offset != std::string::npos && offset < file.getSize()) {

					// Check for malformed chunk size
					if (file.getSize() > sound.aiChunkSize && sound.aiChunkSize > 0)
						offset += sound.aiChunkSize + sizeof(TChunkID) + sizeof(DWORD);
					else
						offset = file.getSize() * 75 / 100;

					if (file.getSize() > offset) {
						// Read ID3 chunk header
						parser.clear();
						pattern = {"ID3 "};
						parser.scan(file, offset, pattern, "ID3 ");
						if (!parser.empty()) {
							offset = parser["ID3 "];
							if (offset != std::string::npos) {
								TAIFFID3Chunk id3;
								size_t isize = sizeof(TAIFFID3Chunk);
								if (readHeader(file, offset, &id3, isize)) {
									sizeOfID3Tag = convertFromBigEndian32(id3.aiChunkSize);
									if (sizeOfID3Tag > 0) {
										offsetToID3Tag = offset + isize;
									}
								}
							}
						}
					}

					// Read ID3 tags or set pseudo tags derived from filename and path
					if (offsetToID3Tag > 0) {
						readTags(file, offsetToID3Tag, tag.meta);
					}
            	}

            	if (!tag.stream.isValid()) {
    				tag.stream.addErrorHint(tag.error);
    				tag.stream.debugOutput(" AIFF: ");
    				tag.meta.debugOutput(" META: ");
					return false;
            	}

            	return true;

			}
			tag.error = "Invalid AIFF file header";
		}
		tag.error = "Invalid AIFF file, size too small";
	}
	tag.error = "AIFF file not readable";
	return false;
}

bool TAIFFFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TAIFFFileHeader header;
		TAIFFSoundChunk sound;
		size_t hsize = sizeof(TAIFFFileHeader);
		size_t ssize = sizeof(TAIFFSoundChunk);
		size_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == hsize) {

			// Check for valid WAVE/PCM file
			if (isAIFFFile(header)) {

				// Correct endianess
				setFileHeaderEndian(header);

				// Content of AIFF data chunks
				// ================================================
				//
				//	typedef struct CAIFFFileHeader {
				//		TChunkID	aiChunkID;			// Offset 0  "FORM"
				//		DWORD		aiChunkSize;		// Offset 4
				//		TChunkID	aiFormType;			// Offset 8  "AIFF"
				//	} PACKED TAIFFFileHeader;
				//
				//
				//	typedef struct CAIFFCommonChunk {
				//		TChunkID		aiChunkID;			// Offset 0  "COMM"
				//		DWORD			aiChunkSize;		// Offset 4  = 18
				//		INT16			aiNumChannels;		// Offset 6	 2 Byte
				//		DWORD			aiNumSampleFrames;	// Offset 10 4 Byte
				//		INT16 			aiSampleSize;		// Offset 14 2 Byte
				//		util::TIEEE754 	aiSampleRate;		// Offset 16 10 Byte --> 18 Byte
				//	} TAIFFCommonChunk;
				//
				//
				//	typedef struct CAIFFSoundChunk {
				//		TChunkID	aiChunkID;			// Offset 0  "SSND"
				//		DWORD		aiChunkSize;		// Offset 4
				//		DWORD		aiOffset;			// Offset 8  4 Byte
				//		DWORD		aiBlockSize;		// Offset 12 4 Byte
				//	} TAIFFSoundChunk;
				//

				// Find chunks in given header chunk size
				// ATTENTION: Order of chunks is not defined, so seek to each position!
				util::TFileScanner parser;
				app::TStringVector pattern = {"SSND"};
				parser.scan(file, hsize, pattern, header.aiChunkSize);

				// Check if ID3 tags appended after sound chunk
				offsetToID3Tag = 0;
				size_t offset = parser["SSND"];

            	if (offset != std::string::npos && offset < file.getSize()) {

    				// Read sound chunk header
    				if (!readHeader(file, offset, &sound, ssize))
    					return false;
    				setSoundChunkEndian(sound);

					// Check for malformed chunk size
					if (file.getSize() > sound.aiChunkSize && sound.aiChunkSize > 0)
						offset += sound.aiChunkSize + sizeof(TChunkID) + sizeof(DWORD);
					else
						offset = file.getSize() * 75 / 100;

					if (file.getSize() > offset) {
						// Read ID3 chunk header
						parser.clear();
						pattern = {"ID3 "};
						parser.scan(file, offset, pattern, "ID3 ");
						if (!parser.empty()) {
							offset = parser["ID3 "];
							if (offset != std::string::npos) {
								TAIFFID3Chunk id3;
								size_t isize = sizeof(TAIFFID3Chunk);
								if (readHeader(file, offset, &id3, isize)) {
									sizeOfID3Tag = convertFromBigEndian32(id3.aiChunkSize);
									if (sizeOfID3Tag > 0) {
										offsetToID3Tag = offset + isize;
									}
								}
							}
						}
					}

					// Read picture data from file
					if (offsetToID3Tag > 0) {
						CMetaData meta;
						readTags(file, offsetToID3Tag, meta, cover, ETL_PICTURE);
					}
            	}

				return cover.isValid();

			}
		}
	}

	return false;
}


bool TAIFFFile::readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size) {
	if (offset != std::string::npos && offset < file.getSize()) {
		size_t r = file.seek(offset, util::SO_FROM_START);
		if (r == offset) {
			r = file.read((char*)buffer, size, 0, util::SO_FROM_START);
			return (r == size);
		}
	}
	return false;
}


} /* namespace music */
