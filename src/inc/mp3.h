/*
 * mp3.h
 *
 *  Created on: 05.11.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef MP3_H_
#define MP3_H_

#include "audiostream.h"
#include "audiofile.h"
#include "tagtypes.h"
#include "windows.h"
#include "tags.h"
#include "id3.h"
#include "gcc.h"
#include "datetime.h"
#include "mp3types.h"

#include "mpg123/mpg123.h"

namespace music {

class TMP3Init {
public:
	TMP3Init();
	~TMP3Init();
};


class TMP3FrameReader {
private:
	bool debug;

	bool isValidFrame(const TMP3FrameValues& frame) const ;
	DWORD convertToMP3Header(const void* buffer) const ;

	size_t getFrameSize(int layerIndex, size_t bitRate, size_t sampleRate, bool padding) const ;
	size_t getBitrate(int layerIndex, int versionIndex, int bitRateIndex) const ;
	size_t getSampleRate(int versionIndex, int sampleRateIndex) const ;
	int getVersionNumber(int versionIndex) const ;
	std::string getModeAsString(int modeIndex) const ;
	int getChannels(int modeIndex) const ;
	MPEGLayer getMPEGLayer(int layerIndex) const ;

	int getFrameSync(DWORD header) const ;
	int getVersionIndex(DWORD header) const ;
	int getLayerIndex(DWORD header) const ;
	bool getProtectionBit(DWORD header) const ;
	int getBitrateIndex(DWORD header) const ;
	int getSampleRateIndex(DWORD header) const ;
	bool getPaddingBit(DWORD header) const ;
	bool getPrivateBit(DWORD header) const ;
	int getModeIndex(DWORD header) const ;
	int getModeExtIndex(DWORD header) const ;
	bool getCoprightBit(DWORD header) const ;
	bool getOrginalBit(DWORD header) const ;
	int getEmphasisIndex(DWORD header) const ;

public:
	bool findMP3Frame(const void* buffer, size_t size, off_t& offset) const;
	bool getMP3Frame(const void* buffer, TMP3FrameHeader& frame) const ;
	bool getMP3Frame(DWORD header, TMP3FrameHeader& frame) const ;

	void debugOutput(const TMP3FrameHeader& frame, const std::string& preamble = "") const;

	TMP3FrameReader();
	~TMP3FrameReader();
};


class TMP3FrameParser : private TID3HeaderReader {
private:
	TMP3FrameHeader stream;
	util::TTimePart duration;
	size_t sampleCount;
	size_t sampleSize;
	size_t frameCount;
	bool debug;

public:
	bool isValid();

	util::TTimePart getDuration() const { return duration; };
	size_t getSampleCount() const { return sampleCount; };
	size_t getSampleSize() const { return sampleSize; };
	size_t getSampleRate() const { return stream.stream.samplerate; };
	size_t getFrameCount() const { return frameCount; };
	size_t getChannels() const { return stream.stream.channels; };

	bool parse(const util::TFile& file, int bitsPerSample /* = 16 */, bool vbr /* = false */, std::string& hint);

	TMP3FrameParser();
	~TMP3FrameParser();
};


class TMP3Stream {
	TDecoderParams params;
	mpg123_handle* hnd;
	bool debug;

	void prime();
	void settings();

public:
	mpg123_handle * handle() { return hnd; };
	mpg123_handle * operator () () { return handle(); };
	bool isOpen() { return util::assigned(hnd); };

	bool getConfiguredValues(TDecoderParams& params);
	bool getRunningValues(TDecoderParams& params);

	int open(const TDecoderParams& params);
	void close();

	void setDebug(const bool value) { debug = value; };

	TMP3Stream();
	virtual ~TMP3Stream();
};


class TMP3Decoder : public TAudioStream {
private:
	util::TFile file;
	TSampleBuffer chunk;
	TMP3Stream decoder;
	size_t sampleSize;
	bool debug;

	bool isEOF(PAudioBuffer buffer, size_t& read);

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);
	void close();

	bool update(PAudioBuffer buffer, size_t& read);
	bool update(const TSample *const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed);

	bool getConfiguredValues(TDecoderParams& params);
	bool getRunningValues(TDecoderParams& params);

	void setDebug(const bool value);

	TMP3Decoder();
	virtual ~TMP3Decoder();
};


class TMP3File : private TID3TagReader, public TSong {
private:
	bool debug;
	void prime();
	void decoderNeeded();
	bool readMetaData123(const std::string& fileName, TFileTag& tag);

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TMP3File();
	TMP3File(const std::string& fileName);
	virtual ~TMP3File();
};


class TMP3Song : public TSong {
private:
	void decoderNeeded();

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TMP3Song();
	virtual ~TMP3Song();
};

static TMP3Init MP3Init;

} /* namespace music */

#endif /* MP3_H_ */
