/*
 * parser.h
 *
 *  Created on: 07.03.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "classes.h"
#include "templates.h"
#include "datetime.h"
#include "nullptr.h"


namespace util {

struct CToken;
struct CParserBuffer;
class TTokenParser;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TToken = CToken;
using PToken = TToken*;
using PTokenParser = TTokenParser*;
using TParserBuffer = CParserBuffer;
using PParserBuffer = TParserBuffer*;
using TTokenList = std::vector<util::PToken>;
using TBufferList = std::vector<util::PParserBuffer>;
using TTokenMap = std::map<std::string, util::PToken>;
using TTokenMapItem = std::pair<std::string, util::PToken>;

#else

typedef CToken TToken;
typedef TToken* PToken;
typedef TTokenParser* PTokenParser;
typedef CParserBuffer TParserBuffer;
typedef TParserBuffer* PParserBuffer;
typedef std::vector<util::PToken> TTokenList;
typedef std::vector<util::PParserBuffer> TBufferList;
typedef std::map<std::string, util::PToken> TTokenMap;
typedef std::pair<std::string, util::PToken> TTokenMapItem;

#endif



struct CParserBuffer {

	// Buffer
	char* buffer;
	size_t size;

	// Reference counter giving the number
	// of references to this buffer
	int refC;

	// Time when this buffer was last used.
	time_t timestamp;

	// ID
	unsigned long int id;

	// Flags
	bool valid;

	void setTimeStamp() {
		timestamp = util::now();
	}

	const bool isValid() const {
		return (util::assigned(buffer) && (size > 0));
	}

	void prime() {
		buffer = nil;
		valid = false;
		size = 0;
		refC = 0;
		id = 0;
		setTimeStamp();
	}

	void clear() {
		if (util::assigned(buffer)) {
			delete[] buffer;
		}
		prime();
	}

	CParserBuffer() {
		prime();
	}

	~CParserBuffer() {
		clear();
	}
};


struct CToken {
	// Token item
	std::string token;
	std::string value;

	// Pointer to one char before token + mask
	// Content change not allowed by const modifier!
	const char* begin;

	// Pointer to char after token + mask
	const char* end;

	// Size of complete placeholder
	//    size = sizeof(startMask) + sizeof(token) + sizeof(endMask)
	// or
	//    size = end - 1 - begin + 1 + 1 = end - begin + 1;
	size_t size;

	// Index in buffer
	size_t index;

	CToken() {
		token.clear();
		value.clear();
		begin = nil;
		end = nil;
		size = 0;
		index = 0;
	}
};


class TTokenParser : public app::TObject {
private:
	enum EParseState { ST_FIND_START_MASK, ST_FIND_END_MASK };

	const char* rdBuffer;
	size_t rdBufferSize;
	PParserBuffer currentBuffer;
	size_t tokenSize;
	size_t valueSize;
	std::string startMask;
	std::string endMask;
	TTokenMap tokenMap;
	TTokenList tokenList;
	TBufferList bufferList;
	mutable std::mutex mtx;

	void init();
	void clearTokenMap();
	bool findMask(const char*& p, const std::string& mask);
	void addToken(const char* maBegin, const char* maEnd, const char* toBegin, const char* toEnd);
	void deleteWriteBuffer();
	PParserBuffer newWriteBuffer();
	PParserBuffer getWriteBuffer();
	void copy(char* dest, const char* src, size_t size);
	int tokenize(char* data, size_t max);
	void writeToFile(const char *const buffer, const size_t size, const std::string& fileName);
	size_t calcValueSize();
	size_t calcTokenSize();

public:
	int parse();
	int replace();
	void invalidate();
	void initialize(const char* buffer, const size_t size, const std::string& startMask, const std::string& endMask);
	void debugOutput(bool verbose = true);

	int decBufferRefCount();
	int getBufferRefCount();

	size_t deleteInvalidatedBuffers();
	bool setTokenValue(const std::string& token, const std::string& value, bool invalidate = false);
	std::string getTokenValue(const std::string& token);
	const PParserBuffer getData();
	bool hasToken() const { return !tokenMap.empty(); }
	PToken getToken(const std::string& key) const;

	TTokenParser();
	explicit TTokenParser(const char* buffer, size_t size, const std::string& startMask, const std::string& endMask);
	virtual ~TTokenParser();
};

} /* namespace util */

#endif /* PARSER_H_ */
