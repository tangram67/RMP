/*
 * convert.cpp
 *
 *  Created on: 07.10.2015
 *      Author: Dirk Brinkmeier
 */

#include <errno.h>
#include <cstdlib>
#include "memory.h"
#include "templates.h"
#include "stringutils.h"
#include "localization.h"
#include "numlimits.h"
#include "exception.h"
#include "convert.h"
#include "gcc.h"

using namespace util;


double util::strToDouble(const std::string& value, double defValue, const app::TLocale& locale) {
	double retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtod_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


long double util::strToLongDouble(const std::string& value, long double defValue, const app::TLocale& locale) {
	long double retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtold_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


float util::strToFloat(const std::string& value, float defValue, const app::TLocale& locale) {
	float retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtof_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


int32_t util::strToInt(const std::string& value, int32_t defValue, const app::TLocale& locale, int base) {
	long int retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtol_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q )
			retVal = defValue;

		if (retVal > (long int)TLimits::LIMIT_INT32_MAX ||
			retVal < (long int)TLimits::LIMIT_INT32_MIN)
			retVal = defValue;
	}
	return (int32_t)retVal;
}


uint32_t util::strToUnsigned(const std::string& value, uint32_t defValue, const app::TLocale& locale, int base) {
	unsigned long int retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtoul_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;

		if (retVal > (unsigned long int)TLimits::LIMIT_UINT32_MAX)
			retVal = defValue;
	}
	return (uint32_t)retVal;
}


int64_t util::strToInt64(const std::string& value, int64_t defValue, const app::TLocale& locale, int base) {
	int64_t retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtoll_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


uint64_t util::strToUnsigned64(const std::string& value, uint64_t defValue, const app::TLocale& locale, int base) {
	uint64_t retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		char* q;
		const char *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = strtoull_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


double util::strToDouble(const std::wstring& value, double defValue, const app::TLocale& locale) {
	double retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstod_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


long double util::strToLongDouble(const std::wstring& value, long double defValue, const app::TLocale& locale) {
	long double retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstold_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


float util::strToFloat(const std::wstring& value, float defValue, const app::TLocale& locale) {
	float retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstof_l(p, &q, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


int32_t util::strToInt(const std::wstring& value, int32_t defValue, const app::TLocale& locale, int base) {
	long int retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstol_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;

		if (retVal > (long int)TLimits::LIMIT_INT32_MAX ||
			retVal < (long int)TLimits::LIMIT_INT32_MIN)
			retVal = defValue;
	}
	return (int32_t)retVal;
}


uint32_t util::strToUnsigned(const std::wstring& value, uint32_t defValue, const app::TLocale& locale, int base) {
	unsigned long int retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstoul_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;

		if (retVal > (unsigned long int)TLimits::LIMIT_UINT32_MAX)
			retVal = defValue;
	}
	return (uint32_t)retVal;
}


int64_t util::strToInt64(const std::wstring& value, int64_t defValue, const app::TLocale& locale, int base) {
	int64_t retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstoll_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}


uint64_t util::strToUnsigned64(const std::wstring& value, uint64_t defValue, const app::TLocale& locale, int base) {
	uint64_t retVal = defValue;
	if (!value.empty()) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		wchar_t* q;
		const wchar_t *const p = value.c_str();
		errno = EXIT_SUCCESS;
		retVal = wcstoull_l(p, &q, base, local);
		if (EXIT_SUCCESS != errno || (p + value.size()) != q)
			retVal = defValue;
	}
	return retVal;
}



size_t util::strToSize(const std::string& value, std::string& unit, size_t defValue, const app::TLocale& locale, int base) {
	if (value.empty()) {
		unit.clear();
		return (size_t)0;
	}

	locale_t local = (locale_t)0;
	app::TLocaleGuard<locale_t> guard(local, locale.getType());
	locale.duplicate(locale, local);

	const char *const p = value.c_str();
	size_t retVal;
	char* q;
	unit.clear();
	errno = EXIT_SUCCESS;

	// Convert digits to value
	retVal = strtoul_l(p, &q, base, local);
	if (EXIT_SUCCESS != errno || (p == q))
		retVal = defValue;

	// Return unit (trimmed rest of string)
	if (*q != '\0') {
		size_t pos = q - p;
		if (pos > 0 && pos < value.size()) {
			std::string s(q, value.size() - pos);
			unit = trim(s);
			if (!unit.empty())
				replace(unit, " ", "");
		}
	}

	return (size_t)retVal;
}


size_t util::strToSize(const std::string& value, EValueDomain valueDomain, size_t defValue, const app::TLocale& locale, int base) {
	std::string unit;
	size_t retVal = strToSize(value, unit, defValue, locale, base);
	if (retVal > 0 && !unit.empty()) {
//		size_t d = 1024;
//		if (valueDomain == VD_SI)
//	    	d = 1000;
		char c = ::tolower(unit[0]);
		switch (c) {
			case 'b':
				// 'b' for "Byte" is obsolete!
				break;
			case 'k':
				//retVal *= d; // VD_BINARY: 10 Bit, VD_SI: 10^3 --> kilo
				retVal *= (valueDomain == VD_SI) ? 1000U: 1024U;
				break;
			case 'm':
				//retVal *= d * d; // VD_BINARY: 20 Bit, VD_SI: 10^6 --> mega
				retVal *= (valueDomain == VD_SI) ? 1000000UL : 1048576UL;
				break;
			case 'g':
				//retVal *= d * d * d; // VD_BINARY: 30 Bit, VD_SI: 10^9 --> giga
				retVal *= (valueDomain == VD_SI) ? 1000000000UL : 1073741824UL;
				break;
#ifdef TARGET_X64
			case 't':
				//retVal *= d * d * d * d; // VD_BINARY: 40 Bit, VD_SI: 10^12 --> tera
				retVal *= (valueDomain == VD_SI) ? 1000000000000LLU : 1099511627776LLU;
				break;
			case 'e':
				//retVal *= d * d * d * d * d; // VD_BINARY: 50 Bit, VD_SI: 10^15 --> exa
				retVal *= (valueDomain == VD_SI) ? 1000000000000000LLU : 1125899906842624LLU;
				break;
#endif
			default:
				app_error("util::strToSize() : Invalid unit <" + unit + ">");
				break;
			}
	}
	return retVal;
}


const std::string siUnits[]     = { "Byte", "kB",   "MB",   "GB",   "TB",   "PB",   "EB",   "ZB",   "YB"   };
const std::string binaryUnits[] = { "Byte", "kiB",  "MiB",  "GiB",  "TiB",  "PiB",  "EiB",  "ZiB",  "YiB"  };
const std::string bitUnits[]    = { "Bit",  "kBit", "MBit", "GBit", "TBit", "PBit", "EBit", "ZBit", "YBit" };

// Attention: printf and co. depend on the system locale!
std::string util::sizeToStr(size_t size, unsigned int precision, EValueDomain valueDomain, const app::TLocale& locale) {
    int i = 0;
    long double s = size;
    long double d;
    if (valueDomain == VD_SI)
    	d = 1000.0;
    else
    	d = 1024.0;
    while (s > d && i < 9) {
        s /= d;
        i++;
    }

    std::string fmt;
    if (i > 0 && precision > 0)
    	fmt = "%." + std::to_string((size_u)precision) + "Lf %s";
    else
    	fmt = "%Lg %s";

    // Check for using alternate locale settings
    if (locale != syslocale) {
		switch (valueDomain) {
			case VD_SI:
				return util::cprintf(locale, fmt, s, siUnits[i].c_str());
				break;
			case VD_BINARY:
				return util::cprintf(locale, fmt, s, binaryUnits[i].c_str());
				break;
			case VD_BIT:
				return util::cprintf(locale, fmt, s, bitUnits[i].c_str());
				break;
		}
    } else {
		switch (valueDomain) {
			case VD_SI:
				return util::cprintf(fmt, s, siUnits[i].c_str());
				break;
			case VD_BINARY:
				return util::cprintf(fmt, s, binaryUnits[i].c_str());
				break;
			case VD_BIT:
				return util::cprintf(fmt, s, bitUnits[i].c_str());
				break;
		}
    }

    return "[Invalid]";
}
