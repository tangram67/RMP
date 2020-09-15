/*
 * mp3.cpp
 *
 *  Created on: 05.11.2016
 *      Author: Dirk Brinkmeier
 */

#include "gcc.h"
#include "audiobuffer.h"
#include "audioconsts.h"
#include "audiotypes.h"
#include "audiofile.h"
#include "mp3.h"

#define MP3_CBR_FRAME_THRESHOLD 128
#define MP3_FRAME_PADDING_SEEK_OFFSET 1152

namespace music {

static bool MP3LibraryInitialized = false;


TMP3Init::TMP3Init() {
	if (!music::MP3LibraryInitialized) {
		int r = mpg123_init();
		music::MP3LibraryInitialized = (r == MPG123_OK);
	}
}

TMP3Init::~TMP3Init() {
	if (music::MP3LibraryInitialized) {
		mpg123_exit();
		music::MP3LibraryInitialized = false;
	}
}

// See some information at the following URLs:
//   http://id3.org/mp3Frame
//   http://blog.bjrn.se/2008/10/lets-build-mp3-decoder.html
//   http://web.archive.org/web/20080801005127/http://www.devhood.com/tutorials/tutorial_details.aspx?tutorial_id=79
//   http://www.zedwood.com/article/php-calculate-duration-of-mp3
//   https://en.wikipedia.org/wiki/MP3

TMP3FrameReader::TMP3FrameReader() {
	debug = false;
}

TMP3FrameReader::~TMP3FrameReader() {
}

DWORD TMP3FrameReader::convertToMP3Header(const void* buffer) const {
	// Remember that 1 DWORD is 4 bytes long, buffer overflow may happen...
	const BYTE* p = (const BYTE*)buffer;
	return (DWORD)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]));
}

bool TMP3FrameReader::isValidFrame(const TMP3FrameValues& frame) const {
	return (((frame.sync      & 0x7FFu) == 0x7FFu) && // = 0000 0111 1111 1111
			((frame.version    &     3) !=      1) &&
			((frame.layer      &     3) !=      0) &&
			((frame.bitrate    &    15) !=      0) &&
			((frame.bitrate    &    15) !=     15) &&
			((frame.samplerate &     3) !=      3) &&
			((frame.emphasis   &     3) !=      2));
}

bool TMP3FrameReader::getMP3Frame(DWORD header, TMP3FrameHeader& frame) const {
	// Convert header bits to native values
	frame.values.sync       = getFrameSync(header);
	frame.values.version    = getVersionIndex(header);
	frame.values.layer      = getLayerIndex(header);
	frame.values.protection = getProtectionBit(header);
	frame.values.bitrate    = getBitrateIndex(header);
	frame.values.samplerate = getSampleRateIndex(header);
	frame.values.padding    = getPaddingBit(header);
	frame.values.option     = getPrivateBit(header);
	frame.values.mode       = getModeIndex(header);
	frame.values.extmode    = getModeExtIndex(header);
	frame.values.copyright  = getCoprightBit(header);
	frame.values.original   = getOrginalBit(header);
	frame.values.emphasis   = getEmphasisIndex(header);

	// Convert native header values to MP3 stream properties
	if (isValidFrame(frame.values)) {
		frame.stream.channels   = getChannels(frame.values.mode);
		frame.stream.layer      = getMPEGLayer(frame.values.layer);
		frame.stream.version    = getVersionNumber(frame.values.version);
		frame.stream.samplerate = getSampleRate(frame.values.version, frame.values.samplerate);
		frame.stream.bitrate    = getBitrate(frame.values.layer, frame.values.version, frame.values.bitrate);
		frame.stream.framesize  = getFrameSize(frame.values.layer, frame.stream.bitrate, frame.stream.samplerate, frame.values.padding);
		if (debug) debugOutput(frame, "Valid ");
		return true;
	} else {
		frame.stream.channels   = 0;
		frame.stream.layer      = MPEG_UNKNOWN;
		frame.stream.version    = 0;
		frame.stream.samplerate = 0;
		frame.stream.bitrate    = 0;
		frame.stream.framesize  = 0;
		if (debug) debugOutput(frame, "Invalid ");
	}

	return false;
}

bool TMP3FrameReader::getMP3Frame(const void* buffer, TMP3FrameHeader& frame) const {
	DWORD header = convertToMP3Header(buffer);
	if (header > 0xFFE00000u) // = 1111 1111 1110 0000 000...
		return getMP3Frame(header, frame);
	return false;
}


bool TMP3FrameReader::findMP3Frame(const void* buffer, size_t size, off_t& offset) const {
	offset = 0;
	if (size > 4) {
		DWORD header;
		TMP3FrameHeader frame;
		const char* p = (const char*)buffer;
		for (size_t i=0; i<size-4; ++i) {
			header = convertToMP3Header(p);
			if (header > 0xFFE00000u) { // = 1111 1111 1110 0000 000...
				if (getMP3Frame(header, frame)) {
					offset = i;
					return true;
				}
			}
			++p;
		}
	}
	return false;
}


MPEGLayer TMP3FrameReader::getMPEGLayer(int layerIndex) const {
	// Layer index values:
	// 0 0 - Not defined, invalid
	// 0 1 - Layer III
	// 1 0 - Layer II
	// 1 1 - Layer I
	switch (layerIndex) {
	case 1:
		return MP3;
	case 2:
		return MP2;
	case 3:
		return MP1;
	default:
		return MPEG_UNKNOWN;
	}
}

size_t TMP3FrameReader::getFrameSize(int layerIndex, size_t bitRate, size_t sampleRate, bool padding) const {
	off_t offs = padding ? 1 : 0;
	switch (layerIndex) {
		case 1:
			return ((144 * bitRate / sampleRate) + offs);
		case 2:
		case 3:
			return (((12 * bitRate / sampleRate) + offs) * 4);
	}
	return 0;
}

size_t TMP3FrameReader::getBitrate(int layerIndex, int versionIndex, int bitRateIndex) const {
	if (layerIndex > 0)
		return MP3BitRates[versionIndex & 1][layerIndex - 1][bitRateIndex];
	return 0;
}

size_t TMP3FrameReader::getSampleRate(int versionIndex, int sampleRateIndex) const {
    return MP3SampleRates[versionIndex][sampleRateIndex];
}

int TMP3FrameReader::getVersionNumber(int versionIndex) const {
	return MP3Versions[versionIndex];
}

int TMP3FrameReader::getChannels(int modeIndex) const {
	switch(modeIndex) {
		case 0:
		case 1:
		case 2:
			return 2;
		case 3:
			return 1;
		default:
			return 0;
	}
}

std::string TMP3FrameReader::getModeAsString(int modeIndex) const {
	switch(modeIndex) {
		case 0:
			return "Stereo";
		case 1:
			return "Joint Stereo";
		case 2:
			return "Dual Channel";
		case 3:
			return "Monaural";
		default:
			return "<invalid>";
	}
}


int TMP3FrameReader::getFrameSync(DWORD header) const {
	if (debug) std::cout << "TMP3FrameReader::getFrameSync() Sync = " << (header >> 21) << std::endl;
	return (int)((header >> 21) & 0x7FFu);
}

int TMP3FrameReader::getVersionIndex(DWORD header) const {
	return (int)((header >> 19) & 3);
}

int TMP3FrameReader::getLayerIndex(DWORD header) const {
	return (int)((header >> 17) & 3);
}

bool TMP3FrameReader::getProtectionBit(DWORD header) const {
	return ((header >> 16) & 1) > 0;
}

int TMP3FrameReader::getBitrateIndex(DWORD header) const {
	return (int)((header >> 12) & 15);
}

int TMP3FrameReader::getSampleRateIndex(DWORD header) const {
	return (int)((header >> 10) & 3);
}

bool TMP3FrameReader::getPaddingBit(DWORD header) const {
	return ((header >> 9) & 1) > 0;
}

bool TMP3FrameReader::getPrivateBit(DWORD header) const {
	return ((header >> 8) & 1) > 0;
}

int TMP3FrameReader::getModeIndex(DWORD header) const {
	return (int)((header >> 6) & 3);
}

int TMP3FrameReader::getModeExtIndex(DWORD header) const {
	return (int)((header >> 4) & 3);
}

bool TMP3FrameReader::getCoprightBit(DWORD header) const {
	return ((header >> 3) & 1) > 0;
}

bool TMP3FrameReader::getOrginalBit(DWORD header) const {
	return ((header >> 2) & 1) > 0;
}

int TMP3FrameReader::getEmphasisIndex(DWORD header) const {
	return (int)(header & 3);
}


void TMP3FrameReader::debugOutput(const TMP3FrameHeader& frame, const std::string& preamble) const {
	std::cout << preamble << " Binary values:" << std::endl;
	std::cout << preamble << "  Sync              : " << frame.values.sync << std::endl;
	std::cout << preamble << "  Version           : " << frame.values.version << std::endl;
	std::cout << preamble << "  Layer             : " << frame.values.layer << std::endl;
	std::cout << preamble << "  Protected         : " << frame.values.protection << std::endl;
	std::cout << preamble << "  Samplerate        : " << frame.values.samplerate << std::endl;
	std::cout << preamble << "  Padding           : " << frame.values.padding << std::endl;
	std::cout << preamble << "  Mode              : " << frame.values.mode << std::endl;
	std::cout << preamble << std::endl;
	std::cout << preamble << " Stream values:" << std::endl;
	std::cout << preamble << "  Version           : " << frame.stream.version << std::endl;
	std::cout << preamble << "  Layer             : " << frame.stream.layer << std::endl;
	std::cout << preamble << "  Frame size        : " << frame.stream.framesize << " Byte" << std::endl;
	std::cout << preamble << "  Bit rate          : " << frame.stream.bitrate / 1000 << " kBit/s" << std::endl;
	std::cout << preamble << "  Samplerate        : " << util::cprintf("%.1f kS/s", (double)frame.stream.samplerate / 1000.0) << " (" << frame.stream.samplerate << ")" << std::endl;
	std::cout << preamble << "  Channels          : " << frame.stream.channels << " \"" << getModeAsString(frame.values.mode) << "\"" << std::endl;
}



TMP3FrameParser::TMP3FrameParser() {
	debug = false;
	duration = 0;
	sampleSize = 0;
	sampleCount = 0;
	frameCount = 0;
	stream.stream.bitrate = 0;
	stream.stream.channels = 0;
	stream.stream.samplerate = 0;
}

TMP3FrameParser::~TMP3FrameParser() {
}

bool TMP3FrameParser::parse(const util::TFile& file, int bitsPerSample, bool vbr, std::string& hint) {
	if (debug)
		std::cout << "TMP3FrameParser::parse(" << file.getName() << ")" << std::endl;
	hint.clear();
	duration = 0;
	sampleSize = 0;
	sampleCount = 0;
	stream.stream.bitrate = 0;
	stream.stream.channels = 0;
	stream.stream.samplerate = 0;
	TMP3FrameHeader frame;
	TMP3FrameReader reader;
	TID3v2Header header;
	DWORD bytes;
	bool cbr = false;
	size_t bytesPerSecond = 0;
	ssize_t r = readHeader(file, 0, header, hint);
	if (r > 0) {
		// Start with first byte after ID3 tags
		frameCount = 1;
		size_t eof = file.getSize() - 4;
		size_t hsize = r;
		size_t offset = r;
		bool ok;

		// Seek to first position after ID3 tags
		r = file.seek(offset);
		if (r != (ssize_t)offset) {
			if (debug)
				std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") Seeking file pointer behind MP3 tags failed." << std::endl;
			hint = "Seeking file pointer behind MP3 tags failed";
			return false;
		}

		// Read buffer to find first header
		size_t padding = 2 * MP3_FRAME_PADDING_SEEK_OFFSET;
		util::TBuffer buffer(padding);
		r = file.read(buffer.data(), buffer.size(), offset);
		if (r != (ssize_t)buffer.size()) {
			if (debug)
				std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") File to small for valid header." << std::endl;
			hint = util::csnprintf("File corrupted, too small for valid header (%:%)", r, buffer.size());
			return false;
		}

		// Look for first MP3 header
		// --> Skip padding area
		off_t offs;
		if (!reader.findMP3Frame(buffer.data(), buffer.size(), offs)) {
			if (debug)
				std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") No valid MP3 start frame found." << std::endl;
			hint = "No valid MP3 start frame found";
			return false;
		}

		// Set offset to first valid MP3 header
		offset += offs;

		// Walk through MP3 frame headers
		do {
			ok = false;
			r = file.seek(offset);
			if (r == (ssize_t)offset) {
				if (offset < eof) {
					r = file.read(&bytes, 4);
					if (r == 4) {
						// Read MP3 header word
						if (reader.getMP3Frame(&bytes, frame)) {

							// Frame is valid
							ok = true;

							// Offset to next MP3 frame header entry
							offset += frame.stream.framesize;

							// Check current stream params against stored values
							if (frameCount > 1) {
								// Check for change in bitrate:
								// --> Detect VBR file
								if (stream.stream.bitrate != frame.stream.bitrate) {
									cbr = false;
									stream = frame;
									bytesPerSecond = stream.stream.bitrate / 8;
									if (debug)
										reader.debugOutput(stream, "TMP3FrameParser::parse(" + file.getBaseName() + ") Changed ");;
								}

							} else {
								// Take over stream parameters once
								cbr = true;
								stream = frame;
								bytesPerSecond = stream.stream.bitrate / 8;
								if (debug)
									reader.debugOutput(stream, "TMP3FrameParser::parse(" + file.getBaseName() + ") First ");;
							}

							// Calculate duration in milliseconds for current frame:
							// --> Datasize is current framesize
							if (bytesPerSecond > 0)
								duration += 1000 * stream.stream.framesize / bytesPerSecond;

							// Check if file has constant bit rate
							if (!vbr) {
								// No change after 100 frames?
								// --> Assume file is CBR
								if (cbr && frameCount > MP3_CBR_FRAME_THRESHOLD) {
									// Calculate stream duration from file size
									TID3V1TagReader reader;
									bool hasID3V1Tags = reader.hasTags(file);

									// Calculate datasize:
									// Subtract header and MP3v1 tags if present
									size_t dataSize = hasID3V1Tags ? file.getSize() - 128 - hsize : file.getSize() - hsize;

									// Calculate duration in milliseconds:
									//   bytesPerSecond = stream.stream.bitrate / 8
									//   --> * 1000 (for ms) / 8 ==> * 125
									duration = 1000 * dataSize / bytesPerSecond;

									// Calculate sample count from given duration
									// Remember duration is in milliseconds...
									sampleCount = duration * stream.stream.samplerate / 1000;

									// Calculate sample size
									// Assume 16 Bit for now ???
									sampleSize = sampleCount * stream.stream.channels * bitsPerSample / 8;

									if (debug) {
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: File size     : " << file.getSize() << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: Data size     : " << dataSize << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: Bytes per sec : " << bytesPerSecond << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: Duration      : " << TSong::timeToStr(duration / 1000) << " min " << duration << " ms" << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: Sample count  : " << sampleCount << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: Sample size   : " << sampleSize << std::endl;
										std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") CBR: ID3v1 header  : " << hasID3V1Tags << std::endl;
									}

									ok = false;
								}
							}

							// Frame is processed
							if (ok)
								++frameCount;

						} else {
							if (debug) {
								std::string hex = util::TBinaryConvert::binToHexA(&bytes, 4);
								std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") Invalid frame: " << hex << std::endl;
							}
						}
					} else {
						if (debug)
							std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") Read failed (" << r << ")" << std::endl;
					}
				} else {
					if (debug)
						std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") EOF detected." << std::endl;
				}
			} else {
				if (debug)
					std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") Seek failed (" << r << ")" << std::endl;
			}
		} while (ok);

	} else {
		// ID3 header could not be parsed...
		if (debug)
			std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") Invalid ID3 header (" << r << ") : "  << hint << std::endl;
		return false;
	}

	// Check if VBR file detected
	// --> Calculate values afterwards
	if (!cbr && duration > 0) {
		// Calculate sample count from given duration
		// Remember duration is in milliseconds...
		sampleCount = duration * stream.stream.samplerate / 1000;

		// Calculate sample size
		// Assume 16 Bit for now ???
		sampleSize = sampleCount * stream.stream.channels * bitsPerSample / 8;

		if (debug) {
			std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") VBR: File size     : " << file.getSize() << std::endl;
			std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") VBR: Duration      : " << TSong::timeToStr(duration / 1000) << " min " << duration << " ms" << std::endl;
			std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") VBR: Sample count  : " << sampleCount << std::endl;
			std::cout << "TMP3FrameParser::parse(" << file.getBaseName() << ") VBR: Sample size   : " << sampleSize << std::endl;
		}
	}

	if (!isValid()) {
		if (!hint.empty())
			hint += ", ";
		hint += util::csnprintf("Missing or invalid sound properties: Duration=% (5000), SampleSize=% (882000), SampleCount=% (220500)", duration, sampleSize, sampleCount);
		return false;
	}

	return true;
}

bool TMP3FrameParser::isValid() {
	// At last 5 seconds of data...
	return 	duration > 5000 &&
			sampleSize > 882000 &&
			sampleCount > 220500;
}




TMP3Stream::TMP3Stream() {
	prime();
}

TMP3Stream::~TMP3Stream() {
	close();
}

void TMP3Stream::prime() {
	hnd = nil;
	debug = false;
}

int TMP3Stream::open(const TDecoderParams& params) {
	int r = MPG123_OK;
	const char* name = (params.decoder.empty()) ? nil : params.decoder.c_str();
	hnd = mpg123_new(name, &r);
	if (r != MPG123_OK)
		hnd = nil;
	this->params = params;
	settings();
	return r;
}

void TMP3Stream::close() {
	if (util::assigned(hnd))
		mpg123_delete(hnd);
	hnd = nil;
}

void TMP3Stream::settings() {
	if (!util::assigned(hnd))
		return;

	/* Basic settings.
	 * Don't spill messages, enable better resync with non-seekable streams */
	if (!(debug || params.debug)) {
		mpg123_param(hnd, MPG123_ADD_FLAGS, MPG123_QUIET, 0.0);
	}

	/* Bail out on malformed streams  */
	if (params.paranoid) {
		mpg123_param(hnd, MPG123_ADD_FLAGS, MPG123_NO_RESYNC, 0.0);
	}

	if (params.metaint > 0) {
		int r = mpg123_feature(MPG123_FEATURE_PARSE_ICY);
		if (r < 1) {
			throw util::app_error("TMP3Stream::settings() MPG123 has no ICY support.");
		} else {
			if (debug || params.debug)
				std::cout << "TMP3Stream::settings() Enable ICY support for " << params.metaint << " bytes." << std::endl;
		}
		mpg123_param(hnd, MPG123_ICY_INTERVAL, params.metaint, 0.0);
	}

	/* Prevent funky automatic resampling.
	 * This way, we can be sure that one frame will never produce
	 * more than 1152 stereo samples. */
	mpg123_param(hnd, MPG123_REMOVE_FLAGS, MPG123_AUTO_RESAMPLE, 0.0);

	/* Ignore any stream length information contained in the stream,
	 * which can be contained in a 'TLEN' frame of an ID3v2 tag or a Xing tag */
	mpg123_param(hnd, MPG123_ADD_FLAGS, MPG123_IGNORE_STREAMLENGTH, 0.0);

	/* Do not parse the LAME/Xing info frame, treat it as normal MPEG data */
	mpg123_param(hnd, MPG123_ADD_FLAGS, MPG123_IGNORE_INFOFRAME, 0.0);

	/* Older mpg123 is vulnerable to concatenated streams when gapless cutting
	 * is enabled (will only play the jingle of a badly constructed radio
	 * stream). The versions using framewise decoding are fine with that. */
	if (params.gapless) {
		mpg123_param(hnd, MPG123_ADD_FLAGS, MPG123_GAPLESS, 0.0);
	}	

	/* Force fixed sample rate */
	if (params.sampleRate > 0) {
		mpg123_param(hnd, MPG123_FORCE_RATE, params.sampleRate, 0.0);
		if (debug || params.debug)
			std::cout << "TMP3Stream::settings() Force fixed sample rate " << params.sampleRate << " Samples/sec" << std::endl;
	}
}

bool TMP3Stream::getConfiguredValues(TDecoderParams& params) {
	params = this->params;
	return true;
}

bool TMP3Stream::getRunningValues(TDecoderParams& params) {
	params = this->params;
	params.valid = false;
	if (isOpen()) {
		long rate;
		int channels;
		int encoding;

		// Get format information from running stream
		int result = mpg123_getformat(hnd, &rate, &channels, &encoding);
		if (MPG123_OK == result) {
			params.channels = channels;
			params.sampleRate = rate;
			params.bytesPerSample = MPG123_SAMPLESIZE(encoding); // @suppress("Suggested parenthesis around expression")
			params.bitsPerSample = params.bytesPerSample * 8;
		} else {
			return false;
		}

		// Get bitrate from stream
		struct mpg123_frameinfo frame;
		result = mpg123_info(hnd, &frame);
		if (MPG123_OK == result) {
			if (MPG123_CBR == frame.vbr) {
				params.bitRate = frame.bitrate;
			} else {
				// Variable bitrate...
				params.bitRate = std::string::npos;
			}
		} else {
			return false;
		}

		params.valid = true;
	}
	return params.valid;
}


TMP3Decoder::TMP3Decoder() {
	sampleSize = 0;
	debug = false;
}

TMP3Decoder::~TMP3Decoder() {
	if (opened)
		close();
}

void TMP3Decoder::setDebug(const bool value) {
	debug = value;
	decoder.setDebug(debug);
}

bool TMP3Decoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TMP3Decoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TMP3Decoder::open(const std::string& fileName, const CStreamData& properties) {
	sampleSize = 0;
	if (!opened) {
		setRead(0);
		if (properties.isValid()) {
			TDecoderParams params;
			params.paranoid = true;
			//params.rate = properties.sampleRate;
			//params.bits = properties.bitsPerSample;
			decoder.open(params);
			if (decoder.isOpen()) {
				// Open MPEG3 decoder for feeding
				int r = mpg123_open_feed(decoder());
				if (r == MPG123_OK) {
					file.assign(fileName);
					file.open(O_RDONLY);
					opened = file.isOpen();
					if (opened)
						setStream(properties);
					if (debug) {
						std::cout << "TMP3Decoder::open() File \"" << fileName << "\", Opened = " << opened << std::endl;
						stream.debugOutput();
					}
				} else {
					if (debug) {
						std::string errmsg = mpg123_plain_strerror(r);
						std::cout << "TMP3Decoder::open(\"" << fileName << "\") Open MP3 stream failed: \"" << errmsg << "\"" << std::endl;
					}
				}
			}
		}
	}
	return opened;
}

bool TMP3Decoder::open(const TDecoderParams& params) {
	sampleSize = 0;
	if (!opened) {
		setRead(0);
		decoder.open(params);
		if (decoder.isOpen()) {
			// Open MPEG3 decoder for feeding
			int r = mpg123_open_feed(decoder());
			if (r == MPG123_OK) {
				// OK...
				opened = true;
				// setDebug(true);
			} else {
				if (debug) {
					std::string errmsg = mpg123_plain_strerror(r);
					std::cout << "TMP3Decoder::open() Open MP3 stream failed: \"" << errmsg << "\"" << std::endl;
				}
			}
		}
	}
	return opened;
}


bool TMP3Decoder::getConfiguredValues(TDecoderParams& params) {
	return decoder.getConfiguredValues(params);
}

bool TMP3Decoder::getRunningValues(TDecoderParams& params) {
	return decoder.getRunningValues(params);
}

bool TMP3Decoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;
	if (debug)
		 std::cout << "TMP3Decoder::update(0) Open = " << opened << " EOF = " << eof << " Error = " << error << " Buffer = " << util::assigned(buffer) << std::endl;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		PSong song = buffer->getSong();
		if (util::assigned(song)) {
			PSample samples = buffer->writer();
			size_t decoded = 0;
			ssize_t r = 0;
			int errval;

			// Resize simple buffer if needed
			if (chunk.size() < MP3_INPUT_CHUNK_SIZE) {
				chunk.resize(MP3_INPUT_CHUNK_SIZE, false);
			}

			// File reader may throw exception...
			try {

				r = file.read(chunk.data(), chunk.size());
				if (r > 0) {

					// Check for last data chunk of file
					if (r < (ssize_t)chunk.size()) {
						eof = true;
					}
					if (debug) {
						std::cout << "TMP3File::update(1) Bytes read from file = " << r << " EOF = " << eof << std::endl;
						std::cout << "TMP3File::update(1) Total bytes read from file = " << sampleSize << std::endl;
					}

					// Feed input chunk and get first chunk of decoded audio
					errval = mpg123_decode(decoder(), (unsigned char*)chunk.data(), r, samples, song->getChunkSize(), &decoded);
					if (decoded > 0) {
						// Set write buffer pointer to current position after successful decode
						read += decoded;
						sampleSize += decoded;
						buffer->write(decoded);
						samples = buffer->writer();
					}
					if (debug) {
						std::string errmsg = mpg123_plain_strerror(errval);
						std::cout << "TMP3File::update(2) Bytes decoded = " << decoded << std::endl;
						std::cout << "TMP3File::update(2) Decoder state: \"" << errmsg << "\" (" << errval << ")" << std::endl;
					}

					// Decode the rest of the data read from file
					while (errval == MPG123_OK) { // && decoded > 0) {
						// Get all decoded audio that is available now before feeding more input
						errval = mpg123_decode(decoder(), NULL, 0, samples, song->getChunkSize(), &decoded);
						if (decoded > 0) {
							// Set write buffer pointer to current position after successful decode
							read += decoded;
							sampleSize += decoded;
							buffer->write(decoded);
							samples = buffer->writer();
						} else {
							// Nothing to do...
							if (debug) std::cout << "TMP3File::update(*) No more bytes to decode." << std::endl;
							break;
						}
						if (debug) {
							std::string errmsg = mpg123_plain_strerror(errval);
							std::cout << "TMP3File::update(3) Bytes decoded = " << decoded << std::endl;
							std::cout << "TMP3File::update(3) Decoder state: \"" << errmsg << "\" (" << errval << ")" << std::endl;
						}
					}

					// Detect End Of File
					if (isEOF(buffer, read)) {
						eof = true;
					}
					if (debug) {
						std::string errmsg = mpg123_plain_strerror(errval);
						std::cout << "TMP3File::update(4) Decoder progress: " << buffer->getWritten() << " of " << song->getSampleSize() << std::endl;
						std::cout << "TMP3File::update(4) Decoder state: \"" << errmsg << "\" (" << errval << ") Bytes written = " << read << std::endl;
					}

				} else {
					eof = true;
				}

			} catch (...) {
				error = true;
			}

			// Close open file as early as possible...
			if (eof && file.isOpen())
				file.close();
			if (eof && decoder.isOpen())
				decoder.close();
			

			if (debug && eof) {
				 std::cout << "TMP3Decoder::update(5) EOF detected: Total " << buffer->getWritten() << " bytes written." << std::endl;
			}

			// Check for error...
			if (!error)
				return true;
		}
	}

	if (debug)
		 std::cout << "TMP3Decoder::update(6) Open = " << opened << " EOF = " << eof << " Error = " << error << " Buffer = " << util::assigned(buffer) << std::endl;

	// Create internal error
	error = true;
	setBuffer(nil);
	file.close();
	decoder.close();
	return false;
}

bool TMP3Decoder::update(const TSample *const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed) {
	written = 0;
	consumed = 0;
//	if (debug) {
//		if (util::assigned(buffer))
//			std::cout << "TMP3Decoder::update(0) Open = " << opened << " Error = " << error << " Buffersize = " << buffer->size() << std::endl;
//		else
//			std::cout << "TMP3Decoder::update(0) Open = " << opened << " Error = " << error << " Buffer = " << util::assigned(buffer) << std::endl;
//	}

	// Decode single chunk of data from file
	if (opened && !error && size > 0 && util::assigned(data) && util::assigned(buffer)) {
		PSample samples = buffer->writer();
		size_t chunk = (buffer->size() < MP3_OUTPUT_CHUNK_SIZE) ? buffer->size() : MP3_OUTPUT_CHUNK_SIZE;
		size_t decoded = 0;
		int errval;

		// Feed input chunk and get first chunk of decoded audio
		errval = mpg123_decode(decoder(), data, size, samples, chunk, &decoded);
		if (decoded > 0) {
			// Set write buffer pointer to current position after successful decode
			written += decoded;
			sampleSize += decoded;
			buffer->write(decoded);
			samples = buffer->writer();
		}
		if (debug && errval != MPG123_OK) {
			std::string errmsg = mpg123_plain_strerror(errval);
			std::cout << "TMP3File::update(2) Bytes decoded = " << decoded << std::endl;
			std::cout << "TMP3File::update(2) Decoder state: \"" << errmsg << "\" (" << errval << ")" << std::endl;
		}

		// Decode the rest of the data read from file
		while (errval == MPG123_OK) { // && decoded > 0) {
			// Get all decoded audio that is available now before feeding more input
			errval = mpg123_decode(decoder(), NULL, 0, samples, chunk, &decoded);
			if (decoded > 0) {
				// Set write buffer pointer to current position after successful decode
				written += decoded;
				sampleSize += decoded;
				buffer->write(decoded);
				samples = buffer->writer();
			} else {
				// Nothing to do...
				if (debug) std::cout << "TMP3File::update(*) No more bytes to decode." << std::endl;
				break;
			}
			if (debug) {
				std::string errmsg = mpg123_plain_strerror(errval);
				std::cout << "TMP3File::update(3) Bytes decoded = " << decoded << std::endl;
				std::cout << "TMP3File::update(3) Decoder state: \"" << errmsg << "\" (" << errval << ")" << std::endl;
			}
		}

		// Check for error...
		if (!error) {
			// All bytes consumed!
			consumed = size;
			return true;
		}
	}

	if (debug)
		 std::cout << "TMP3Decoder::update(6) Open = " << opened << " Error = " << error << " Buffer = " << util::assigned(buffer) << std::endl;

	// Create internal error
	error = true;
	setBuffer(nil);
	file.close();
	decoder.close();
	return false;
}

bool TMP3Decoder::isEOF(PAudioBuffer buffer, size_t& read) {
	// Detect End Of File
	PSong song = buffer->getSong();
	if (util::assigned(song)) {
		if (sampleSize >= song->getSampleSize()) {
			size_t written = buffer->getWritten();
			size_t diff = sampleSize - song->getSampleSize();
			if (diff > 0) {
				written -= diff;
				buffer->setWritten(written);
				if (read > diff)
					read -= diff;
			}
			return true;
		}
	}
	return false;
}


void TMP3Decoder::close() {
	clear();
	file.close();
	decoder.close();
	if (debug)
		 std::cout << "TMP3Decoder::close()" << std::endl;
}


TMP3File::TMP3File() : TSong(EFT_MP3) {
	prime();
}

TMP3File::TMP3File(const std::string& fileName) : TSong(fileName, EFT_MP3) {
	prime();
}

TMP3File::~TMP3File() {
}

void TMP3File::prime() {
	debug = false;
}

void TMP3File::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TMP3Decoder;
}


bool TMP3File::readMetaData123(const std::string& fileName, TFileTag& tag) {
	TMP3Stream stream;
	TDecoderParams params;
	// params.rate = tag.stream.sampleRate;
	// params.bits = tag.stream.bitsPerSample;
	stream.open(params);
	if (stream.isOpen()) {
		long rate = 0;
		int r, channels = 0, encoding = 0;
		off_t samples = (off_t)MPG123_ERR;
		util::TDataBuffer<BYTE> in(AUDIO_CHUNK_SIZE);
		util::TDataBuffer<BYTE> out(2 * AUDIO_CHUNK_SIZE);
		util::TFile file(fileName);
		size_t decoded;
		ssize_t read;

		// File reader or MP3 decoder may throw exception...
		try {

			// Open MPEG3 feed
			r = mpg123_open_feed(stream());
			if (r != MPG123_OK) {
				std::string errmsg = mpg123_plain_strerror(r);
				std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Open feed failed: \"" << errmsg << "\"" << std::endl;
				return false;
			}

			// Read first data chunk from file
			file.open(O_RDONLY);
			read = file.read(in.data(), in.size());
			if (read != (ssize_t)in.size()) {
				std::string errmsg = sysutil::getSysErrorMessage(file.error());
				std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Reading data chunk failed: \"" << errmsg << "\"" << std::endl;
				return false;
			}

			// Decode first data chunk to read MP3 header data
			r = mpg123_decode(stream(), in.data(), in.size(), out.data(), out.size(), &decoded);
			if (r != MPG123_NEW_FORMAT) {
				std::string errmsg = mpg123_plain_strerror(r);
				std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Decode MP3 chunk failed: \"" << errmsg << "\"" << std::endl;
				return false;
			}

			// Parse audio stream
			std::string hint;
			TMP3FrameParser parser;
			parser.parse(file, 16, false, hint);

			// Read audio format from MP3 stream
			r = mpg123_getformat(stream(), &rate, &channels, &encoding);
			if (r != MPG123_OK || rate <= 0 || channels <= 0 || encoding <= 0) {
				std::string errmsg = mpg123_plain_strerror(r);
				std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Invalid format data: \"" << errmsg << "\" (" << rate << "/" << channels << "/" << encoding << ")" << std::endl;
				return false;
			}

			// Get estimated sample size
			samples = mpg123_length(stream());
			if (samples <= 0 || samples == (off_t)MPG123_ERR) {
				if (r != MPG123_OK) {
					std::string errmsg = mpg123_plain_strerror(r);
					std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Invalid sample count: \"" << errmsg << "\" (" << samples << ")" << std::endl;
				} else {
					std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Invalid sample count (" << samples << ")" << std::endl;
				}
				return false;
			}

			// Calculate frame size, sample count and duration
			util::TTimePart sec, ms;
			ms = (util::TTimePart)(samples * 1000 / rate);
			sec = ms / 1000;

			// Time is crap, escape divide-by-zero
			if (ms <= 0 || sec <= 0) {
				std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Invalid duration: " << ms << "/" << sec << std::endl;
				return false;
			}

			// Get stream information
			tag.stream.duration = ms;
			tag.stream.seconds = sec;
			tag.stream.channels = channels;
			tag.stream.sampleRate = rate;
			tag.stream.bytesPerSample = MPG123_SAMPLESIZE(encoding); // @suppress("Suggested parenthesis around expression")
			tag.stream.bitsPerSample = tag.stream.bytesPerSample * 8;
			tag.stream.sampleCount = samples; // Simple sample count for all channels
			tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.bytesPerSample * tag.stream.channels; // Storage size for all samples in bytes
			tag.stream.bitRate = tag.stream.sampleSize / tag.stream.seconds / 128; // in kBit/s (1024 for kBit / 8 Bit in sampleSize --> divide by 128)

			// Estimated buffer size for frame decoder
			tag.stream.chunkSize = MP3_OUTPUT_CHUNK_SIZE;

			if (tag.stream.isValid()) {
				if (file.isOpen()) {
					readTags(file, 0, tag.meta);
					tag.stream.debugOutput("MP3: " + fileName);
					tag.meta.debugOutput("  --> ");
					return tag.stream.isValid();
				}
			}

		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::cout << std::endl;
			std::cout << "TMP3File::readMetaData(\"" << fileName << "\") Exception: \"" << sExcept << "\"" << std::endl;
			std::cout << std::endl;
		}
	}
	return false;
}


bool TMP3File::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {
		TMP3FrameParser parser;
		int bitsPerSample = 16;

		// File reader or MP3 decoder may throw exception...
		try {
			std::string hint;
			if (!parser.parse(file, bitsPerSample, false, hint)) {
				tag.error = hint;
				return false;
			}

			// Get duration from parser
			util::TTimePart ms = parser.getDuration();
			util::TTimePart sec = ms / 1000;

			// Time is crap, escape divide-by-zero
			if (ms <= 0 || sec <= 0) {
				tag.error = "Invalid song duration in metadata: Millisecond";
				return false;
			}

			// Get stream information
			tag.stream.duration = ms;
			tag.stream.seconds = sec;
			tag.stream.channels = parser.getChannels();
			tag.stream.sampleRate = parser.getSampleRate();
			tag.stream.bitsPerSample = bitsPerSample;
			tag.stream.bytesPerSample = tag.stream.bitsPerSample / 8;
			tag.stream.sampleCount = parser.getSampleCount();
			tag.stream.sampleSize = parser.getSampleSize();
			tag.stream.bitRate = tag.stream.sampleSize / tag.stream.seconds / 128; // in kBit/s (1024 for kBit / 8 Bit in sampleSize --> divide by 128)

			// Estimated buffer size for frame decoder
			tag.stream.chunkSize = MP3_OUTPUT_CHUNK_SIZE;

			if (tag.stream.isValid()) {
				readTags(file, 0, tag.meta);
				return true;
			}

			tag.stream.addErrorHint(tag.error);
			tag.stream.debugOutput(" MP3: ");
			tag.meta.debugOutput(" META: ");
			return false;

		} catch (const std::exception& e)	{
			tag.error = e.what();
		} catch (...) {
			tag.error = "Unknown exception";
		}
		return false;
	}
	tag.error = "MP3 file not readable";
	return false;
}

bool TMP3File::readPictureData(const std::string& fileName, TCoverData& cover) {
	util::TFile file(fileName);
	file.open(O_RDONLY);
	if (file.isOpen()) {
		// File reader or MP3 decoder may throw exception...
		try {
			CMetaData meta;
			readTags(file, 0, meta, cover, ETL_PICTURE);
			return cover.isValid();

		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::cout << std::endl;
			std::cout << "TMP3File::readPictureData(\"" << fileName << "\") Exception: \"" << sExcept << "\"" << std::endl;
			std::cout << std::endl;
		}
	}
	return false;
}


TMP3Song::TMP3Song() : TSong(EFT_MP3) {
}

TMP3Song::~TMP3Song() {
}

void TMP3Song::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TMP3Decoder;
}

bool TMP3Song::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	tag.error = "No ID3 data for plain stream.";
	return false;
}

bool TMP3Song::readPictureData(const std::string& fileName, TCoverData& cover) {
	return false;
}

} /* namespace music */
