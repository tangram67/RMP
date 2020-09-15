/*
 * sha.cpp
 *
 *  Created on: 03.08.2015
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include "exception.h"
#include "sha.h"
#include "encoding.h"

namespace util {


TSHA1::TSHA1() {
	result20 = new UINT_8[SHA1_LEN];
	init();
}

TSHA1::~TSHA1() {
	delete[] result20;
}

void TSHA1::init() {
	valid = false;
	buffer.resize(4 * SHA1_LEN, false);
	reportType = ERT_HEX_SHORT_NCASE;
	if (util::assigned(result20))
		memset(result20, 0, SHA1_LEN * sizeof(UINT_8));
}

std::string TSHA1::getSHA1(const char* data, size_t size) {
	if (util::assigned(data) && size > 0) {
		UINT_32 c;
		size_t n = 0;
		if (size > static_cast<size_t>(MAX_BUFFER_SIZE))
			c = MAX_BUFFER_SIZE;
		else
			c = static_cast<UINT_32>(size);
		const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
		if (util::assigned(p) && c > 0) {
			sha.Reset();
			do {
				sha.Update(p, c);
				p += c;
				n += c;
				if ((n + c) > size)
					c = size - n;
			} while (n < size);
			sha.Final();
			sha.GetHash(result20);
			valid = true;
			if (reportType == ERT_BASE64) {
				return util::TBase64::encode((char*)result20, 20);
			} else {
				sha.ReportHash(buffer.data(), convertType());
				return std::string(buffer.data(), getResultSize());
			}
		}
	}
	return "";
}

std::string TSHA1::getSHA1(const std::string data) {
	if (!data.empty())
		return getSHA1(data.c_str(), data.size());
	return "";
}

std::string TSHA1::getSHA1() {
	if (valid) {
		sha.ReportHash(buffer.data(), convertType());
		return std::string(buffer.data(), getResultSize());
	}
	return "";
}

CSHA1::REPORT_TYPE TSHA1::convertType() {
	CSHA1::REPORT_TYPE retVal = CSHA1::REPORT_HEX_SHORT_NCASE;
	switch (reportType) {
		case ERT_HEX:
			retVal = CSHA1::REPORT_HEX;
			break;
		case ERT_HEX_SHORT:
			retVal = CSHA1::REPORT_HEX_SHORT;
			break;
		case ERT_HEX_NCASE:
			retVal = CSHA1::REPORT_HEX_NCASE;
			break;
		case ERT_HEX_SHORT_NCASE:
		default:
			retVal = CSHA1::REPORT_HEX_SHORT_NCASE;
			break;
	}
	return retVal;
}

size_t TSHA1::getResultSize() {
	size_t retVal = 0;
	switch (reportType) {
		case ERT_HEX_SHORT_NCASE:
		case ERT_HEX_SHORT:
			retVal = 40;
			break;
		case ERT_HEX_NCASE:
		case ERT_HEX:
			retVal = 59;
			break;
		case ERT_DIGIT:
		default:
			retVal = strnlen(buffer.data(), 4 * SHA1_LEN);
			break;
	}
	return retVal;
}

bool TSHA1::compare(const UINT_8* value20) const {
	bool retVal = false;
	if (util::assigned(value20) && util::assigned(result20)) {
		const UINT_8* p = value20;
		const UINT_8* q = result20;
		for (size_t i=0; i<SHA1_LEN; i++, p++, q++) {
			if (*p != *q) {
				retVal = false;
				break;
			}	
			retVal = true;
		}
	}
	return retVal;
}

void TSHA1::setFormat(const EReportType type) {
	if (reportType != type && type == ERT_DIGIT) {
		buffer.fillchar(0);
	}
	reportType = type;
};

} /* namespace util */
