/*
 * parser.cpp
 *
 *  Created on: 07.03.2015
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include "parser.h"
#include "ASCII.h"
#include "ansi.h"
#include "exception.h"

namespace util {


STATIC_CONST int BUFFER_DELETE_AGE = 2000;
static int TokenParserCnt = 0;


TTokenParser::TTokenParser() {
	init();
}


#ifdef STL_HAS_DELEGATING_CTOR
TTokenParser::TTokenParser(const char* buffer, const size_t size, const std::string& startMask, const std::string& endMask) : TTokenParser() {
#else
TTokenParser::TTokenParser(const char* buffer, const size_t size, const std::string& startMask, const std::string& endMask) {
	init();
#endif
	initialize(buffer, size, startMask, endMask);
}


TTokenParser::~TTokenParser() {
	clearTokenMap();
	deleteWriteBuffer();
}


void TTokenParser::init() {
	rdBuffer = nil;
	rdBufferSize = 0;
	currentBuffer = nil;
	tokenSize = 0;
	valueSize = 0;
	startMask.clear();
	endMask.clear();
	deleteWriteBuffer();
	TokenParserCnt++;
}


void TTokenParser::initialize(const char* buffer, const size_t size, const std::string& startMask, const std::string& endMask) {
	rdBuffer = buffer;
	rdBufferSize = size;
	this->startMask = startMask;
	this->endMask = endMask;
	clearTokenMap();
	deleteWriteBuffer();
}


void TTokenParser::invalidate() {
	std::lock_guard<std::mutex> lock(mtx);
	deleteWriteBuffer();
}


void TTokenParser::clearTokenMap() {
	if (!tokenMap.empty()) {
		PToken o;
		TTokenMap::const_iterator it = tokenMap.begin();
		while (it != tokenMap.end()) {
			o = it->second;
			if (assigned(o))
				freeAndNil(o);
			it++;
		}
		tokenMap.clear();
	}
	tokenList.clear();
}


size_t TTokenParser::calcValueSize() {
	size_t retVal = 0;
	if (!tokenMap.empty()) {
		PToken o;
		TTokenMap::const_iterator it = tokenMap.begin();
		while (it != tokenMap.end()) {
			o = it->second;
			retVal += o->value.size();
			it++;
		}
	}
	return retVal;
}


size_t TTokenParser::calcTokenSize() {
	size_t retVal = 0;
	if (!tokenMap.empty()) {
		PToken o;
		TTokenMap::const_iterator it = tokenMap.begin();
		while (it != tokenMap.end()) {
			o = it->second;
			retVal += o->size;
			it++;
		}
	}
	return retVal;
}


void TTokenParser::debugOutput(bool verbose) {
	if (!tokenMap.empty()) {
		std::cout << app::blue << "Token list for " << std::to_string((size_u)tokenMap.size()) << " token:" << app::reset << std::endl << std::endl;
		int count = 1;
		PToken o;
		TTokenList::const_iterator it = tokenList.begin();
		while (it != tokenList.end()) {
			o = *it;

			std::cout << app::white << std::to_string((size_u)count) << ". Token" << app::reset << std::endl;
			std::cout << "  Token\t" << o->token << std::endl;
			std::cout << "  Mask \t" << o->value << std::endl;

			if (verbose) {
				std::cout << "  Size \t" << std::to_string((size_u)o->size) << std::endl;
				std::cout << "  Index\t" << std::to_string((size_u)o->index) << std::endl;

				if (assigned(o->begin)) {
					std::cout << "  Begin\t" << std::to_string((size_s)(o->begin - rdBuffer)) << std::endl;
					int cnt = 3;
					ssize_t idx = o->begin - rdBuffer;
					while (idx >= 0 && cnt > 0) {
						std::cout << "  Leading char[" << std::to_string((size_s)(idx)) << "] = " << util::TBinaryConvert::binToAscii(rdBuffer[idx]) << std::endl;
						--idx;
						--cnt;
					}
				} else
					std::cout << "  Begin\t" << app::blue << "<not assigned>" << app::reset << std::endl;

				if (assigned(o->end)) {
					std::cout << "  End\t" << std::to_string((size_s)(o->end - rdBuffer)) << std::endl;
					int cnt = 3;
					size_t idx = o->end - rdBuffer;
					while (idx < rdBufferSize && cnt > 0) {
						std::cout << "  Trailing char[" << std::to_string((size_s)idx) << "] = " << util::TBinaryConvert::binToAscii(rdBuffer[idx]) << std::endl;
						++idx;
						--cnt;
					}
				} else
					std::cout << "  End\t" << app::blue << "<not assigned>" << app::reset << std::endl;
			}

			std::cout << std::endl;
			it++;
			count++;
		}
	} else
		std::cout << app::red << "Token list is empty." << app::reset << std::endl;
}


bool TTokenParser::setTokenValue(const std::string& token, const std::string& value, bool invalidate) {
	std::lock_guard<std::mutex> lock(mtx);
	bool retVal = false;
	PToken o;
	TTokenMap::const_iterator it = tokenMap.find(token);
	if (it != tokenMap.end()) {
		o = it->second;
		if (assigned(o)) {
			if (o->value != value) {
				// Recalculate size of value for given token
				valueSize = valueSize - o->value.size() + value.size();
				o->value = value;
				if (invalidate) {
					deleteWriteBuffer();
				}
			}
			retVal = true;
		}
	}
	return retVal;
}


std::string TTokenParser::getTokenValue(const std::string& token) {
	std::string retVal = "";
	PToken o;
	TTokenMap::const_iterator it = tokenMap.find(token);
	if (it != tokenMap.end()) {
		o = it->second;
		if (assigned(o)) {
			std::lock_guard<std::mutex> lock(mtx);
			retVal = o->value;
		}
	}
	return retVal;
}


PParserBuffer TTokenParser::getWriteBuffer() {
	PParserBuffer o = nil;
	if (assigned(currentBuffer)) {
		if (currentBuffer->valid)
			o = currentBuffer;
	}

	if (!assigned(o)) {
		o = newWriteBuffer();
	}

	// Set reference count and time stamp
	currentBuffer = o;
	currentBuffer->refC++;
	currentBuffer->setTimeStamp();

	return currentBuffer;
}


PParserBuffer TTokenParser::newWriteBuffer() {
	PParserBuffer o = new TParserBuffer;

	// Add one char for zero terminated buffer
	o->size = rdBufferSize - tokenSize + valueSize;
	o->buffer = new char[o->size + 1];
	o->buffer[o->size] = '\0';
	o->valid = false;

	// Add current buffer to list
	bufferList.push_back(o);
	currentBuffer = o;

	//std::cout << "(N) CParserBuffer()" << std::endl;
	return o;
}


void TTokenParser::deleteWriteBuffer() {
	// "Virtually" delete current write write buffer object by reset valid flag
	// and null pointer to current write object
	if (assigned(currentBuffer)) {
		currentBuffer->valid = false;
		currentBuffer = nil;
	}
}


const PParserBuffer TTokenParser::getData() {
	std::lock_guard<std::mutex> lock(mtx);

	// Check for valid content of current buffer
	PParserBuffer o = getWriteBuffer();
	if (!o->valid) {
		tokenize(o->buffer, o->size);
		o->valid = true;
	}

	// Return current buffer
	return o;
}


int TTokenParser::decBufferRefCount() {
	std::lock_guard<std::mutex> lock(mtx);
	if (assigned(currentBuffer)) {
		if (currentBuffer->refC) {
			currentBuffer->refC--;
			currentBuffer->setTimeStamp();
			return currentBuffer->refC;
		}
	}
	return 0;
}


int TTokenParser::getBufferRefCount() {
	int retVal = 0;
	std::lock_guard<std::mutex> lock(mtx);
	if (assigned(currentBuffer))
		retVal = currentBuffer->refC;
	return retVal;
}


struct CBufferEraser {
	CBufferEraser(int age, time_t now) : _age(age), _now(now) {}
	int _age;
	time_t _now;
    bool operator () (PParserBuffer o) const {
    	bool retVal = (((_now - o->timestamp) > (_age / 1000)) && (o->refC <= 0) && !o->valid);
    	if (retVal) {
			if (assigned(o)) {
				util::freeAndNil(o);
			}
    	}
    	return retVal;
    }
};


size_t TTokenParser::deleteInvalidatedBuffers() {
	int deleted = 0;
	if (bufferList.size() > 0) {
		std::lock_guard<std::mutex> lock(mtx);
		size_t size = bufferList.size();
		time_t now = util::now();

		// Delete invalidated buffers by age
		bufferList.erase(std::remove_if(bufferList.begin(), bufferList.end(), CBufferEraser(BUFFER_DELETE_AGE, now)), bufferList.end());
		deleted = size - bufferList.size();
	}
	return deleted;
}


void TTokenParser::copy(char* dest, const char* src, size_t size) {
	if (size > 0) {
		memcpy(dest, src, size);
	}
}


int TTokenParser::tokenize(char* data, size_t max) {
	if (!util::assigned(data))
		return 0;

	/*
	std::cout << "(10.0) Original Token size = " << std::to_string((size_u)tokenSize) << std::endl;
	std::cout << "(10.1) Original Value size = " << std::to_string((size_u)valueSize) << std::endl;
	std::cout << "(10.2) Calculated Token size = " << std::to_string((size_u)calcTokenSize()) << std::endl;
	std::cout << "(10.3) Calculated Value size = " << std::to_string((size_u)calcValueSize()) << std::endl;
	std::cout << "(10.4) Write buffer size = " << std::to_string((size_u)wrBufferSize) << std::endl;
	std::cout << "(10.5) Read buffer size = " << std::to_string((size_u)rdBufferSize) << std::endl;
	std::cout << "(10.6) Calculated write buffer size = " << std::to_string((size_u)(rdBufferSize - tokenSize + valueSize)) << std::endl;
	*/

	const char *from = rdBuffer;
	char *to = data;
	size_t size;
	PToken o, prev = nil;
	TTokenList::const_iterator it = tokenList.begin();
	while (it != tokenList.end()) {
		o = *it;

		// Copy region until first item
		if (o->index == 0 && assigned(o->begin)) {
			from = rdBuffer;
			to = data;
			size = o->begin - rdBuffer + 1;
			copy(to, from, size);
			to += size;
		}

		// Copy region between previous item and current item
		if (assigned(prev)) {
			from = prev->end;
			size = o->begin - prev->end + 1;
			copy(to, from, size);
			to += size;
		}

		// Copy value from string to wrBuffer
		if (!o->value.empty()) {
			from = o->value.c_str();
			size = o->value.size();
			copy(to, from, size);
			to += size;
		}

		// Copy last region until rdBuffer end
		if (o->index == pred(tokenList.size()) && assigned(o->end)) {
			from = o->end;
			size = rdBufferSize - (o->end - rdBuffer);
			copy(to, from, size);
			to += size;
		}

		// Check write buffer range
		size = to - data;
		if (size > max)
			app_error("TTokenParser::replace() : Write buffer size exceeded, current offset = " + std::to_string((size_u)size) + ", buffer size = " + std::to_string((size_u)max));

		prev = o;
		it++;
	}
	//writeToFile(data, max, "/tmp/" + name);
	return tokenMap.size();
}



int TTokenParser::replace() {
	std::lock_guard<std::mutex> lock(mtx);
	int retVal;

	// Check for valid content of current buffer
	PParserBuffer o = getWriteBuffer();
	retVal = tokenize(o->buffer, o->size);
	o->valid = true;

	return retVal;
}



void TTokenParser::writeToFile(const char *const buffer, const size_t size, const std::string& fileName) {
	std::ofstream of;
	TStreamGuard<std::ofstream> strm(of);
	strm.open(fileName, std::ofstream::binary);
	of.write(buffer, size);
}


bool TTokenParser::findMask(const char*& p, const std::string& mask) {
	bool retVal = false;
	if (!mask.empty()) {
		// Check for first char of mask
		if (*p == mask[0]) {
			retVal = true;
			// Check for complete mask
			if (mask.size() > 1) {
				for (size_t i=1; i<mask.size(); ++i) {
					p++;
					if (*p != mask[i]) {
						retVal = false;
						break;
					}
				}
			}
		}
	}
	return retVal;
}


void TTokenParser::addToken(const char* maBegin, const char* maEnd, const char* toBegin, const char* toEnd) {

	/*
	std::cout << "addToken()" << std::endl \
	<< "maBegin\t" << std::to_string((size_s)(maBegin - rdBuffer)) << std::endl \
	<< "toBegin\t" << std::to_string((size_s)(toBegin - rdBuffer)) << std::endl \
	<< "toEnd\t" << std::to_string((size_s)(toEnd - rdBuffer)) << std::endl \
	<< "maEnd\t" << std::to_string((size_s)(maEnd - rdBuffer)) << std::endl;
	*/

	// Check if parameters are OK
	if (!util::assigned(maEnd))
		return;

	if (!util::assigned(maEnd))
		return;

	if (!util::assigned(toBegin))
		return;

	if (!util::assigned(toEnd))
		return;

	if (maEnd <= maBegin)
		return;

	if (toEnd <= toBegin)
		return;

	if (toBegin <= maBegin)
		return;

	if (maEnd <= toEnd)
		return;

	// Create new token from buffer data
	PToken o = new TToken;
	o->token = std::string(toBegin, toEnd - toBegin + 1);
	o->size = maEnd - maBegin + 1;
	o->value = o->token;
	o->begin = pred(maBegin);
	o->end = succ(maEnd);
	o->index = tokenMap.size();

	// Check if token/mask in valid buffer range
	if (o->begin <= rdBuffer)
		o->begin = nil;

	if (o->end >= rdBuffer + rdBufferSize)
		o->end = nil;

	// Calculate cumulative sizes
	tokenSize += o->size;
	valueSize += o->value.size();

	// Add token to map
	if (tokenMap.find(o->token) == tokenMap.end()) {
		tokenMap.insert(TTokenMapItem(o->token, o));
		tokenList.push_back(o);
	} else {
		// Double token strictly forbidden by design!
		throw app_error("TTokenParser::addToken() : Token <" + o->token + "> duplicated.");
	}

}


int TTokenParser::parse() {
	tokenSize = 0;
	valueSize = 0;
	if (rdBufferSize > 0 && assigned(rdBuffer)) {
		const char *eol = rdBuffer + rdBufferSize;
		const char *p = rdBuffer;
		const char *maBegin = nil, *maEnd = nil, *toBegin = nil, *toEnd = nil;
		EParseState state = ST_FIND_START_MASK;

		while (p < eol) {
			switch (state) {
				case ST_FIND_START_MASK:
					maBegin = p;
					if (findMask(p, startMask)) {
						state = ST_FIND_END_MASK;
						toBegin = succ(p);
					}
					break;

				case ST_FIND_END_MASK:
					toEnd = pred(p);
					if (findMask(p, endMask)) {
						state = ST_FIND_START_MASK;
						maEnd = p;
						addToken(maBegin, maEnd, toBegin, toEnd);
					}
					break;
			}
			// Goto next char
			p++;
		}
	}
	return tokenMap.size();
}


PToken TTokenParser::getToken(const std::string& key) const {
	PToken retVal = nil;
	util::TTokenMap::const_iterator it = tokenMap.find(key);
	if (it != tokenMap.end())
		retVal = it->second;
	return retVal;
}


} /* namespace util */
