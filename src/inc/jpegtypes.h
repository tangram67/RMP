/*
 * jpegtypes.h
 *
 *  Created on: 30.09.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef INC_JPEGTYPES_H_
#define INC_JPEGTYPES_H_

#include <jpeglib.h>
#include "memory.h"
#include "gcc.h"

namespace util {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TRGB = JSAMPLE;
using PRGB = TRGB*;
using TRGBData = TDataBuffer<TRGB>;
using TRGBSize = JDIMENSION;

#else

typedef JSAMPLE TRGB;
typedef TRGB* PRGB;
typedef TDataBuffer<TRGB> TRGBData;
typedef JDIMENSION TRGBSize;

#endif


#ifdef STL_HAS_CONSTEXPR

static constexpr size_t RGB_SIZE = sizeof(TRGB);
static constexpr size_t MIN_JPEG_SIZE = 9;
static constexpr size_t MIN_EXIF_SIZE = 11;
static constexpr size_t MIN_PNG_SIZE = 10;

#else

static const size_t RGB_SIZE = sizeof(TRGB);
static const size_t MIN_JPEG_SIZE = 9;
static const size_t MIN_EXIF_SIZE = 11;
static const size_t MIN_PNG_SIZE = 10;

#endif


} /* namespace util */

#endif /* INC_JPEGTYPES_H_ */
