/*
 * audiostream.h
 *
 *  Created on: 16.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef AUDIOSTREAM_H_
#define AUDIOSTREAM_H_

#include "audiotypes.h"
#include "audiobuffer.h"
#include "tagtypes.h"
#include "gcc.h"

namespace music {

class TAudioStream;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PAudioStream = TAudioStream*;

#else

typedef TAudioStream* PAudioStream;

#endif


template<typename T>
class TAudioStreamGuard
{
private:
	typedef T stream_t;
	stream_t&  instance;

public:
	TAudioStreamGuard& operator=(const TAudioStreamGuard&) = delete;

	bool isOpen() const {
		return instance.isOpen();
	}

	void close() {
		if (isOpen())
			instance.close();
	}

	explicit TAudioStreamGuard(stream_t& F) : instance(F) {}
	TAudioStreamGuard(const TAudioStreamGuard&) = delete;
	~TAudioStreamGuard() {
		close();
	}
};


class TAudioStream {
protected:
	PAudioBuffer buffer;
	CStreamData stream;
	size_t consumed;
	size_t read;
	bool error;
	bool opened;
	bool eof;

	void setStream(const CStreamData& properties) { stream = properties; };

	void setBuffer(PAudioBuffer buffer) { this->buffer = buffer; };
	PAudioBuffer getBuffer() const { return buffer; };
	bool good() const { return util::assigned(buffer); };

	void setRead(size_t value) { read = value; };
	void addRead(size_t value) { read += value; };
	size_t getRead() const { return read; };

	void setConsumed(size_t value) { consumed = value; };
	void addConsumed(size_t value) { consumed += value; };
	size_t getConsumed() const { return consumed; };

	void prime();
	void clear();

public:
	bool isEOF() const { return eof; };
	bool isOpen() const { return opened; };
	bool hasError() const { return error; };

	virtual size_t getBlockSize() const { return (size_t)0; };

	virtual bool getConfiguredValues(TDecoderParams& params);
	virtual bool getRunningValues(TDecoderParams& params);

	virtual bool update(PAudioBuffer buffer, size_t& read) = 0;
	virtual bool update(const TSample *const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed);

	virtual bool open(const std::string& fileName, const CStreamData& properties) = 0;
	virtual bool open(const TSong& song) = 0;
	virtual bool open(const PSong song) = 0;
	virtual bool open(const TDecoderParams& params) = 0;
	virtual void close() = 0;

	TAudioStream();
	virtual ~TAudioStream();
};


class TAudioStreamAdapter {
protected:
	PAudioStream stream;

	virtual void decoderNeeded() = 0;
	void clear();

public:
	PAudioStream getStream();

	TAudioStreamAdapter();
	virtual ~TAudioStreamAdapter();
};


} /* namespace music */

#endif /* AUDIOSTREAM_H_ */
