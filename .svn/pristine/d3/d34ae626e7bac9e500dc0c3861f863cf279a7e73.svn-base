/*
 * bits.h
 *
 *  Created on: 17.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef BITS_H_
#define BITS_H_

#include "gcc.h"

namespace util {

extern const uint8_t bitReverseTable[256];
extern const bool parityLookupTable[256];
extern const uint8_t bitCountLookupTable[256];

static inline uint8_t bitReverse(uint8_t value) {
	return bitReverseTable[value];
}

static inline bool bitParity(uint8_t value) {
	return parityLookupTable[value];
}

static inline bool bitParity(uint32_t value) {
	value ^= value >> 16;
	value ^= value >> 8;
	return parityLookupTable[value];
}

static inline size_t bitCount(uint32_t value) {
	return bitCountLookupTable[value & 0xff] +
			bitCountLookupTable[(value >> 8) & 0xff] +
			bitCountLookupTable[(value >> 16) & 0xff] +
			bitCountLookupTable[value >> 24];
}

} /* namespace util */

#endif /* BITS_H_ */
