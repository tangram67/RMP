/*
 * dsd.h
 *
 *  Created on: 15.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef DSD_H_
#define DSD_H_

#include "gcc.h"
#include "id3.h"
#include "endian.h"
#include "windows.h"
#include "audiotypes.h"
#include "dsdtypes.h"

namespace music {


class TDSDStream {
protected:
	TSample reverse(TSample sample, int bitsPerSample);
	size_t convertDSDtoDoP(const TSample* dsd, TSample* dop, size_t size, size_t offset, int bitsPerSample);

public:
	TDSDStream();
	virtual ~TDSDStream();
};


class TDSFStream : public util::TEndian {
protected:
	void setDSFFileHeaderEndian(TDSFFileHeader& header) const;
	void setDSFHeaderEndian(TDSFHeader& header) const;
	void setDSFFormatEndian(TDSFFormat& format) const;
	void setDSFDataEndian(TDSFData& data) const;

public:
	TDSFStream();
	virtual ~TDSFStream();
};


class TDSFDecoder : private TDSFStream, private TDSDStream, public TAudioStream {
private:
	TSampleBuffer chunk;
	util::TFile file;
	bool debug;

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TDSFDecoder();
	virtual ~TDSFDecoder();
};


class TDSFFile : private TDSFStream, private TID3TagReader, public TSong {
private:
	size_t offsetToID3Tag;
	size_t sizeOfID3Tag;
	void prime();

	void decoderNeeded();
	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool isDSFFile(const TDSFFileHeader& header) const;

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);
	size_t getOffsetToID3Tag() const { return offsetToID3Tag; };

	TDSFFile();
	TDSFFile(const std::string& fileName);
	virtual ~TDSFFile();
};


class TDFFStream : public util::TEndian {
protected:
	void setDSFFileHeaderEndian(TDFFFileHeader& header) const;
	void setDFFFormChunkEndian(TDFFFormChunk& header) const;
	void setDFFSampleRateEndian(TDFFSampleRate& rate) const;
	void setDFFChannelsEndian(TDFFChannels& channels) const;
	void setDFFSpeakerEndian(TDFFSpeakerConfig& speakers) const;
	void setDFFSoundDataEndian(TDFFSoundDataChunk& data) const;

public:
	TDFFStream();
	virtual ~TDFFStream();
};


class TDFFDecoder : private TDFFStream, private TDSDStream, public TAudioStream {
private:
	TSampleBuffer chunk;
	util::TFile file;
	bool debug;

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TDFFDecoder();
	virtual ~TDFFDecoder();
};


class TDFFFile : private TDFFStream, private TID3TagReader, public TSong {
private:
	size_t offsetToID3Tag;
	size_t sizeOfID3Tag;
	void prime();

	void decoderNeeded();
	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool isDFFFile(const TDFFFormChunk& header) const;
	bool readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size);

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);
	size_t getOffsetToID3Tag() const { return offsetToID3Tag; };

	TDFFFile();
	TDFFFile(const std::string& fileName);
	virtual ~TDFFFile();
};



} /* namespace music */

#endif /* DSD_H_ */
