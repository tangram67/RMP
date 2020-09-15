/*
 * dsd.cpp
 *
 *  Created on: 15.09.2016
 *      Author: Dirk Brinkmeier
 */

#include <cstring>
#include "fileutils.h"
#include "templates.h"
#include "stringutils.h"
#include "audiobuffer.h"
#include "audioconsts.h"
#include "audiotypes.h"
#include "audiofile.h"
#include "bits.h"
#include "gcc.h"
#include "dsd.h"

namespace music {



TDSDStream::TDSDStream() {
}

TDSDStream::~TDSDStream() {
}

TSample TDSDStream::reverse(TSample sample, int bitsPerSample) {
	if (bitsPerSample > ES_DSD_NE)
		return sample;
	return util::bitReverse(sample);
}

size_t TDSDStream::convertDSDtoDoP(const TSample* dsd, TSample* dop, size_t size, size_t offset, int bitsPerSample) {
	size_t i = 0, idx = 0, written = 0;
	TSample LC1, RC1, LC2, RC2;
	const TSample* p = dsd;
	while (i < size) {

		// Read first L/R channel frame from DSD data
		if (offset > 1) {
			LC1 = reverse(dsd[idx], bitsPerSample);
			RC1 = reverse(dsd[idx + offset], bitsPerSample);
			++idx;
		} else {
			LC1 = reverse(*p++, bitsPerSample);
			RC1 = reverse(*p++, bitsPerSample);
		}

		// Read second L/R channel frame from DSD data
		if (offset > 1) {
			LC2 = reverse(dsd[idx], bitsPerSample);
			RC2 = reverse(dsd[idx + offset], bitsPerSample);
			++idx;
		} else {
			LC2 = reverse(*p++, bitsPerSample);
			RC2 = reverse(*p++, bitsPerSample);
		}

		// Write 2 frames for left channel
		*dop++ = LC2;
		*dop++ = LC1;

		// Write 2 frames for right channel
		*dop++ = RC2;
		*dop++ = RC1;

		// 4 samples were read and 4 bytes written
		i += 4; // * AUDIO_SAMPLE_SIZE;
		written += 4;
	}
	return written;
}



TDSFStream::TDSFStream() {
}

TDSFStream::~TDSFStream() {
}

void TDSFStream::setDSFFileHeaderEndian(TDSFFileHeader& header) const {
#ifdef TARGET_BIG_ENDIAN
	setDSFHeaderEndian(header.header);
	setDSFFormatEndian(header.format);
	setDSFDataEndian(header.data);
#endif
}

void TDSFStream::setDSFHeaderEndian(TDSFHeader& header) const {
	//	TChunkID	dsChunkID;			// Offset 0  "DSD "
	//	QWORD		dsChunkSize;		// Offset 4	 = 28 Byte!
	//	QWORD		dsFileSize;			// Offset 12
	//	QWORD		dsOffsetToID3Tag;	// Offset 20
#ifdef TARGET_BIG_ENDIAN
	header.dsChunkSize = convertFromLittleEndian64(header.dsChunkSize);
	header.dsFileSize = convertFromLittleEndian64(header.dsFileSize);
	header.dsOffsetToID3Tag = convertFromLittleEndian64(header.dsOffsetToID3Tag);
#endif
}

void TDSFStream::setDSFFormatEndian(TDSFFormat& format) const {
	//	TChunkID	dsChunkID;			// Offset 0  "fmt "
	//	QWORD		dsChunkSize;		// Offset 4	 = 52 Byte!
	//	DWORD		dsVersion;			// Offset 12 = 1
	//	DWORD		dsFormatID;			// Offset 16 = 0: DSD raw
	//	DWORD		dsChannelType;		// Offset 20 = 2: stereo
	//	DWORD		dsChannelNum;		// Offset 24 = 2: stereo
	//	DWORD		dsSampleRate;		// Offset 28 = 2822400, 5644800
	//	DWORD		dsBitsPerSample;	// Offset 32 = 1, 8
	//	QWORD		dsSampleCount;		// Offset 36 = number per channel
	//	DWORD		dsBlockSize;		// Offset 44 = 4096
	//	DWORD		dsReserved;			// Offset 48 = 0
#ifdef TARGET_BIG_ENDIAN
	format.dsChunkSize = convertFromLittleEndian64(format.dsChunkSize);
	format.dsVersion = convertFromLittleEndian32(format.dsVersion);
	format.dsFormatID = convertFromLittleEndian32(format.dsFormatID);
	format.dsChannelType = convertFromLittleEndian32(format.dsChannelType);
	format.dsChannelNum = convertFromLittleEndian32(format.dsChannelNum);
	format.dsSampleRate = convertFromLittleEndian32(format.dsSampleRate);
	format.dsBitsPerSample = convertFromLittleEndian32(format.dsBitsPerSample);
	format.dsSampleCount = convertFromLittleEndian64(format.dsSampleCount);
	format.dsBlockSize = convertFromLittleEndian32(format.dsBlockSize);
	format.dsReserved = convertFromLittleEndian32(format.dsReserved);
#endif
}

void TDSFStream::setDSFDataEndian(TDSFData& data) const {
	//	TChunkID	dsChunkID;			// Offset 0  "data"
	//	QWORD		dsChunkSize;		// Offset 4
#ifdef TARGET_BIG_ENDIAN
	data.dsChunkSize = convertFromLittleEndian64(data.dsChunkSize);
#endif
}



TDSFDecoder::TDSFDecoder() {
	debug = false;
}

TDSFDecoder::~TDSFDecoder() {
}


bool TDSFDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TDSFDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TDSFDecoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TDSFDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TDSFDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {
				// Position file reader to first data byte after PCM file header
				size_t hsize = sizeof(TDSFFileHeader);

				// Is file big enough for seek position?
				if (hsize > file.getSize())
					error = true;

				// Seek to position and check resulting position
				if (!error) {
					size_t r = file.seek(hsize);
					if (r != hsize)
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


bool TDSFDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			size_t r = 0;
			PSample samples = buffer->writer();

			// Resize simple buffer if needed
			if (chunk.size() != song->getChunkSize()) {
				chunk.resize(song->getChunkSize(), false);
			}

			// File reader may throw exception...
			try {

				r = file.read((char*)chunk.data(), song->getChunkSize());
				if (r > 0) {

					// Check if all audio data is read
					// --> Ignore chunk at end of file (MP3Tags, etc.)
					if ((getRead() + r) >= song->getSampleSize()) {
						if (debug) {
							std::cout << "TDSFDecoder::update() EOF while chunk read:" << std::endl;
							std::cout << "TDSFDecoder::update()   Read from file : " << r << std::endl;
							std::cout << "TDSFDecoder::update()   Read until now : " << getRead() << std::endl;
							std::cout << "TDSFDecoder::update()   Sample size    : " << song->getSampleSize() << std::endl;
							std::cout << "TDSFDecoder::update()   Total read     : " << getRead() + r << std::endl;
						}
						r = song->getSampleSize() - getRead();
						if (debug) std::cout << "TDSFDecoder::update()   Corrected size : " << r << std::endl;
						eof = true;
					}

					// Set current file reader position
					read = r;
					addRead(read);

					// Convert DSD chunk to PCM DoP data:
					// DSF data is build up of alternating 4096 blocks of DSD samples for left and right.
					// Convert the buffer holding 1 block of 4096 DSD left samples and 1 block of 4096 DSD right samples
					// to 8k of samples in normalized PCM left/right order.
					size_t channelOffset = song->getChunkSize() / 2;
					size_t written = convertDSDtoDoP(chunk.data(), samples, read, channelOffset, song->getBitsPerSample());

					// Set write buffer pointer to current position after successful read
					buffer->write(written);

					// Check for successful DoP conversion
					if (read != written) {
						if (debug) std::cout << "TDSFDecoder::update() Expected size " << read << " != written size " << written << std::endl;
					}

				} else {
					eof = true;
				}

			} catch (...) {
				error = true;
			}

			// Detect End Of File
			if (!error && !eof && r < song->getChunkSize())
				eof = true;

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				 std::cout << "TDSFDecoder::update() EOF detected." << std::endl;
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

void TDSFDecoder::close() {
	clear();
	file.close();
	if (debug) std::cout << "TDSFDecoder::close()" << std::endl;
}



TDSFFile::TDSFFile() : TSong(EFT_DSF) {
	prime();
}

TDSFFile::TDSFFile(const std::string& fileName) : TSong(fileName, EFT_DSF) {
	prime();
}

TDSFFile::~TDSFFile() {
}

void TDSFFile::prime() {
	offsetToID3Tag = 0;
	sizeOfID3Tag = 0;
}

void TDSFFile::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TDSFDecoder;
}

bool TDSFFile::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

bool TDSFFile::isDSFFile(const TDSFFileHeader& header) const {

	// Check "magic" header bytes
	if (!isChunkID(header.header.dsChunkID, "DSD   "))
		return false;
	if (!isChunkID(header.format.dsChunkID, "fmt   "))
		return false;
	if (!isChunkID(header.data.dsChunkID, "data  "))
		return false;

	// Check for valid chunk sizes
	if (header.header.dsChunkSize != 28)
		return false;
	if (header.format.dsChunkSize != 52)
		return false;
	if (header.data.dsChunkSize <= 0)
		return false;

	return true;
}

bool TDSFFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TDSFFileHeader header;
		size_t hsize = sizeof(TDSFFileHeader);
		ssize_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == (ssize_t)hsize) {

			// Correct endianess
			setDSFFileHeaderEndian(header);

			// Check for valid DSF file
			if (isDSFFile(header)) {
				util::TTimePart sec, ms;

				// Content of TDSFFileHeader
				// ================================================
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "DSD "
				//		QWORD		dsChunkSize;		// Offset 4	 = 28 Byte!
				//		QWORD		dsFileSize;			// Offset 12
				//		QWORD		dsOffsetToID3Tag;	// Offset 20
				//	} PACKED TDSFHeader;
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "fmt "
				//		QWORD		dsChunkSize;		// Offset 4	 = 52 Byte!
				//		DWORD		dsVersion;			// Offset 12 = 1
				//		DWORD		dsFormatID;			// Offset 16 = 0: DSD raw
				//		DWORD		dsChannelType;		// Offset 20 = 2: stereo
				//		DWORD		dsChannelNum;		// Offset 24 = 2: stereo
				//		DWORD		dsSampleRate;		// Offset 28 = 2822400, 5644800
				//		DWORD		dsBitsPerSample;	// Offset 32 = 1, 8
				//		QWORD		dsSampleCount;		// Offset 36 = number per channel
				//		DWORD		dsBlockSize;		// Offset 44 = 4096
				//		DWORD		dsReserved;			// Offset 48 = 0
				//	} PACKED TDSFFormat;
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "data"
				//		QWORD		dsChunkSize;		// Offset 4
				//	} PACKED TDSFData;

				// Info is crap, escape divide-by-zero
				if (header.format.dsSampleCount <= 0 || header.format.dsSampleRate <= 0) {
					tag.error = util::csnprintf("Missing or zeroed sound properties: SampleCount=%, SampleRate=%", header.format.dsSampleCount, header.format.dsSampleRate);
					return false;
				}

				// Calculate frame size, sample count and duration
				ms = (util::TTimePart)(header.format.dsSampleCount * 1000 / header.format.dsSampleRate);
				sec = ms / 1000;

				// Time is crap, escape divide-by-zero
				if (ms <= 0 || sec <= 0) {
					tag.error = "Invalid song duration in metadata";
					return false;
				}

				// Get stream information
				tag.stream.duration = ms;
				tag.stream.seconds = sec;
				tag.stream.channels = header.format.dsChannelNum;
				tag.stream.sampleRate = header.format.dsSampleRate / header.format.dsChannelNum / 8;
				tag.stream.bitsPerSample = (header.format.dsBitsPerSample > 1) ? ES_DSD_OE : ES_DSD_NE;
				tag.stream.bytesPerSample = 0; // Set to 0 since 1 sample is 1 Bit!
				tag.stream.sampleCount = header.format.dsSampleCount;
				tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.channels / 8; // Storage size for all 1 Bit samples in bytes
				tag.stream.bitRate = bitRateFromTags(tag);

				// Estimated buffer size for frame decoder
				tag.stream.chunkSize = (header.format.dsBlockSize != DSF_CHUNK_SIZE) ?
						header.format.dsBlockSize * header.format.dsChannelNum :
						DSF_CHUNK_SIZE * header.format.dsChannelNum;

				// Read ID3 tags
				offsetToID3Tag = header.header.dsOffsetToID3Tag;
				if (offsetToID3Tag > 0)
					readTags(file, offsetToID3Tag, getTags());

	        	if (!tag.stream.isValid()) {
					tag.stream.addErrorHint(tag.error);
					tag.stream.debugOutput(" DSF: ");
					tag.meta.debugOutput(" META: ");
					return false;
	        	}

	        	return true;

			} else {
				tag.error = "Malformed DSF header or invalid DSF file format \"" + util::TBinaryConvert::binToAsciiA(&header.header.dsChunkID, sizeof(TChunkID)) + "\"";
			}
		}
	}

	return false;
}

bool TDSFFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TDSFFileHeader header;
		size_t hsize = sizeof(TDSFFileHeader);
		ssize_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == (ssize_t)hsize) {

			// Correct endianess
			setDSFFileHeaderEndian(header);

			// Check for valid DSF file
			if (isDSFFile(header)) {

				// Content of TDSFFileHeader
				// ================================================
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "DSD "
				//		QWORD		dsChunkSize;		// Offset 4	 = 28 Byte!
				//		QWORD		dsFileSize;			// Offset 12
				//		QWORD		dsOffsetToID3Tag;	// Offset 20
				//	} PACKED TDSFHeader;
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "fmt "
				//		QWORD		dsChunkSize;		// Offset 4	 = 52 Byte!
				//		DWORD		dsVersion;			// Offset 12 = 1
				//		DWORD		dsFormatID;			// Offset 16 = 0: DSD raw
				//		DWORD		dsChannelType;		// Offset 20 = 2: stereo
				//		DWORD		dsChannelNum;		// Offset 24 = 2: stereo
				//		DWORD		dsSampleRate;		// Offset 28 = 2822400, 5644800
				//		DWORD		dsBitsPerSample;	// Offset 32 = 1, 8
				//		QWORD		dsSampleCount;		// Offset 36 = number per channel
				//		DWORD		dsBlockSize;		// Offset 44 = 4096
				//		DWORD		dsReserved;			// Offset 48 = 0
				//	} PACKED TDSFFormat;
				//
				//	typedef struct {
				//		TChunkID	dsChunkID;			// Offset 0  "data"
				//		QWORD		dsChunkSize;		// Offset 4
				//	} PACKED TDSFData;

				// Info is crap, escape divide-by-zero
				if (header.format.dsSampleCount <= 0 || header.format.dsSampleRate <= 0)
					return false;

				// Read picture data from ID3 tags
				offsetToID3Tag = header.header.dsOffsetToID3Tag;
				if (offsetToID3Tag > 0) {
					CMetaData meta;
					readTags(file, offsetToID3Tag, meta, cover, ETL_PICTURE);
				}

				return cover.isValid();

			}
		}
	}

	return false;
}



TDFFStream::TDFFStream() {
}

TDFFStream::~TDFFStream() {
}

void TDFFStream::setDSFFileHeaderEndian(TDFFFileHeader& header) const {
	//	typedef struct CDFFHeader {
	//		TChunkID	dfChunkID;			// Offset 0  "FRM8"
	//		QWORD		dfChunkSize;		// Offset 4
	//	} PACKED TDFFHeader;
#ifdef TARGET_LITTLE_ENDIAN
	header.dfChunkSize = convertFromBigEndian64(header.dfChunkSize);
#endif
}

void TDFFStream::setDFFFormChunkEndian(TDFFFormChunk& header) const {
	//	typedef struct CDFFFormChunk {
	//		TChunkID	dfChunkID1;			// Offset 0  "FRM8"
	//		QWORD		dfChunkSize;		// Offset 4
	//		TChunkID	dfChunkID2;			// Offset 12 "DSD "
	//	} PACKED TDFFFormChunk;
#ifdef TARGET_LITTLE_ENDIAN
	header.dfChunkSize = convertFromBigEndian64(header.dfChunkSize);
#endif
}

void TDFFStream::setDFFSampleRateEndian(TDFFSampleRate& rate) const {
	//	typedef struct CDFFSampleRate {
	//		TChunkID	dfChunkID;			// Offset 0  "FS "
	//		QWORD		dfChunkSize;		// Offset 4
	//		DWORD		dfSampleRate;		// Offset 12
	//	} PACKED TDFFSampleRate;
#ifdef TARGET_LITTLE_ENDIAN
	rate.dfChunkSize = convertFromBigEndian64(rate.dfChunkSize);
	rate.dfSampleRate = convertFromBigEndian32(rate.dfSampleRate);
#endif
}

void TDFFStream::setDFFChannelsEndian(TDFFChannels& channels) const {
	//	typedef struct CDFFChannels {
	//		TChunkID	dfChunkID;			// Offset 0  "CHNL"
	//		QWORD		dfChunkSize;		// Offset 4
	//		DWORD		dfNumChannels;		// Offset 12
	//	} PACKED TDFFChannels;
#ifdef TARGET_LITTLE_ENDIAN
	channels.dfChunkSize = convertFromBigEndian64(channels.dfChunkSize);
	channels.dfNumChannels = convertFromBigEndian16(channels.dfNumChannels);
#endif
}

void TDFFStream::setDFFSpeakerEndian(TDFFSpeakerConfig& speakers) const {
	//	typedef struct CDFFSpeakerConfig {
	//		TChunkID	dfChunkID;			// Offset 0  "LSCO"
	//		QWORD		dfChunkSize;		// Offset 4
	//	} PACKED TDFFSpeakerConfig;
#ifdef TARGET_LITTLE_ENDIAN
	speakers.dfChunkSize = convertFromBigEndian64(speakers.dfChunkSize);
#endif
}


void TDFFStream::setDFFSoundDataEndian(TDFFSoundDataChunk& data) const {
	//	typedef struct CDFFSoundDataChunk {
	//		TChunkID	dfChunkID;			// Offset 0  "DSD "
	//		QWORD		dfChunkSize;		// Offset 4
	//	} PACKED TDFFSoundDataChunk;
#ifdef TARGET_LITTLE_ENDIAN
	data.dfChunkSize = convertFromBigEndian64(data.dfChunkSize);
#endif
}



TDFFDecoder::TDFFDecoder() {
	debug = true;
}

TDFFDecoder::~TDFFDecoder() {
}


bool TDFFDecoder::open(const TDecoderParams& params) {
	return false;
}

bool TDFFDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TDFFDecoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TDFFDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened) {
		if (properties.isValid()) {
			file.assign(fileName);
			file.open(O_RDONLY);
			opened = file.isOpen();
			if (debug) std::cout << "TDFFDecoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
			if (opened) {
				// Position file reader to first data byte after format chunk header
				size_t hsize = sizeof(TDFFFormChunk);
				size_t offset;
				ssize_t r;

				// Find DSD sound data chunk
				// ATTENTION: Order of chunks is not defined, so seek to each position!
				// REMEMBER: "DSD " is not unique, so stop on first location
				util::TFileScanner parser;
				app::TStringVector pattern = {"LSCO"};
				if (parser.scan(file, hsize, pattern, "DSD ") <= 0)
					return false;

				// Scan for DSD sound data chunk:
				// If speaker config found, start from there
				// Otherwise assume offset to start from
				pattern = {"DSD "};
				offset = parser["LSCO"];
				if (offset != std::string::npos && offset < file.getSize()) {
					r = parser.scan(file, offset, pattern, 1024);
				} else {
					r = parser.scan(file, 0x70, pattern, 1024);
				}

				// No DSD sound data chunk found?
				offset = parser["DSD "];
				if (r <= 0 || offset == std::string::npos || offset >= file.getSize()) {
					error = true;
				}

				// Seek to position and check resulting position
				if (!error) {
					r = file.seek(offset);
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
			}
			return opened;
		}
	}
	setRead(0);
	return false;
}


bool TDFFDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			size_t r = 0;
			PSample samples = buffer->writer();

			// Resize simple buffer if needed
			if (chunk.size() != song->getChunkSize()) {
				chunk.resize(song->getChunkSize(), false);
			}

			// File reader may throw exception...
			try {

				r = file.read((char*)chunk.data(), song->getChunkSize());
				if (r > 0) {

					// Check if all audio data is read
					// --> Ignore chunk at end of file (MP3Tags, etc.)
					if ((getRead() + r) >= song->getSampleSize()) {
						if (debug) {
							std::cout << "TDFFDecoder::update() EOF while chunk read:" << std::endl;
							std::cout << "TDFFDecoder::update()   Read from file : " << r << std::endl;
							std::cout << "TDFFDecoder::update()   Read until now : " << getRead() << std::endl;
							std::cout << "TDFFDecoder::update()   Sample size    : " << song->getSampleSize() << std::endl;
							std::cout << "TDFFDecoder::update()   Total read     : " << getRead() + r << std::endl;
						}
						r = song->getSampleSize() - getRead();
						if (debug) std::cout << "TDFFDecoder::update()   Corrected size : " << r << std::endl;
						eof = true;
					}

					// Set current file reader position
					read = r;
					addRead(read);

					// Convert DSD chunk to PCM DoP data
					size_t written = convertDSDtoDoP(chunk.data(), samples, read, 1, song->getBitsPerSample());

					// Set write buffer pointer to current position after successful read
					buffer->write(written);

					// Check for successful DoP conversion
					if (read != written) {
						if (debug) std::cout << "TDFFDecoder::update() Expected size " << read << " != written size " << written << std::endl;
					}

				} else {
					eof = true;
				}

			} catch (...) {
				error = true;
			}

			// Detect End Of File
			if (!error && !eof && r < song->getChunkSize())
				eof = true;

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();

			if (debug && eof) {
				 std::cout << "TDFFDecoder::update() EOF detected." << std::endl;
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

void TDFFDecoder::close() {
	clear();
	file.close();
	if (debug) std::cout << "TDFFDecoder::close()" << std::endl;
}



TDFFFile::TDFFFile() : TSong(EFT_DFF) {
	prime();
}

TDFFFile::TDFFFile(const std::string& fileName) : TSong(fileName, EFT_DFF) {
	prime();
}

TDFFFile::~TDFFFile() {
}

void TDFFFile::prime() {
	offsetToID3Tag = 0;
	sizeOfID3Tag = 0;
}

void TDFFFile::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TDFFDecoder;
}

bool TDFFFile::isChunkID(const TChunkID& value, const char* ID) const {
	return 0 == strncmp(value, ID, sizeof(TChunkID));
}

bool TDFFFile::isDFFFile(const TDFFFormChunk& header) const {

	// Check "magic" header bytes
	if (!isChunkID(header.dfChunkID1, "FRM8  "))
		return false;
	if (!isChunkID(header.dfChunkID2, "DSD   "))
		return false;

	// Check for valid chunk size
	if (header.dfChunkSize <= 4)
		return false;

	return true;
}

bool TDFFFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TDFFFormChunk header;
		size_t offset, hsize = sizeof(TDFFFormChunk);
		ssize_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == (ssize_t)hsize) {

			// Correct endianess
			setDFFFormChunkEndian(header);

			// Check for valid DFF file
			if (isDFFFile(header)) {
				util::TTimePart sec, ms;
				TDFFSampleRate rate;
				TDFFChannels channels;
				TDFFSpeakerConfig speakers;
				TDFFSoundDataChunk data;

				// Find chunks in given header chunk size
				// ATTENTION: Order of chunks is not defined, so seek to each position!
				// REMEMBER: "DSD " is not unique, so stop on first location
				util::TFileScanner parser;
				app::TStringVector pattern = {"FVER", "SND ", "FS  ", "CHNL", "CMPR", "LSCO"};
				if (parser.scan(file, hsize, pattern, "DSD ") <= 0) {
					tag.error = "No DSD chunk found in metadata";
					return false;
				}

				// Scan for DSD sound data chunk:
				// If speaker config found, start from there
				// Otherwise assume offset to start from
				pattern = {"DSD "};
				offset = parser["LSCO"];
				if (readHeader(file, offset, &speakers, sizeof(TDFFSpeakerConfig))) {
					r = parser.scan(file, offset, pattern, 1024);
				} else {
					r = parser.scan(file, 0x70, pattern, 1024);
				}

				// No DSD sound data chunk found?
				offset = parser["DSD "];
				if (r <= 0 || offset == std::string::npos) {
					tag.error = "No DSD chunk found in LSCO sound chunk";
					return false;
				}

				// Read sample rate chunk
				offset = parser["FS  "];
				if (!readHeader(file, offset, &rate, sizeof(TDFFSampleRate))) {
					tag.error = "No FS chunk found in metadata";
					return false;
				}
				setDFFSampleRateEndian(rate);

				// Read channles chunk
				offset = parser["CHNL"];
				if (!readHeader(file, offset, &channels, sizeof(TDFFChannels))) {
					tag.error = "No CHNL chunk found in metadata";
					return false;
				}
				setDFFChannelsEndian(channels);

				// Read DSD sound data chunk
				offsetToID3Tag = offset = parser["DSD "];
				if (!readHeader(file, offset, &data, sizeof(TDFFSoundDataChunk))) {
					tag.error = "No DSD sound data found";
					return false;
				}
				setDFFSoundDataEndian(data);

				// Info is crap, escape divide-by-zero
				if (data.dfChunkSize <= 0 || rate.dfSampleRate <= 0 || channels.dfNumChannels <= 0) {
					tag.error = util::csnprintf("Missing or zeroed sound properties: ChunkSize=%, SampleRate=%, NumChannels=%", data.dfChunkSize, rate.dfSampleRate, channels.dfNumChannels);
					return false;
				}

				// Get basic stream information
				tag.stream.channels = channels.dfNumChannels;
				tag.stream.sampleSize = data.dfChunkSize;
				tag.stream.sampleCount = tag.stream.sampleSize * 8 / tag.stream.channels;
				tag.stream.sampleRate = rate.dfSampleRate / tag.stream.channels / 8;

				// Calculate duration
				ms = (util::TTimePart)(tag.stream.sampleCount * 1000 / rate.dfSampleRate);
				sec = ms / 1000;

				// Time is crap, escape divide-by-zero
				if (ms <= 0 || sec <= 0) {
					tag.error = "Invalid song duration in metadata";
					return false;
				}

				// Set extended stream information
				tag.stream.duration = ms;
				tag.stream.seconds = sec;
				tag.stream.bitsPerSample = ES_DSD_OE;
				tag.stream.bytesPerSample = 0; // Set to 0 since 1 sample is 1 Bit!
				tag.stream.bitRate = bitRateFromTags(tag);

				// Estimated buffer size for frame decoder
				tag.stream.chunkSize = AUDIO_CHUNK_SIZE;

				// Read ID3 tags after last byte of DSD sound data chunk
				offset = offsetToID3Tag + data.dfChunkSize;
				offsetToID3Tag = 0;
				if (offset < file.getSize()) {
					parser.clear();
					pattern = {"ID3 "};
					if (parser.scan(file, offset, pattern, tag.stream.chunkSize) > 0) {
						TDFFID3Header id3;
						offset = parser["ID3 "];
						if (readHeader(file, offset, &id3, sizeof(TDFFID3Header))) {
							offsetToID3Tag = offset + sizeof(TChunkID) + sizeof(QWORD);
						}
					}
				}
				if (offsetToID3Tag > 0)
					readTags(file, offsetToID3Tag, getTags());

	        	if (!tag.stream.isValid()) {
					tag.stream.addErrorHint(tag.error);
					tag.stream.debugOutput(" DFF: ");
					tag.meta.debugOutput(" META: ");
					return false;
	        	}

	        	return true;

			} else {
				tag.error = "Malformed DFF header or invalid DFF file format \"" + util::TBinaryConvert::binToAsciiA(&header.dfChunkID1, sizeof(TChunkID)) + "\"";
			}
		}
	}

	return false;
}

bool TDFFFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {

		// Read DSF header from file
		TDFFFormChunk header;
		size_t offset, hsize = sizeof(TDFFFormChunk);
		ssize_t r = file.read((char*)&header, hsize);

		// Complete header read from file?
		if (r == (ssize_t)hsize) {

			// Correct endianess
			setDFFFormChunkEndian(header);

			// Check for valid DFF file
			if (isDFFFile(header)) {
				TDFFSpeakerConfig speakers;
				TDFFSoundDataChunk data;

				// Find chunks in given header chunk size
				// ATTENTION: Order of chunks is not defined, so seek to each position!
				// REMEMBER: "DSD " is not unique, so stop on first location
				util::TFileScanner parser;
				app::TStringVector pattern = {"FVER", "SND ", "FS  ", "CHNL", "CMPR", "LSCO"};
				if (parser.scan(file, hsize, pattern, "DSD ") <= 0)
					return false;

				// Scan for DSD sound data chunk:
				// If speaker config found, start from there
				// Otherwise assume offset to start from
				pattern = {"DSD "};
				offset = parser["LSCO"];
				if (readHeader(file, offset, &speakers, sizeof(TDFFSpeakerConfig))) {
					r = parser.scan(file, offset, pattern, 1024);
				} else {
					r = parser.scan(file, 0x70, pattern, 1024);
				}

				// No DSD sound data chunk found?
				offset = parser["DSD "];
				if (r <= 0 || offset == std::string::npos)
					return false;

				// Read DSD sound data chunk
				offsetToID3Tag = offset = parser["DSD "];
				if (!readHeader(file, offset, &data, sizeof(TDFFSoundDataChunk)))
					return false;

				// Read ID3 tags after last byte of DSD sound data chunk
				offset = offsetToID3Tag + data.dfChunkSize;
				offsetToID3Tag = 0;
				if (offset < file.getSize()) {
					parser.clear();
					pattern = {"ID3 "};
					if (parser.scan(file, offset, pattern, AUDIO_CHUNK_SIZE) > 0) {
						TDFFID3Header id3;
						offset = parser["ID3 "];
						if (readHeader(file, offset, &id3, sizeof(TDFFID3Header))) {
							offsetToID3Tag = offset + sizeof(TChunkID) + sizeof(QWORD);
						}
					}
				}
				if (offsetToID3Tag > 0) {
					CMetaData meta;
					readTags(file, offsetToID3Tag, meta, cover, ETL_PICTURE);
				}

				return cover.isValid();

			}
		}
	}

	return false;
}


bool TDFFFile::readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size) {
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
