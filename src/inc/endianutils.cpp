/*
 * endian.cpp
 *
 *  Created on: 21.08.2016
 *      Author: Dirk Brinkmeier
 */

#include <cmath>
#include "gcc.h"
#include "endianutils.h"
#include "../config.h"
#ifdef USE_GLIBC_SWAP_MACROS
# include <byteswap.h>
#endif

namespace util {

TEndian::TEndian() {
}

TEndian::~TEndian() {
}


uint64_t TEndian::swap64(uint64_t value) const {
#ifdef USE_GLIBC_SWAP_MACROS
	return bswap_64(value);
#else
	return BSWAP64(value);
#endif
}

uint32_t TEndian::swap32(uint32_t value) const {
#ifdef USE_GLIBC_SWAP_MACROS
	return bswap_32(value);
#else
	return BSWAP32(value);
#endif
}

uint16_t TEndian::swap16(uint16_t value) const {
#ifdef USE_GLIBC_SWAP_MACROS
	return bswap_16(value);
#else
	return BSWAP16(value);
#endif
}


int64_t TEndian::swap64(int64_t value) const {
	union {
		int64_t value;
		uint64_t uval;
	} swap;
	swap.value = value;
	swap.uval = swap64(swap.uval);
	return swap.value;
}

int32_t TEndian::swap32(int32_t value) const {
	union {
		int32_t value;
		uint32_t uval;
	} swap;
	swap.value = value;
	swap.uval = swap32(swap.uval);
	return swap.value;
}

int16_t TEndian::swap16(int16_t value) const {
	union {
		int16_t value;
		uint16_t uval;
	} swap;
	swap.value = value;
	swap.uval = swap16(swap.uval);
	return swap.value;
}


double TEndian::ntoh64d(uint64_t value) const {
	union {
		uint64_t value;
		double retval;
	} cu;
	cu.value = ntoh64(value);
	return cu.retval;
}

double TEndian::ntoh32d(uint32_t value) const {
	union {
		uint32_t value;
		float retval;
	} cu;
	cu.value = ntoh32(value);
	return cu.retval;
}


uint64_t TEndian::hton64d(double value) const {
	union {
		double value;
		uint64_t retval;
	} cu;
	cu.value = value;;
	return hton64(cu.retval);
}

uint32_t TEndian::hton32d(float value) const {
	union {
		float value;
		uint32_t retval;
	} cu;
	cu.value = value;
	return hton32(cu.retval);
}


uint64_t TEndian::ntoh64(uint64_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap64(value);
#endif
	return value;
}

uint32_t TEndian::ntoh32(uint32_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap32(value);
#endif
	return value;
}

uint16_t TEndian::ntoh16(uint16_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap16(value);
#endif
	return value;
}


uint64_t TEndian::hton64(uint64_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap64(value);
#endif
	return value;
}

uint32_t TEndian::hton32(uint32_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap32(value);
#endif
	return value;
}

uint16_t TEndian::hton16(uint16_t value) const {
#ifdef TARGET_LITTLE_ENDIAN
	return swap16(value);
#endif
	return value;
}


double TEndian::convertFromLittleEndianDouble64(double value) const {
#ifdef TARGET_FLOAT_BIG_ENDIAN
	union {
		double float64;
		uint64_t integer64;
	} swap;
	swap.float64 = value;
	swap.integer64 = swap64(swap.integer64);
	return swap.integer64;
#else
	return value;
#endif
}

double TEndian::convertFromBigEndianDouble64(double value) const {
#ifdef TARGET_FLOAT_LITTLE_ENDIAN
	union {
		double float64;
		uint64_t integer64;
	} swap;
	swap.float64 = value;
	swap.integer64 = swap64(swap.integer64);
	return swap.integer64;
#else
	return value;
#endif
}


uint64_t TEndian::convertFromLittleEndian64(uint64_t value) const {
	if (value == 0)
		return (uint64_t)0;
#ifdef TARGET_BIG_ENDIAN
	value = swap64(value);
#endif
	return value;
}

uint32_t TEndian::convertFromLittleEndian32(uint32_t value) const {
	if (value == 0)
		return (uint32_t)0;
#ifdef TARGET_BIG_ENDIAN
	value = swap32(value);
#endif
	return value;
}

uint16_t TEndian::convertFromLittleEndian16(uint16_t value) const {
	if (value == 0)
		return (uint16_t)0;
#ifdef TARGET_BIG_ENDIAN
	value = swap16(value);
#endif
	return value;
}


uint64_t TEndian::convertFromBigEndian64(uint64_t value) const {
	if (value == 0)
		return (uint64_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	value = swap64(value);
#endif
	return value;
}

uint32_t TEndian::convertFromBigEndian32(uint32_t value) const {
	if (value == 0)
		return (uint32_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	value = swap32(value);
#endif
	return value;
}

uint16_t TEndian::convertFromBigEndian16(uint16_t value) const {
	if (value == 0)
		return (uint16_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	value = swap16(value);
#endif
	return value;
}


int64_t TEndian::convertFromLittleEndian64(int64_t value) const {
	if (value == 0)
		return (int64_t)0;
#ifdef TARGET_BIG_ENDIAN
	return swap64(value);
#endif
	return value;
}

int32_t TEndian::convertFromLittleEndian32(int32_t value) const {
	if (value == 0)
		return (int32_t)0;
#ifdef TARGET_BIG_ENDIAN
	return swap32(value);
#endif
	return value;
}

int16_t TEndian::convertFromLittleEndian16(int16_t value) const {
	if (value == 0)
		return (int16_t)0;
#ifdef TARGET_BIG_ENDIAN
	return swap16(value);
#endif
	return value;
}


int64_t TEndian::convertFromBigEndian64(int64_t value) const {
	if (value == 0)
		return (int64_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	return swap64(value);
#endif
	return value;
}

int32_t TEndian::convertFromBigEndian32(int32_t value) const {
	if (value == 0)
		return (int32_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	return swap32(value);
#endif
	return value;
}

int16_t TEndian::convertFromBigEndian16(int16_t value) const {
	if (value == 0)
		return (int16_t)0;
#ifdef TARGET_LITTLE_ENDIAN
	return swap16(value);
#endif
	return value;
}


bool TEndian::isLittleEndian() {
#ifdef TARGET_LITTLE_ENDIAN
	return true;
#endif
	return false;
}

bool TEndian::isBigEndian() {
#ifdef TARGET_BIG_ENDIAN
	return true;
#endif
	return false;
}


EEndianType TEndian::getSystemEndian() {
#ifdef TARGET_LITTLE_ENDIAN
	return EE_LITTLE_ENDIAN;
#endif
#ifdef TARGET_BIG_ENDIAN
	return EE_BIG_ENDIAN;
#endif
	return EE_UNKNOWN_ENDIAN;
}


EEndianType TEndian::detectEndian() {
	union {
		uint32_t value;
		uint8_t data[sizeof(uint32_t)];
	} number;

	number.data[0] = 0x00;
	number.data[1] = 0x01;
	number.data[2] = 0x02;
	number.data[3] = 0x03;

	switch (number.value) {
		case 0x00010203U:
			return EE_BIG_ENDIAN;
		case UINT32_C(0x03020100):
			return EE_LITTLE_ENDIAN;
		case UINT32_C(0x02030001):
			return EE_BIG_WORD_ENDIAN;
		case UINT32_C(0x01000302):
			return EE_LITTLE_WORD_ENDIAN;
		default:
			return EE_UNKNOWN_ENDIAN;
	}
}



} /* namespace music */
