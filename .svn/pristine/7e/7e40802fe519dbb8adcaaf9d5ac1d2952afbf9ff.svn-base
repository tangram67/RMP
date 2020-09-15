/*
 * aiff.h
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef AIFF_H_
#define AIFF_H_

#include "id3.h"
#include "audiotypes.h"
#include "audiobuffer.h"
#include "aifftypes.h"

namespace music {

class TAIFFStream : public util::TEndian {
public:
	void setFileHeaderEndian(TAIFFFileHeader& header) const;
	void setCommonChunkEndian(TAIFFCommonChunk& common) const;
	void setSoundChunkEndian(CAIFFSoundChunk& sound) const;

	TAIFFStream();
	virtual ~TAIFFStream();
};


class TAIFFDecoder : private TAIFFStream, public TAudioStream {
private:
	TSampleBuffer chunk;
	util::TFile file;
	bool debug;

	size_t convertToLittleEndian(const TSample* aiff, TSample* pcm, size_t size, int bitsPerSample);

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TAIFFDecoder();
	virtual ~TAIFFDecoder();
};


class TAIFFFile : private TAIFFStream, private TID3TagReader, public TSong {
private:
	size_t offsetToID3Tag;
	size_t sizeOfID3Tag;
	void prime();

	void decoderNeeded();
	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool isAIFFFile(const TAIFFFileHeader& header) const;
	bool readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size);

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);
	size_t getOffsetToID3Tag() const { return offsetToID3Tag; };

	TAIFFFile();
	TAIFFFile(const std::string& fileName);
	virtual ~TAIFFFile();
};





} /* namespace music */

#endif /* AIFF_H_ */
