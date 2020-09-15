/*
 * gzip.h
 *
 *  Created on: 07.05.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef GZIP_H_
#define GZIP_H_

#include "zlib/zlib.h"
#include "gcc.h"

namespace util {

#ifdef STL_HAS_CONSTEXPR

static constexpr const int Z_WINDOW_SIZE = 15;
static constexpr const int Z_GZIP_ENCODING = 16;
static constexpr const int Z_MEM_LEVEL = 9;

#else

# define Z_WINDOW_SIZE 15
# define Z_GZIP_ENCODING 16
# define Z_MEM_LEVEL 9

#endif

#if defined ZLIB_VER_MAJOR && defined ZLIB_VER_MINOR && defined ZLIB_VER_REVISION
# define GZIP_VERSION ( ZLIB_VER_MAJOR * 10000 \
					   + ZLIB_VER_MINOR * 100 \
					   + ZLIB_VER_REVISION )
#else
# define GZIP_VERSION 0
#endif

class TZLib {
private:
	enum EZLibAction { EZ_COMPRESS, EZ_DECOMPRESS };

	z_stream* zs;
	int window;
	int level;
	void clear();
	std::string actionToStr(EZLibAction action);
	size_t action(EZLibAction action, const char* const src, char*& dst, size_t ssize);

public:
	inline size_t gzip(const char* const src, char*& dst, size_t size) {
		window = Z_WINDOW_SIZE | Z_GZIP_ENCODING;
		return action(EZ_COMPRESS, src, dst, size);
	}
	inline size_t gunzip(const  char* const src, char*& dst, size_t size) {
		window = Z_WINDOW_SIZE | Z_GZIP_ENCODING;
		return action(EZ_DECOMPRESS, src, dst, size);
	}
	inline size_t deflate(const char* const src, char*& dst, size_t size) {
		window = Z_WINDOW_SIZE;
		return action(EZ_COMPRESS, src, dst, size);
	}
	inline size_t inflate(const char* const src, char*& dst, size_t size) {
		window = Z_WINDOW_SIZE;
		return action(EZ_DECOMPRESS, src, dst, size);
	}

	TZLib(int level = Z_BEST_COMPRESSION);
	virtual ~TZLib();
};

} /* namespace util */

#endif /* GZIP_H_ */
