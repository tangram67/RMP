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

TLocale::TLocale(const ELocaleType type) : type(type) {
	locale = ELocale::sysloc;
	sysloc = create(locale);
}

TLocale::TLocale(const ELocale locale, const ELocaleType type) : locale(locale), type(type) {
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
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
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

bool TLocale::duplicate(const TLocale &value, locale_t& locale) const {
	// Do not duplicate constant locales
	// --> Not needed, since they are NOT INTENDED to be changed!
	if (type != ELT_CONSTANT) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
		locale = duplocale(sysloc);
	}
	return (locale_t)0 != locale;
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
	locale_t r = (locale_t)0;
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

	// Create libC based locale
	if (setProperties(code)) {
		errno = EXIT_SUCCESS;
		r = newlocale(LC_ALL_MASK, name.c_str(), (locale_t)0);
	}

	// Try UTF-8 charset for given locale code
	if (!util::assigned(r)) {
		if (setProperties(code, "UTF-8")) {
			errno = EXIT_SUCCESS;
			r = newlocale(LC_ALL_MASK, name.c_str(), (locale_t)0);
		}
	}

	// Setting locale failed
	if (!util::assigned(r)) {
		std::string s = getInfo();
		clear();
		throw util::sys_error("TLocale::create() failed for <" + s + ">");
	}

	// No standard locale, use given locale instead
	if (!useLocale) {
		this->locale = locale;
	}

	return r;
}

void TLocale::destroy() {
	if (util::assigned(sysloc)) {
		freelocale(sysloc);
		sysloc = nil;
	}
}

bool TLocale::isValidLocale(const ELocale locale) {
	return ELocale::nloc != locale /*&& locale != ELocale::siloc && locale != ELocale::sysloc && locale != ELocale::syspos*/ ;
}

const char* TLocale::getBooleanTrueNameWithNolock() const {
	return util::assigned(booleanTrueNameA) ? booleanTrueNameA : BOOLEAN_TRUE_NAME_A;
};
const char* TLocale::getBooleanFalseNameWithNolock() const {
	return util::assigned(booleanFalseNameA) ? booleanFalseNameA : BOOLEAN_FALSE_NAME_A;
};

std::string TLocale::getInfo() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return "[" + name + "/" + base + "/" + region + "]";
}
std::string TLocale::getLocation() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return "[" + language + "/" + country + "/" + mainland + "]";
}
const std::string& TLocale::getBase() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return base;
};
const std::string& TLocale::getName() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return name;
};
const std::string& TLocale::getRegion() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return region;
};
const std::string& TLocale::getMainland() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return mainland;
};
const std::string& TLocale::getCodeset() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return codeset;
};
const std::string& TLocale::getCountry() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return country;
};
const std::string& TLocale::getLanguage() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return language;
};
const std::string& TLocale::getDescription() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return description;
};
const std::string& TLocale::asISO639() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return iso;
};
const char* TLocale::getBooleanTrueName() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return getBooleanTrueNameWithNolock();
};
const char* TLocale::getBooleanFalseName() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return getBooleanFalseNameWithNolock();
};
const wchar_t* TLocale::getBooleanTrueNameW() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return util::assigned(booleanTrueNameW) ? booleanTrueNameW : BOOLEAN_TRUE_NAME_W;
};
const wchar_t* TLocale::getBooleanFalseNameW() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return util::assigned(booleanFalseNameW) ? booleanFalseNameW : BOOLEAN_FALSE_NAME_W;
};
const char* TLocale::getTimeFormat() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return util::assigned(datetimeA) ? datetimeA : DEFAULT_LOCALE_TIME_FORMAT_A;
};
const wchar_t* TLocale::getWideTimeFormat() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return util::assigned(datetimeW) ? datetimeW : DEFAULT_LOCALE_TIME_FORMAT_W;
};
ELocale TLocale::getLocale() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return locale;
};
ERegion TLocale::getZone() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return zone;
};

locale_t TLocale::getNativeLocale() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return sysloc;
};

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
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
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
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
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
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
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
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
	if (util::assigned(sysloc)) {
		// Set glibc standard C locale
		const char* p = setlocale(LC_ALL, name.c_str());
		if (util::assigned(p)) {

			// Check C system locale
			std::string c = getSystemLocale();
			if (name != c)
				throw util::app_error_fmt("TLocale::use()::getSystemLocale() failed: Wrong locale [%] after setlocale(%)", c, name);

			// Set global C++ local
			imbue(name);

		} else
			throw util::sys_error_fmt("TLocale::use()::setLocale(%) failed.", name);
	}
}

void TLocale::imbue(const std::string& name) const {
	// Set global C++ local
	std::locale cloc(name.c_str());
	std::locale apploc(cloc, new noGroupBoolNameFacet<char>(cloc, getBooleanTrueNameWithNolock(), getBooleanFalseNameWithNolock()));
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
		throw util::app_error("TLocale::imbue() failed: Wrong locale [" + cloc.name() + "] after std::locale(" + name + ")" );
}

locale_t TLocale::change(locale_t locale) {
	if (locale == (locale_t)0)
		throw util::sys_error("TLocale::change() failed, locale not set.");
	locale_t r = uselocale(locale);
	if (r == (locale_t)0)
		throw util::sys_error("TLocale::change()::uselocale() failed.");
	return r;
}

void TLocale::restore(locale_t locale) {
	if (locale == (locale_t)0)
		throw util::sys_error("TLocale::restore() failed, locale not set.");
	locale_t r = uselocale(locale);
	if (r == (locale_t)0)
		throw util::sys_error("TLocale::restore()::uselocale() failed.");
}

bool TLocale::set(const ELocale locale) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
	locale_t r = (locale_t)0;
	if (isValidLocale(locale)) {
		r = create(locale);
		if ((locale_t)0 != r) {
			sysloc = r;
			return true;
		}
	}
	return false;
}


TLocale& TLocale::operator = (const TLocale &value) {
	assign(value);
	return *this;
}

bool TLocale::operator == (const TLocale &value) const {
	// app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return (locale == value.locale);
};
bool TLocale::operator != (const TLocale &value) const {
	// app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return (locale != value.locale);
};

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
