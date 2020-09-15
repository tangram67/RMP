/*
 * locale.cpp
 *
 *  Created on: 24.10.2015
 *      Author: Dirk Brinkmeier
 */

#include <cstring>
#include <iostream>
#include "gcc.h"
#include "locale.h"
#include "templates.h"
#include "exception.h"
#include "stringutils.h"
#include "../config.h"

#include <assert.h>

static bool localesInitialized = false;

#ifdef STL_HAS_LAMBDAS

static const char* initLocale __attribute__((unused)) = []{
	const char* retVal = nil;

	// Allow access to all locales
	if (!localesInitialized) {
		retVal = setlocale(LC_ALL, "");
		localesInitialized = true;
	}

	return retVal;
} (); // () invokes lambda

#else

const char* initLocales() {
	const char* retVal;

	// Allow access to all locales
	if (!localesInitialized) {
		retVal = setlocale(LC_ALL, "");
		localesInitialized = true;
	}
	return retVal;
}

static const char* initLocale __attribute__((unused)) = initLocales();

#endif

namespace app {


TLocale::TLocale() {
	locale = ELocale::sysloc;
	sysloc = create(locale);
}

TLocale::TLocale(const ELocale locale) : locale(locale) {
	sysloc = create(locale);
}

TLocale::~TLocale() {
	destroy();
}


void TLocale::clear() {
	locale = ELocale::nloc;
	zone = ERegion::nreg;
	iso.clear();
	base.clear();
	name.clear();
	region.clear();
	mainland.clear();
	codeset.clear();
	country.clear();
	language.clear();
	description.clear();
	datetimeA = DEFAULT_LOCALE_TIME_FORMAT_A;
	datetimeW = DEFAULT_LOCALE_TIME_FORMAT_W;
	booleanTrueNameA = BOOLEAN_TRUE_NAME_A;
	booleanFalseNameA = BOOLEAN_FALSE_NAME_A;
	booleanTrueNameW = BOOLEAN_TRUE_NAME_W;
	booleanFalseNameW = BOOLEAN_FALSE_NAME_W;
}


void TLocale::assign(const TLocale &value) {
	destroy();
	sysloc = duplocale(value());
	locale = value.locale;
	zone = value.zone;
	iso = value.iso;
	base = value.base;
	name = value.name;
	region = value.region;
	mainland = value.mainland;
	country = value.country;
	codeset = value.codeset;
	language = value.language;
	description = value.description;
	datetimeA = value.datetimeA;
	datetimeW = value.datetimeW;
	booleanTrueNameA = value.booleanTrueNameA;
	booleanFalseNameA = value.booleanFalseNameA;
	booleanTrueNameW = value.booleanTrueNameW;
	booleanFalseNameW = value.booleanFalseNameW;
}


bool TLocale::setProperties(const std::string& code, const std::string& charset) {
	if (!code.empty()) {
		TLanguage c;
		if (find(code.c_str(), c)) {
			size_t fp;
			base    = c.name;
			locale  = c.locale;
			region  = c.region;
			codeset = charset.empty() ? c.codeset : charset;
			country = c.country;
			language = c.language;
			description = c.description;
			datetimeA = c.datetimeA;
			datetimeW = c.datetimeW;
			booleanTrueNameA = c.booleanTrueNameA;
			booleanFalseNameA = c.booleanFalseNameA;
			booleanTrueNameW = c.booleanTrueNameW;
			booleanFalseNameW = c.booleanFalseNameW;
			iso = util::replace(base, '_', '-');
			switch (locale) {
				case ELocale::cloc:
				case ELocale::ploc:
					name = c.name;
					break;
				default:
					fp = code.find('.');
					if (std::string::npos != fp) {
						name = util::csnprintf("%.%", code.substr(0, fp), codeset);
					} else {
						name = util::csnprintf("%.%", code, codeset);
					}
					break;
			}
			TRegion region;
			if (area(c.region, region)) {
				zone = region.region;
				mainland = region.mainland;
			}
		}
	}
	return !name.empty();
}


locale_t TLocale::create(const ELocale locale) {
	locale_t retVal = nil;
	bool useLocale = false;
	std::string code;

	// First destroy existing object
	destroy();
	clear();

	// Create new locale
	switch (locale) {
		case ELocale::cloc:
			code = "C";
			break;
		case ELocale::ploc:
			code = "POSIX";
			break;
		case ELocale::siloc:
			// Query current system locale
			code = getSystemLocale();
			break;
		case ELocale::nloc:
		case ELocale::sysloc:
			code = getSystemLocale();
			useLocale = true;
			break;
		default:
			code = find(locale);
			useLocale = true;
			break;
	}

	// Create libc locale
	if (setProperties(code)) {
		errno = EXIT_SUCCESS;
		retVal = newlocale(LC_ALL, name.c_str(), (locale_t)0);
	}

	// Try UTF-8 charset for given locale code
	if (!util::assigned(retVal)) {
		if (setProperties(code, "UTF-8")) {
			errno = EXIT_SUCCESS;
			retVal = newlocale(LC_ALL, name.c_str(), (locale_t)0);
		}
	}

	// Setting locale failed
	if (!util::assigned(retVal)) {
		std::string s = getInfo();
		clear();
		throw util::sys_error("TLocale::create() failed for <" + s + ">");
	}

	// No standard locale, use given locale instead
	if (!useLocale) {
		this->locale = locale;
	}

	return retVal;
}


void TLocale::destroy() {
	if (util::assigned(sysloc)) {
		freelocale(sysloc);
		sysloc = nil;
	}
}

bool TLocale::isValidLocale(const ELocale locale) {
	return locale != ELocale::nloc /*&& locale != ELocale::siloc && locale != ELocale::sysloc && locale != ELocale::syspos*/ ;
}

std::string TLocale::getInfo() const {
	return "[" + getName() + "/" + getBase() + "/" + getRegion() + "]";
}

std::string TLocale::getLocation() const {
	return "[" + getLanguage() + "/" + getCountry() + "/" + getMainland() + "]";
}

std::string TLocale::getSystemLocale() {
	// Query current system locale
	const char* p = setlocale(LC_ALL, nil);
	if (util::assigned(p)) {
		size_t len = strnlen(p, 100);
		if (len > 0) {
			return std::string(p, len);
		}
	}
	return "";
}

bool TLocale::isSystemLocale() const {
	std::string c = getSystemLocale();
	return (c == name);
}


bool TLocale::find(const std::string& name, TLanguage& language) {
	if (!name.empty())
		return find(name.c_str(), language);
	return false;
}


bool TLocale::find(const char* name, TLanguage& language) {
	const struct TLanguage *it;
	for (it = locales; util::assigned(it->name); ++it) {
		if (util::assigned(strstr(name, it->name))) {
			language = *it;
			return true;
		}
	}
	return false;
}


bool TLocale::find(const ELocale locale, TLanguage& language) {
	const struct TLanguage *it;
	for (it = locales; util::assigned(it->name); ++it) {
		if (it->locale == locale) {
			language = *it;
			return true;
		}
	}
	return false;
}


ELocale TLocale::find(const std::string& name) {
	if (!name.empty())
		return find(name.c_str());
	return ELocale::nloc;
}

ELocale TLocale::find(const char* name) {
	const struct TLanguage *it;
	for (it = locales; util::assigned(it->name); ++it) {
		if (util::assigned(strstr(name, it->name))) {
			return it->locale;
		}
	}
	return ELocale::nloc;
}

std::string TLocale::find(const ELocale locale) {
	const struct TLanguage *it;
	for (it = locales; util::assigned(it->name); ++it) {
		if (it->locale == locale) {
			if (util::assigned(it->name))
				return it->name;
		}
	}
	return "";
}

std::string TLocale::query(const nl_item item) {
	std::string retVal;
	const char* p = nl_langinfo(item);
	if (util::assigned(p)) {
		size_t len = strnlen(p, 100);
		if (len > 0)
			retVal.assign(p, len);
	}
	return retVal;
}

void TLocale::modify(const int mask, const std::string& value) {
	if (util::assigned(sysloc) && !value.empty()) {
		locale_t loc = newlocale(mask, value.c_str(), sysloc);
		if (util::assigned(loc)) {
			sysloc = loc;
		} else
			throw util::sys_error_fmt("TLocale::modify() failed for value $", value);
	}
}

void TLocale::use() const {
	// Set applications locale
	if (util::assigned(sysloc)) {
		// Set glibc standard C locale
		const char* p = setlocale(LC_ALL, name.c_str());
		if (util::assigned(p)) {

			// Check C system locale
			std::string c = getSystemLocale();
			if (name != c)
				throw util::app_error_fmt("TLocale::useLocale()::setlocale() failed: Wrong locale [%] after setlocale(%)", c, name);

			// Set global C++ local
			imbue(name);

		} else
			throw util::sys_error_fmt("TLocale::setLocale(%) failed.", name);
	}
}

void TLocale::imbue(const std::string& name) const {
	// Set global C++ local
	std::locale cloc(name.c_str());
	std::locale apploc(cloc, new noGroupBoolNameFacet<char>(cloc, getBooleanTrueName(), getBooleanFalseName()));
	std::locale::global(apploc);
	std::cout.imbue(apploc);
	std::wcout.imbue(apploc);

#ifdef USE_BOOLALPHA
	// Switch cout to alphanumeric boolean values
	std::cout << std::boolalpha;
	std::wcout << std::boolalpha;
#endif

	// Check C++ system locale before applying facet
	// Locale name with facet applied is set to '*' for some reason...
	if (name != cloc.name())
		throw util::app_error("TLocale::useLocale()::locale() failed: Wrong locale [" + cloc.name() + "] after std::locale(" + name + ")" );
}

locale_t TLocale::change(locale_t locale) {
	locale_t retVal = uselocale(locale);
	if (retVal == (locale_t)0)
		throw util::sys_error("TLocale::switchLocale()::uselocale() failed.");
	return retVal;
}

void TLocale::restore(locale_t locale) {
	if (locale == (locale_t)0)
		throw util::sys_error("TLocale::restoreLocale() failed, locale not set.");
	locale_t r = uselocale(locale);
	if (r == (locale_t)0)
		throw util::sys_error("TLocale::restoreLocale()::uselocale() failed.");
}

void TLocale::set(const ELocale locale) {
	if (isValidLocale(locale))
		sysloc = create(locale);
}


TLocale& TLocale::operator = (const TLocale &value) {
	assign(value);
	return *this;
}


ERegion TLocale::area(const std::string& name) {
	return area(name.c_str());
}

ERegion TLocale::area(const char* name) {
	TRegion region;
	if (area(name, region))
		return region.region;
	return ERegion::nreg;
}

bool TLocale::area(const std::string& name, TLanguage& region) {
	return area(name.c_str(), region);
}

bool TLocale::area(const char* name, TRegion& region) {
	const struct TRegion *it;
	for (it = regions; util::assigned(it->name); ++it) {
		if (util::assigned(strstr(it->name, name))) {
			region = *it;
			return true;
		}
	}
	return false;
}


} /* namespace app */
