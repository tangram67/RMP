/*
 * convert.h
 *
 *  Created on: 07.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef CONVERT_H_
#define CONVERT_H_

#include "syslocale.h"
#include "converttemplates.h"

namespace util {

enum EValueDomain { VD_SI, VD_BINARY, VD_BIT };

double strToDouble(const std::string& value, double defValue = 0.0, const app::TLocale& locale = syslocale);
long double strToLongDouble(const std::string& value, long double defValue, const app::TLocale& locale = syslocale);
float strToFloat(const std::string& value, float defValue = 0.0, const app::TLocale& locale = syslocale);
int32_t strToInt(const std::string& value, int32_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
uint32_t strToUnsigned(const std::string& value, uint32_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
int64_t strToInt64(const std::string& value, int64_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
uint64_t strToUnsigned64(const std::string& value, uint64_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);

double strToDouble(const std::wstring& value, double defValue = 0.0, const app::TLocale& locale = syslocale);
long double strToLongDouble(const std::wstring& value, long double defValue, const app::TLocale& locale = syslocale);
float strToFloat(const std::wstring& value, float defValue = 0.0, const app::TLocale& locale = syslocale);
int32_t strToInt(const std::wstring& value, int32_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
uint32_t strToUnsigned(const std::wstring& value, uint32_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
int64_t strToInt64(const std::wstring& value, int64_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
uint64_t strToUnsigned64(const std::wstring& value, uint64_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);

size_t strToSize(const std::string& value, std::string& unit, size_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
size_t strToSize(const std::string& value, EValueDomain valueDomain = VD_BINARY, size_t defValue = 0, const app::TLocale& locale = syslocale, int base = 10);
std::string sizeToStr(size_t size, unsigned int precision = 1, EValueDomain valueDomain = VD_BINARY, const app::TLocale& locale = syslocale);

} /* namespace util */

#endif /* CONVERT_H_ */
