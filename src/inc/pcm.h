/*
 * pcm.h
 *
 *  Created on: 21.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef PCM_H_
#define PCM_H_

#include "gcc.h"
#include "endian.h"
#include "windows.h"
#include "audiotypes.h"
#include "audiofile.h"
#include "pcmtypes.h"
#include "ASCII.h"
#include "id3.h"

class TAudioBuffer;

typedef struct CRIFFAtom {
	size_t offset;
	size_t size;
	std::string ID;
	DWORD hash;
} TRIFFAtom;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TAtomMap = std::map<std::string, TRIFFAtom>;
using TAtomMapItem = std::pair<std::string, TRIFFAtom>;
using TTagMap = std::map<DWORD, std::string>;
using TTagMapItem = std::pair<DWORD, std::string>;
using PAudioBuffer = TAudioBuffer*;

#else

typedef std::map<std::string, TRIFFAtom> TAtomMap;
typedef std::pair<std::string, TRIFFAtom> TAtomMapItem;
typedef std::map<DWORD, std::string> TTagMap;
typedef std::pair<DWORD, std::string> TTagMapItem;
typedef TAudioBuffer* PAudioBuffer;

#endif


namespace music {

class TRIFFAtomScanner : private util::TEndian, private util::TASCII, private TAudioConvert {
private:
	typedef TAtomMap::iterator iterator;

	TAtomMap atoms;
	TTagMap tags;
	TRIFFAtom defItem;
	size_t seekPos;
	bool debug;

	void prime();
	void clear();

	bool isChunkID(const TChunkID& value, const char* ID) const;

	char* findAtom(const std::string& ID, char* buffer, size_t size) const;
	ssize_t scanAtoms(const util::TFile& file, const size_t offset);
	ssize_t scanTags(const char *const buffer, const size_t size);

	bool seek(const util::TFile& file, const size_t offset);
	bool read(const util::TFile& file, void *const data, const size_t size);
	bool readHeader(const util::TFile& file, const size_t offset, void *const buffer, const size_t size);
	const char *const copyHeader(const char *const buffer, const size_t offset, void *const header, const size_t size) const;

	bool addTag(const DWORD hash, const std::string& value);
	bool validTagID(const DWORD hash) const;
	std::string hashTagToStr(const DWORD hash) const;

public:
	typedef TAtomMap::const_iterator const_iterator;

	bool hasTags() const { return !tags.empty(); };
	ssize_t getTags(CMetaData& tag);

	size_t size() const { return atoms.size(); };
	bool empty() const { return atoms.empty(); };
	const_iterator begin() const { return atoms.begin(); };
	const_iterator end() const { return atoms.end(); };

	const TRIFFAtom& addAtom(const std::string& ID, size_t offset, size_t size);
	const TRIFFAtom& find(const std::string& ID) const;
	const TRIFFAtom& operator[] (const std::string& ID) const;

	ssize_t scan(util::TFile& file, size_t offset);

	void debugOutputChunks(const std::string& preamble) const;
	void debugOutputTags(const std::string& preamble) const;

	TRIFFAtomScanner();
	virtual ~TRIFFAtomScanner();
};


class TPCMStream : public util::TEndian {
protected:
	void setHeaderEndian(TPCMFileHeader& header) const;
	void setHeaderEndian(TPCMBaseHeader& header) const;
	void setChunkEndian(TPCMChunkHeader& header) const;

	size_t parseWaveHeader(const util::TFile& file, TPCMBaseHeader& header, TPCMChunkHeader& data) const;
	size_t readChunkHeader(const util::TBuffer& buffer, const size_t offset, TPCMChunkHeader& header) const;
	bool isChunkID(const TChunkID& value, const char* ID) const;
	bool isWave(const TPCMFileHeader& header) const;
	bool isWave(const TPCMBaseHeader& header) const;

public:
	TPCMStream();
	virtual ~TPCMStream();
};


class TPCMWriter {
protected:
	size_t writeRawData(util::TFile& file, const TAudioBuffer* buffer, const TAudioBufferList* buffers) const;

public:
	bool saveRawFile(const std::string& fileName, const TAudioBuffer* buffer, const TAudioBufferList* buffers = nil) const;

	TPCMWriter();
	virtual ~TPCMWriter();
};


class TPCMEncoder : private TPCMStream, public TPCMWriter {
public:
	bool saveToFile(const std::string& fileName, const TAudioBuffer* buffer, const TAudioBufferList* buffers = nil) const;

	TPCMEncoder();
	virtual ~TPCMEncoder();
};


class TPCMDecoder : private TPCMStream, public TAudioStream {
private:
	bool debug;
	util::TFile file;

public:
	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);

	bool update(PAudioBuffer buffer, size_t& read);

	void close();

	TPCMDecoder();
	virtual ~TPCMDecoder();
};


class TPCMFile : private TPCMStream, private TID3TagReader, public TSong {
private:
	void prime();
	void decoderNeeded();

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TPCMFile();
	TPCMFile(const std::string& fileName);
	virtual ~TPCMFile();
};

} /* namespace music */

#endif /* PCM_H_ */
