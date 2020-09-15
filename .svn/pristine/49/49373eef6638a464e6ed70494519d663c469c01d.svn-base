/*
 * gzip.cpp
 *
 *  Created on: 07.05.2015
 *      Author: Dirk Brinkmeier
 */

#include <cstring>
#include <iostream>
#include "zlib/zlib.h"
#include "templates.h"
#include "exception.h"
#include "memory.h"
#include "gzip.h"

namespace util {

TZLib::TZLib(int level) : level(level) {
	window = Z_WINDOW_SIZE | Z_GZIP_ENCODING;
	zs = new z_stream;
}

TZLib::~TZLib() {
	util::freeAndNil(zs);
}

void TZLib::clear() {
	if (util::assigned(zs))
		memset(zs, 0, sizeof(z_stream));
}

std::string TZLib::actionToStr(EZLibAction action)
{
	std::string retVal("<Unkown ZLib action>");
    switch (action) {
		case EZ_DECOMPRESS:
	        retVal = "decompress";
			break;

		case EZ_COMPRESS:
	        retVal = "compress";
			break;
	}
	return retVal;
}


size_t TZLib::action(EZLibAction action, const char* const src, char*& dst, size_t ssize)
{
	int retVal = Z_ERRNO;
	if (!util::assigned(src) || ssize <= 0)
		return retVal;

	size_t n = (action == EZ_DECOMPRESS) ? 2 * ssize : ssize;
	TBuffer p(0, false);
	TBuffer q(n);
	clear();

	// Clear destination buffer
	if (util::assigned(dst))
		delete[] dst;
	dst = nil;

	// Set ZLib control structure
	switch (action) {
		case EZ_DECOMPRESS:
			retVal = inflateInit2( zs,
								   window );
			break;

		case EZ_COMPRESS:
			retVal = deflateInit2( zs,
								   level,
								   Z_DEFLATED,
								   window,
								   Z_MEM_LEVEL,
								   Z_DEFAULT_STRATEGY );
			break;
	}

	// Check if control structure OK
	if (retVal != Z_OK) {
		if (!util::assigned(zs->msg))
			throw app_error("TZLib::" + actionToStr(action) + ".init() failed with error code " + std::to_string((size_s)retVal));
		else
			throw app_error("TZLib::" + actionToStr(action) + ".init() failed with error <" + std::string(zs->msg) + ">");
	}

    // Set source buffer
	zs->next_in = (Bytef*)src;
    zs->avail_in = ssize;

    // Compress   --> deflate buffer
    // Decompress --> inflate buffer
    do {
        zs->next_out = (Bytef*)q.data();
        zs->avail_out = q.size();

    	// Execute appropriate ZLib function
        switch (action) {
    		case EZ_DECOMPRESS:
    	        retVal = ::inflate(zs, Z_NO_FLUSH);
    			break;

    		case EZ_COMPRESS:
    	        retVal = ::deflate(zs, Z_FINISH);
    			break;
    	}

        if (zs->total_out > 0)
        	p.append(q.data(), zs->total_out - p.size());

    } while (retVal == Z_OK);

	// Finalize ZLib action
    switch (action) {
		case EZ_DECOMPRESS:
		    inflateEnd(zs);
			break;

		case EZ_COMPRESS:
		    deflateEnd(zs);
			break;
	}

    // Check for serious ZLib errors
	if (retVal != Z_STREAM_END) {
		if (!util::assigned(zs->msg))
			throw app_error("TZLib::" + actionToStr(action) + "() failed with error code " + std::to_string((size_s)retVal));
		else
			throw app_error("TZLib::" + actionToStr(action) + "() failed with error <" + std::string(zs->msg) + ">");
	}

    // Return destination buffer and size
    dst = p.data();
    return p.size();
}


} /* namespace util */
