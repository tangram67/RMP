/*
 * bits.cpp
 *
 *  Created on: 17.09.2016
 *      Author: Dirk Brinkmeier
 */

#include "bits.h"

namespace util {

/**
 * @see http://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
 */
const uint8_t bitReverseTable[256] = {
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
		 R6(0), R6(2), R6(1), R6(3)
};

const bool parityLookupTable[256] = {
#define P2(n) n, n^1, n^1, n
#define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
		 P6(0), P6(1), P6(1), P6(0)
};

const uint8_t bitCountLookupTable[256] = {
#define B2(n) n, n+1, n+1, n+2
#define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
		 B6(0), B6(1), B6(1), B6(2)
};

} /* namespace util */
