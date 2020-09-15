/*
 * alac.h
 *
 *  Created on: 01.10.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ALAC_H_
#define ALAC_H_

#include <map>
#include "gcc.h"

#include "id3.h"
#include "endian.h"
#include "audiofile.h"
#include "audiotypes.h"
#include "audiobuffer.h"
#include "alactypes.h"

#include "alac/ALACBitUtilities.h"
#include "libfaad/neaacdec.h"

class ALACDecoder;

namespace music {

typedef struct CAlacAtom {
	size_t offset;
	size_t data;
	size_t size;
	std::string ID;
	DWORD hash;
} TALACAtom;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TAtomMap = std::map<std::string, TALACAtom>;
using TAtomMapItem = std::pair<std::string, TALACAtom>;
using TTagMap = std::map<DWORD, std::string>;
using TTagMapItem = std::pair<DWORD, std::string>;
using TDecoderBuffer = util::TDataBuffer<uint8_t>;

#else

typedef std::map<std::string, TALACAtom> TAtomMap;
typedef std::pair<std::string, TALACAtom> TAtomMapItem;
typedef std::map<DWORD, std::string> TTagMap;
typedef std::pair<DWORD, std::string> TTagMapItem;
typedef util::TDataBuffer<uint8_t> TDecoderBuffer;

#endif


class TALACAtomScanner : private util::TASCII, private util::TEndian, private TAudioConvert {
private:
	typedef TAtomMap::iterator iterator;

	TAtomMap atoms;
	TTagMap tags;
	TArtwork artwork;
	TALACAtom defItem;
	size_t defaultSize;
	size_t bufferSize;
	size_t seekPos;
	ECodecType codec;
	bool debug;

	void prime();
	void clear();

	ssize_t scanner(util::TFile& file, const ETagLoaderType mode);

	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool isAlacFile(const TAlacFileHeader& header) const;

	ssize_t scanChunks(const util::TFile& file, const size_t offset);
	ssize_t scanSubChunks(const util::TFile& file, const std::string& ID);
	ssize_t scanSubChunks(const util::TFile& file, const TALACAtom& atom);
	ssize_t scanMetaData(const util::TFile& file, const ETagLoaderType mode);

	bool addAtoms(const util::TFile& file, const std::string& ID, size_t offset, size_t size);
	char* findAtom(const std::string& ID, char* buffer, size_t size) const;
	bool hasChilds(const TALACAtom& atom) const;
	bool hasChilds(const DWORD hash) const;

	void getFlavour(const util::TFile& file, ECodecType& type);
	size_t getBlockLength(const void *const buffer);

	bool seek(const util::TFile& file, const size_t offset);
	bool read(const util::TFile& file, void *const data, const size_t size);
	bool readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size);

	bool addTag(const DWORD hash, const std::string& value);
	ssize_t scanTags(const char *const buffer, const size_t size, const ETagLoaderType mode);
	const char *const copyHeader(const char *const buffer, const size_t offset, void *const header, const size_t size) const;
	bool validTagID(const DWORD hash) const;
	std::string hashTagToStr(const DWORD hash) const;

public:
	typedef TAtomMap::const_iterator const_iterator;

	bool hasTags() const { return !tags.empty(); };
	ssize_t getTags(CMetaData& tag);
	bool getPicture(CCoverData& cover);

	ECodecType getCodec() const { return codec; };
	void setCodec(const ECodecType value) { codec = value; };

	size_t getBufferSize() const { return defaultSize; };
	void setBufferSize(const size_t value) { defaultSize = value; };
	void setDebug(const bool value) { debug = value; };

	size_t size() const { return atoms.size(); };
	bool empty() const { return atoms.empty(); };
	const_iterator begin() const { return atoms.begin(); };
	const_iterator end() const { return atoms.end(); };

	const TALACAtom& addAtom(const std::string& ID, size_t offset, size_t size);
	const TALACAtom& find(const std::string& ID) const;
	const TALACAtom& operator[] (const std::string& ID) const;

	ssize_t scan(const std::string& fileName, const ETagLoaderType mode);
	ssize_t scan(util::TFile& file, const ETagLoaderType mode);

	void debugOutputChunks(const std::string& preamble) const;
	void debugOutputTags(const std::string& preamble) const;

	TALACAtomScanner();
	virtual ~TALACAtomScanner();
};


class TALACStream : public util::TEndian {
public:
	void setFileHeaderEndian(TAlacFileHeader& header) const;
	void setStreamChunkEndian(TAlacStreamChunk& stream) const;
	void setMediaChunkEndian(TAlacMediaChunk& media) const;

	TALACStream();
	virtual ~TALACStream();
};


class TAACDecoder : private TALACStream, public TAudioStream {
private:
	NeAACDecHandle decoder;
	NeAACDecFrameInfo info;
	size_t readBufferSize;
    size_t inputBufferSize;
    TDecoderBuffer inputBuffer;
    size_t fileOffset;
    size_t lastOffset;
    size_t readOffset;
    size_t bufferSize;
    size_t decodedSize;

    TAlacBlockHeader blockHeader;
	util::TFile file;
	bool debug;

	void prime();
	void clear();

	bool createDecoder();
	void freeDecoder();

	bool seek(const util::TFile& file, const size_t offset);
	ssize_t read(const util::TFile& file, TDecoderBuffer& buffer, const size_t size);

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TAACDecoder();
	virtual ~TAACDecoder();
};


class TALACDecoder : private TALACStream, public TAudioStream {
private:
	ALACDecoder* decoder;
	size_t readBufferSize;
    size_t inputBufferSize;
    int32_t inputPacketSize;
    int32_t outputPacketSize;
    TDecoderBuffer inputBuffer;
    BitBuffer readBuffer;
    size_t fileOffset;
    size_t lastOffset;
    size_t decodedSize;

    TAlacBlockHeader blockHeader;
	util::TFile file;
	bool debug;

	void prime();

	bool createDecoder();
	void freeDecoder();

	size_t getBlockLength(const TDecoderBuffer& buffer, size_t& length);
	bool getBlockHeader(const void *const buffer, const size_t size, TAlacBlockHeader& header) const;
	bool isBlockHeader(const TAlacBlockHeader& header) const;
	bool compareBlockID(const void *const buffer, const TAlacBlockHeader& header) const;
	bool compareBlockHeader(const void *const buffer, const TAlacBlockHeader& header) const;
	size_t findNextBlockHeader(const void *const buffer, const size_t size, const TAlacBlockHeader& header) const;

	bool seek(const util::TFile& file, const size_t offset);
	ssize_t read(const util::TFile& file, TDecoderBuffer& buffer, const size_t size);

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TALACDecoder();
	virtual ~TALACDecoder();
};


class TALACFile : private TALACStream, private TID3TagReader, public TSong {
private:
	void prime();

	void decoderNeeded();
	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size);
	bool seek(const util::TFile& file, const size_t offset);
	ssize_t read(const util::TFile& file, void *const data, const size_t size);

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TALACFile();
	TALACFile(const ECodecType type);
	TALACFile(const std::string& fileName);
	virtual ~TALACFile();
};


} /* namespace music */

#endif /* ALAC_H_ */
