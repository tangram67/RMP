/*
 * endian.h
 *
 *  Created on: 21.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ENDIANUTILS_H_
#define ENDIANUTILS_H_

#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#define BSWAP16(x) ((((x) << 8) | \
					(((x) >> 8) & UINT16_C(0x00FF))))

#define BSWAP32(x) ((((x) << 24) | \
					(((x) << 8)  & UINT32_C(0x00FF0000)) | \
					(((x) >> 8)  & UINT32_C(0x0000FF00)) | \
					(((x) >> 24) & UINT32_C(0x000000FF))))

#define BSWAP64(x) ((((x) << 56) | \
					(((x) << 40) & UINT64_C(0x00FF000000000000)) | \
                    (((x) << 24) & UINT64_C(0x0000FF0000000000)) | \
                    (((x) << 8)  & UINT64_C(0x000000FF00000000)) | \
                    (((x) >> 8)  & UINT64_C(0x00000000FF000000)) | \
                    (((x) >> 24) & UINT64_C(0x0000000000FF0000)) | \
                    (((x) >> 40) & UINT64_C(0x000000000000FF00)) | \
                    (((x) >> 56) & UINT64_C(0x00000000000000FF))))

namespace util {

// See https://sourceforge.net/p/predef/wiki/Endianness/
enum EEndianType {
	EE_UNKNOWN_ENDIAN,
	EE_BIG_ENDIAN,
	EE_LITTLE_ENDIAN,
	EE_BIG_WORD_ENDIAN,    /* Middle-endian, Honeywell 316 style */
	EE_LITTLE_WORD_ENDIAN, /* Middle-endian, PDP-11 style */
	EE_NETWORK_BYTE_ORDER = EE_BIG_ENDIAN
};

class TEndian {
public:
	uint64_t ntoh64(uint64_t value) const;
	uint32_t ntoh32(uint32_t value) const;
	uint16_t ntoh16(uint16_t value) const;

	uint64_t hton64(uint64_t value) const;
	uint32_t hton32(uint32_t value) const;
	uint16_t hton16(uint16_t value) const;

	double ntoh64d(uint64_t value) const;
	double ntoh32d(uint32_t value) const;
	uint64_t hton64d(double value) const;
	uint32_t hton32d(float value) const;

	uint64_t swap64(uint64_t value) const;
	uint32_t swap32(uint32_t value) const;
	uint16_t swap16(uint16_t value) const;

	int64_t swap64(int64_t value) const;
	int32_t swap32(int32_t value) const;
	int16_t swap16(int16_t value) const;

	uint64_t convertFromLittleEndian64(uint64_t value) const;
	uint32_t convertFromLittleEndian32(uint32_t value) const;
	uint16_t convertFromLittleEndian16(uint16_t value) const;
	uint64_t convertFromBigEndian64(uint64_t value) const;
	uint32_t convertFromBigEndian32(uint32_t value) const;
	uint16_t convertFromBigEndian16(uint16_t value) const;

	int64_t convertFromLittleEndian64(int64_t value) const;
	int32_t convertFromLittleEndian32(int32_t value) const;
	int16_t convertFromLittleEndian16(int16_t value) const;
	int64_t convertFromBigEndian64(int64_t value) const;
	int32_t convertFromBigEndian32(int32_t value) const;
	int16_t convertFromBigEndian16(int16_t value) const;

	double convertFromLittleEndianDouble64(double value) const;
	double convertFromBigEndianDouble64(double value) const;

	static EEndianType getSystemEndian();
	static EEndianType detectEndian();
	static bool isLittleEndian();
	static bool isBigEndian();

	TEndian();
	virtual ~TEndian();
};

} /* namespace music */

#endif /* ENDIANUTILS_H_ */
