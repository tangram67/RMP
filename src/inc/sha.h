/*
 * sha.h
 *
 *  Created on: 03.08.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef SHA_H_
#define SHA_H_

#include "sha1/SHA1.h"
#include "ssltypes.h"
#include "memory.h"
#include "gcc.h"

namespace util {

class TSHA1 {
private:
	STATIC_CONST UINT_32 LIMIT_UINT32 = static_cast<UINT_32>(-1);
	STATIC_CONST UINT_32 MAX_BUFFER_SIZE = LIMIT_UINT32 / 2;
	STATIC_CONST size_t SHA1_LEN = 20;
	EReportType reportType;
	util::TBuffer buffer;
	UINT_8* result20;
	CSHA1 sha;
	bool valid;
	void init();
	bool compare(const UINT_8* value20) const;
	size_t getResultSize();
	CSHA1::REPORT_TYPE convertType();

public:
	void setFormat (EReportType type);
	EReportType getFormat () const { return reportType; };
	std::string getSHA1(const char* data, size_t size);
	std::string getSHA1(const std::string data);
	std::string getSHA1();
	inline std::string operator () (const char* data, size_t size) { return getSHA1(data, size); };
	inline std::string operator () (const std::string& data) { return getSHA1(data); };
	inline bool operator == (const TSHA1& value) const { return compare(value.result20); };
	inline bool operator != (const TSHA1& value) const { return !compare(value.result20); };

	TSHA1();
	virtual ~TSHA1();
};

} /* namespace util */

#endif /* SHA_H_ */
