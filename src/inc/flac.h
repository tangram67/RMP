/*
 * flac.h
 *
 *  Created on: 30.09.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef FLAC_H_
#define FLAC_H_

// Include tagtypes.h first to include stdint.h with __STDC_LIMIT_MACROS defined!
#include "audiostream.h"
#include "audiofile.h"
#include "tagtypes.h"
#include "tags.h"
#include "gcc.h"

#include <FLAC/metadata.h>
#include "FLAC/stream_decoder.h"

namespace music {


STATIC_CONST size_t FLAC_DECODER_BLOCKSIZE = 2 * 65536 + 8192; // 2 * Max. FLAC blocksize + 1 callback buffer size

class TFLACStream;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PFLACStream = TFLACStream*;

#else

typedef TFLACStream* PFLACStream;

#endif



class TFLACObject {
protected:
	bool valid;

public:
	bool isValid() const { return valid; };
	virtual void clear() = 0;

	TFLACObject();
	virtual ~TFLACObject() = default;
};



class TFLACMetaDataChain : public TFLACObject {
private:
	FLAC__Metadata_Chain * chain;

public:
	FLAC__Metadata_Chain * operator () () { return chain; };
	void clear();

	TFLACMetaDataChain();
	virtual ~TFLACMetaDataChain();
};


class TFALCMetaDataIterator : public TFLACObject {
private:
	FLAC__Metadata_Iterator * iterator;

public:
	FLAC__Metadata_Iterator * operator () () { return iterator; };
	void clear();

	TFALCMetaDataIterator();
	virtual ~TFALCMetaDataIterator();
};


class TFLACStream {
private:
	bool streaming;
	TDecoderParams configured;
	TDecoderParams running;
	TMetaData metadata;

	void prime();

protected:
	bool streamdataOK;
	bool headerdataOK;
	bool metadataOK;

	void setRunningValues(const TDecoderParams& params);
	void setMetaData(TMetaData& data);

public:
	virtual FLAC__StreamDecoderWriteStatus decoderCallback(const FLAC__Frame * frame, const FLAC__int32 * const buffer[]) = 0;
	virtual FLAC__StreamDecoderWriteStatus writerCallback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]) = 0;
	virtual FLAC__StreamDecoderReadStatus readerCallback(FLAC__byte buffer[], size_t *bytes) = 0;
	virtual void metadataCallback(const FLAC__StreamMetadata *block) = 0;
	virtual void errorCallback(FLAC__StreamDecoderErrorStatus status) = 0;

	bool isStreaming() const { return streaming; };
	bool hasMetadata() const { return metadataOK; };
	bool hasHeaderdata() const { return headerdataOK; };
	bool hasStreamdata() const { return streamdataOK; };

	bool getConfiguredValues(TDecoderParams& params);
	bool getRunningValues(TDecoderParams& params);
	bool getMetaData(TMetaData& data);

	void open(const TDecoderParams& params);
	void close();

	TFLACStream();
	virtual ~TFLACStream();
};


class TFLACDecoder : public TFLACStream, public TAudioStream, protected TAudioConvert {
private:
	FLAC__StreamDecoder * decoder;
	FLAC__StreamDecoderInitStatus initval;
	FLAC__StreamDecoderErrorStatus errval;
	mutable FLAC__StreamDecoderState stateval;
	TSample const * inbuf;
	size_t insize;
	bool updated;
	bool debug;

	void clear();
	void prime();
	FLAC__StreamDecoderWriteStatus decoderCallback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	FLAC__StreamDecoderWriteStatus writerCallback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	FLAC__StreamDecoderReadStatus readerCallback(FLAC__byte buffer[], size_t *bytes);
	void metadataCallback(const FLAC__StreamMetadata *metadata);
	void errorCallback(FLAC__StreamDecoderErrorStatus status);
	void decoderNeeded();

public:
	bool valid() const { return util::assigned(decoder); };
	size_t getBlockSize() const { return FLAC_DECODER_BLOCKSIZE; };

	std::string errmsg() const;
	std::string initmsg() const;
	std::string statusmsg() const;
	std::string statusmsg(const FLAC__StreamDecoderState state) const;
	FLAC__StreamDecoderState statuscode() const;
	FLAC__StreamDecoderInitStatus initcode() const { return initval; };
	FLAC__StreamDecoderErrorStatus errorcode() const { return errval; };

	FLAC__StreamDecoder * operator () () { return decoder; };

	bool open(const std::string& fileName, const CStreamData& properties);
	bool open(const TSong& song);
	bool open(const PSong song);
	bool open(const TDecoderParams& params);
	void close();

	bool update(PAudioBuffer buffer, size_t& read);
	bool update(const TSample * const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed);

	bool getConfiguredValues(TDecoderParams& params);
	bool getRunningValues(TDecoderParams& params);

	TFLACDecoder();
	virtual ~TFLACDecoder();
};


class TFLACFile : public TSong {
private:
	void prime();
	void decoderNeeded();

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TFLACFile();
	TFLACFile(const std::string& fileName);
	virtual ~TFLACFile();
};


class TFLACSong : public TSong {
private:
	void decoderNeeded();

public:
	bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type);
	bool readPictureData(const std::string& fileName, TCoverData& cover);

	TFLACSong();
	virtual ~TFLACSong();
};


} /* namespace music */

#endif /* FLAC_H_ */
