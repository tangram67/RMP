/*
 * encoding.h
 *
 *  Created on: 20.04.2020
 *      Author: dirk
 */

#ifndef INC_ENCODING_H_
#define INC_ENCODING_H_

#include <string>
#include "blob.h"
#include "windows.h"
#include "stringconsts.h"

namespace util {

class TBase36 {
public:
	static std::string randomize(const std::string& preamble, const size_t digits, const bool upper = false);

	static void encode(const uint64_t value, std::string& dst, const bool upper = false);
	static std::string encode(const uint64_t value, const bool upper = false);

	static uint64_t decode(const char *src, const size_t size, const uint64_t defVal = 0);
	static uint64_t decode(const std::string& src, const uint64_t defVal = 0);
};


class TBase64 {
public:
	template<typename output_t>
	static void encode(const char* src, size_t size, output_t&& dst) {
		dst.clear();
		if (!util::assigned(src) || size <= 0)
			return;

		ULONG value;
		size_t idx;
		dst.reserve(((size + 2) / 3) * 4);
		const char* p = src;

		for (idx = 0; idx < size; idx += 3) {
			value  = ((((ULONG)*p++) << 16) & 0xFFFFFF);
			value |= ((((idx + 1) < size) ? (((ULONG)*p++) << 8) : 0) & 0xFFFF);
			value |= ((((idx + 2) < size) ? (((ULONG)*p++) << 0) : 0) & 0x00FF);

			dst.push_back((char)BASE64_CODE_TABLE[(value >> 18) & 0x3F]);
			dst.push_back((char)BASE64_CODE_TABLE[(value >> 12) & 0x3F]);

			if (idx + 1 < size)
				dst.push_back((char)BASE64_CODE_TABLE[(value >> 6) & 0x3F]);
			if (idx + 2 < size)
				dst.push_back((char)BASE64_CODE_TABLE[(value >> 0) & 0x3F]);
		}

		size_t left = 3 - (size % 3);
		if (size % 3) {
			for (idx = 0; idx < left; idx++)
				dst.push_back(BASE64_PADDING_CHAR);
		}
	}

	template<typename output_t>
	static void decode(const char* src, size_t size, output_t&& dst) {
		dst.clear();
		if (!util::assigned(src) || size <= 0)
			return;

		size_t idx;
		if (src[util::pred(size)] == BASE64_PADDING_CHAR) {
			for (idx = util::pred(size); idx >= 0; idx--) {
				if (src[idx] != BASE64_PADDING_CHAR) {
					size = util::succ(idx);
					break;
				}
			}
		}
		if (size <= 0)
			return;

		dst.reserve(size - ((size + 2) / 4));
		const char* p = src;

		for(idx = 0; idx < size; idx += 4) {
			UCHAR s1 = BASE64_LOOKUP_TABLE[(UCHAR)*p++];
			UCHAR s2 = BASE64_LOOKUP_TABLE[(UCHAR)*p++];
			UCHAR s3 = BASE64_LOOKUP_TABLE[(UCHAR)*p++];
			UCHAR s4 = BASE64_LOOKUP_TABLE[(UCHAR)*p++];

			UCHAR d1 = ((s1 & 0x3f) << 2) | ((s2 & 0x30) >> 4);
			UCHAR d2 = ((s2 & 0x0f) << 4) | ((s3 & 0x3c) >> 2);
			UCHAR d3 = ((s3 & 0x03) << 6) | ((s4 & 0x3f) >> 0);

			dst.push_back((char)d1);
			if (s3 == 99u) break; // End padding found

			dst.push_back((char)d2);
			if (s4 == 99u) break; // End padding found

			dst.push_back((char)d3);
		}
	}

	static std::string encode(const char* src, size_t size);
	static void encode(const std::string& src, std::string& dst);
	static std::string encode(const std::string& src);

	static std::string decode(const char* src, size_t size);
	static void decode(const std::string& src, std::string& dst);
	static std::string decode(const std::string& src);
};


class TURL {
public:
	enum ESpaceTrait {
		UST_ENCODED,
		UST_REPLACED,
		UST_DEFAULT = UST_ENCODED
	};
	enum EEncodeType {
		URL_EXTENDED,
		URL_RFC3986,
		URL_DEFAULT = URL_EXTENDED
	};
	static std::string decode(const std::string& url);
	static std::string encode(const std::string& url, const EEncodeType type = URL_DEFAULT, const ESpaceTrait trait = UST_DEFAULT);

private:
	static bool isValidRFC3986Delimiter(const char c);
	static bool isAlphaNumeric(const char c);
	static bool isWhiteSpace(const char c, const ESpaceTrait trait);
	static bool isUriDelimiter(const char c, const EEncodeType type);
};

} /* namespace util */

#endif /* INC_ENCODING_H_ */
