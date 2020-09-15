/*
 * endian.h
 *
 *  Created on: 23.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <endian.h>
#include <features.h>
#include <sys/param.h>

// Endianess
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
  defined(__BIG_ENDIAN__) || \
  defined(__ARMEB__) || \
  defined(__THUMBEB__) || \
  defined(__AARCH64EB__) || \
  defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#  define TARGET_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
  defined(__LITTLE_ENDIAN__) || \
  defined(__ARMEL__) || \
  defined(__THUMBEL__) || \
  defined(__AARCH64EL__) || \
  defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#  define TARGET_LITTLE_ENDIAN
#else
#  error "PREPROCESSOR ERROR ID3v2:gcc.h: No endianess defined."
#endif

#endif /* ENDIAN_H_ */
