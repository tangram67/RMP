/*
 * locale.h
 *
 *  Created on: 24.10.2015
 *      Author: Dirk Brinkmeier
 *
 *  General information:
 *  http://www.stroustrup.com/3rd_loc.pdf
 *
 */

#ifndef LOCALIZATION_H_
#define LOCALIZATION_H_

#include <locale>
#include <string>
#include <locale.h>
#include <langinfo.h>
#include "templates.h"
#include "exception.h"
#include "semaphores.h"
#include "localizations.h"
#include "../config.h"
#include "gcc.h"

namespace app {

class TLocale;

#ifdef STL_HAS_TEMPLATE_ALIAS
using PLocale = TLocale*;
#else
typedef TLocale* PLocale;
#endif

enum ELocaleType { ELT_CONSTANT, ELT_VARIABLE, ELT_SYSTEM };

//
// See http://www.math.hkbu.edu.hk/parallel/pgi/doc/pgC++_lib/stdlibug/fac_3537.htm
//
// Usage:
// std::locale cloc(syslocale.getName().c_str());
// std::locale apploc(cloc, new boolNameFacet<char>(cloc, "Ja", "Nein"));
// std::cout.imbue(apploc);
//
template <typename T>
class boolNameFacet : public std::numpunct_byname<T> {
private:
	std::string yes, no;
	typedef T char_t;
	typedef typename std::numpunct_byname<char_t>::string_type string_type;
	
protected:
	string_type do_truename()  const { return yes.c_str(); }
	string_type do_falsename() const { return no.c_str(); }
	
public:
	explicit boolNameFacet(const std::locale& locale, const std::string& yes, const std::string& no) :
		std::numpunct_byname<char_t>(locale.name().c_str()), yes(yes), no(no) {}
};


//
// See http://www.technologische-hilfe.de/antworten/nochmal-strings-und-locale-numfacet-support-242389772.html
//
// Usage:
// See void TLocale::imbue(const std::string& name)
//
template <typename T>
class noGroupFacet : public std::numpunct<T> {
private:
	typedef T char_t;
	typedef typename std::numpunct<char_t>::string_type string_type;

	std::locale locale;
	std::numpunct<char_t> const& facet;

protected:
	char_t do_decimal_point() const { return facet.decimal_point(); }
	char_t do_thousands_sep() const { return facet.thousands_sep(); }
	string_type do_truename()  const { return facet.truename(); }
	string_type do_falsename() const { return facet.falsename(); }
	string_type do_grouping()  const { return NO_GROUPING_VALUE; }

public:
	explicit noGroupFacet(const std::locale& locale) :
		locale(locale), facet(std::use_facet<std::numpunct<char_t>>(locale)) {};
};

//
//template <typename T>
//class noGroupBoolNameFacet : public std::numpunct<T> {
//private:
//	typedef T char_t;
//	typedef typename std::numpunct<char_t>::string_type string_type;
//
//	std::locale locale;
//	std::string yes, no;
//	std::numpunct<char_t> const& facet;
//
//protected:
//	char_t do_decimal_point() const { return facet.decimal_point(); }
//	char_t do_thousands_sep() const { return facet.thousands_sep(); }
//	string_type do_truename()  const { return (yes.empty() ? facet.truename() : yes.c_str()); }
//	string_type do_falsename() const { return (no.empty() ? facet.falsename() : no.c_str()); }
//	string_type do_grouping()  const { return NO_GROUPING_VALUE; }
//
//public:
//	explicit noGroupBoolNameFacet(const std::locale& locale, const std::string& yes = "", const std::string& no = "") :
//		locale(locale), yes(yes), no(no), facet(std::use_facet<std::numpunct<char_t> >(locale)) {};
//};
//

template <typename T>
class noGroupBoolNameFacet : public std::numpunct<T> {
private:
	typedef T char_t;
	typedef typename std::numpunct<char_t>::string_type string_type;

	std::locale locale;
	string_type yes, no;
	std::numpunct<char_t> const& facet;

protected:
	char_t do_decimal_point() const { return facet.decimal_point(); }
	char_t do_thousands_sep() const { return facet.thousands_sep(); }
	string_type do_truename()  const { return yes; }
	string_type do_falsename() const { return no; }
	string_type do_grouping()  const { return NO_GROUPING_VALUE; }

public:
	explicit noGroupBoolNameFacet(const std::locale& locale, const char* yes = nil, const char* no = nil) :
		locale(locale), facet(std::use_facet<std::numpunct<char_t>>(locale)) {
		this->yes = util::assigned(yes) ? yes : facet.truename();
		this->no = util::assigned(no) ? no : facet.truename();
	};
};

//
// DEPRECATED: To be removed...
//
class TLocaleLock {
	pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

public:
	TLocaleLock() {};
	TLocaleLock(const TLocaleLock&) = delete;
	TLocaleLock& operator=(const TLocaleLock&) = delete;

	void wrLock() {
		int r = pthread_rwlock_wrlock(&rwlock);
		if (r == EDEADLK)
			throw util::sys_error("TLocaleLock::wrLock()::pthread_rwlock_wrlock() Ressource deadlocked.");
		if (util::checkFailed(r))
			throw util::sys_error("TLocaleLock::wrLock()::pthread_rwlock_wrlock() failed.");
	}

	bool wrTryLock() {
		int r = pthread_rwlock_trywrlock(&rwlock);
		if (r == EBUSY)
			return false;
		if (util::checkFailed(r))
			throw util::sys_error("TLocaleLock::wrTryLock()::pthread_rwlock_trywrlock() failed.");
		return true;
	}

	void rdLock() {
		int r;
		do
			r = pthread_rwlock_rdlock(&rwlock);
		while (r == EAGAIN);
		if (r == EDEADLK)
			throw util::sys_error("TLocaleLock::rdLock()::pthread_rwlock_rdlock() Ressource deadlocked.");
		if (util::checkFailed(r))
			throw util::sys_error("TLocaleLock::rdLock()::pthread_rwlock_rdlock() failed.");
	}

	bool rdTryLock() {
		int r = pthread_rwlock_tryrdlock(&rwlock);
		if (r == EBUSY || r == EAGAIN)
			return false;
		if (util::checkFailed(r))
			throw util::sys_error("TLocaleLock::rdTryLock()::pthread_rwlock_tryrdlock() failed.");
		return true;
	}

	void unlock() {
		int r = pthread_rwlock_unlock(&rwlock);
		if (util::checkFailed(r))
			throw util::sys_error("TLocaleLock::unlock()::pthread_rwlock_unlock() failed.");
	}
};


//
// Example for locale properties:
// locale      = ELocale::de_DE
// zone        = ERegion::de
// name        = "de_DE.UTF-8"
// base        = "de_DE"
// region      = "de"
// mainland    = "Western Europe"
// description = "German locale for Germany"
// language    = "German"
// country     = "Germany"
// codeset     = "UTF-8" 
//
class TLocale {
private:
	locale_t sysloc;
	ELocale locale;
	ERegion zone;
	ELocaleType type;
	mutable app::TReadWriteLock rwl;

	// Localization information
	std::string iso;
	std::string base;
	std::string name;
	std::string region;
	std::string mainland;
	std::string codeset;
	std::string country;
	std::string language;
	std::string description;

	// Localization constants
	const char* datetimeA;
	const wchar_t* datetimeW;
	const char* booleanTrueNameA;
	const char* booleanFalseNameA;
	const wchar_t* booleanTrueNameW;
	const wchar_t* booleanFalseNameW;

	void imbue(const std::string& name) const;
	bool setProperties(const std::string& code, const std::string& charset = "");
	locale_t create(const ELocale locale);
	void destroy();
	void clear();

	const char* getBooleanTrueNameWithNolock() const;
	const char* getBooleanFalseNameWithNolock() const;

public:
	STATIC_CONST locale_t nloc = (locale_t)0;

	ELocale getLocale() const;
	std::string getInfo() const;
	std::string getLocation() const;
	const std::string& getBase() const;
	const std::string& getName() const;
	const std::string& getRegion() const;
	const std::string& getMainland() const;
	const std::string& getCodeset() const;
	const std::string& getCountry() const;
	const std::string& getLanguage() const;
	const std::string& getDescription() const;
	const std::string& asISO639() const;
	const char* getBooleanTrueName() const;
	const char* getBooleanFalseName() const;
	const wchar_t* getBooleanTrueNameW() const;
	const wchar_t* getBooleanFalseNameW() const;
	const char* getTimeFormat() const;
	const wchar_t* getWideTimeFormat() const;
	ERegion getZone() const;

	TLocale& operator = (const TLocale &value);
	bool operator == (const TLocale &value) const;
	bool operator != (const TLocale &value) const;
	void assign(const TLocale &value);
	bool isSystemLocale() const;
	bool isFixed() const { return ELT_CONSTANT == type; };
	ELocaleType getType() const { return type; };

	static locale_t change(locale_t locale);
	static void restore(locale_t locale);

	locale_t getNativeLocale() const;
	locale_t operator () () const { return getNativeLocale(); };
	bool duplicate(const TLocale &value, locale_t& locale) const;
	app::TReadWriteLock& aquire() { return rwl; };

	static std::string getSystemLocale();
	static bool isValidLocale(const ELocale locale);

	static bool find(const std::string& name, TLanguage& language);
	static bool find(const char* name, TLanguage& language);
	static bool find(const ELocale locale, TLanguage& language);
	static ELocale find(const std::string& name);
	static ELocale find(const char* name);
	static std::string find(const ELocale locale);

	static bool area(const std::string& name, TLanguage& region);
	static bool area(const char* name, TRegion& region);
	static ERegion area(const std::string& name);
	static ERegion area(const char* name);

	std::string query(const nl_item item);
	void modify(const int mask, const std::string& value);

	bool set(const ELocale locale);
	void use() const;

	TLocale() = delete;
	TLocale(const ELocaleType type = ELT_VARIABLE);
	TLocale(const ELocale locale, const ELocaleType type = ELT_VARIABLE);
	virtual ~TLocale();
};


template<typename T>
class TLocaleGuard {
private:
	typedef T locale_p;
	locale_p locale;
	ELocaleType type;

	void destroy() {
		// Do not release constant locales
		// --> Not allowed since they are NOT duplicated by TLocale::duplicate() !!!
		if ((locale_p)0 != locale && type != ELT_CONSTANT) {
			freelocale(locale);
			locale = (locale_p)0;
		}
	}

public:
	TLocaleGuard& operator=(const TLocaleGuard&) = delete;
	TLocaleGuard(const TLocaleGuard&) = delete;
	explicit TLocaleGuard(const locale_p& locale, const ELocaleType type) : locale(locale), type(type) {}
	~TLocaleGuard() { destroy(); }
};


template<typename T>
class TLocationGuard {
private:
	typedef T location_t;
	const location_t& newloc;
#ifdef IMBUE_LOCATION_GUARD
	std::string syslocale;
#endif
	locale_t locale;

public:
	void change() {
		if (app::TLocale::nloc == locale) {
			if (!newloc.isSystemLocale()) {
				locale = location_t::change(newloc());
#ifdef IMBUE_LOCATION_GUARD
				TLocale::imbue(newloc.getName());
#endif
			}
		}
	}
	void restore() {
		if (app::TLocale::nloc != locale) {
			location_t::restore(locale);
#ifdef IMBUE_LOCATION_GUARD
			TLocale::imbue(syslocale);
#endif
			locale = app::TLocale::nloc;
		}
	}

	TLocationGuard& operator=(const TLocationGuard&) = delete;
	TLocationGuard(const TLocationGuard&) = delete;
	explicit TLocationGuard(const location_t& newloc)
		: newloc(newloc), locale(app::TLocale::nloc) {
#ifdef IMBUE_LOCATION_GUARD
		syslocale = TLocale::getSystemLocale();
#endif
	}
	~TLocationGuard() { restore(); }
};


static const app::TLocale en_US (app::ELocale::en_US, ELT_CONSTANT);
static const app::TLocale en_GB (app::ELocale::en_GB, ELT_CONSTANT);
static const app::TLocale fr_FR (app::ELocale::fr_FR, ELT_CONSTANT);
static const app::TLocale de_DE (app::ELocale::de_DE, ELT_CONSTANT);
static const app::TLocale it_IT (app::ELocale::it_IT, ELT_CONSTANT);
static const app::TLocale es_ES (app::ELocale::es_ES, ELT_CONSTANT);
static const app::TLocale pl_PL (app::ELocale::pl_PL, ELT_CONSTANT);
static const app::TLocale zh_CN (app::ELocale::zh_CN, ELT_CONSTANT);
static const app::TLocale si_SI (app::ELocale::siloc, ELT_CONSTANT);
static const app::TLocale cc_CC (app::ELocale::cloc,  ELT_CONSTANT);

} /* namespace app */

#endif /* LOCALIZATION_H_ */
