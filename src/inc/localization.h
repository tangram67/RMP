/*
 * locale.h
 *
 *  Created on: 24.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef LOCALIZATION_H_
#define LOCALIZATION_H_


#include <locale>
#include <string>
#include <locale.h>
#include <langinfo.h>
#include "templates.h"
#include "../config.h"
#include "localizations.h"
#include "gcc.h"

// General information:
// http://www.stroustrup.com/3rd_loc.pdf

namespace app {

class TLocale;

#ifdef STL_HAS_TEMPLATE_ALIAS
using PLocale = TLocale*;
#else
typedef TLocale* PLocale;
#endif


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
		locale(locale), facet(std::use_facet<std::numpunct<char_t> >(locale)) {};
};


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
		locale(locale), facet(std::use_facet<std::numpunct<char_t> >(locale)) {
		this->yes = util::assigned(yes) ? yes : facet.truename();
		this->no = util::assigned(no) ? no : facet.truename();
	};
};


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

class TLocale {
private:
	locale_t sysloc;
	ELocale locale;
	ERegion zone;

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

	bool setProperties(const std::string& code, const std::string& charset = "");
	locale_t create(const ELocale locale);
	void destroy();
	void clear();

public:
	std::string getInfo() const;
	std::string getLocation() const;
	const std::string& getBase() const { return base; };
	const std::string& getName() const { return name; };
	const std::string& getRegion() const { return region; };
	const std::string& getMainland() const { return mainland; };
	const std::string& getCodeset() const { return codeset; };
	const std::string& getCountry() const { return country; };
	const std::string& getLanguage() const { return language; };
	const std::string& getDescription() const { return description; };
	const std::string& asISO639() const { return iso; };
	const char* getBooleanTrueName() const { return util::assigned(booleanTrueNameA) ? booleanTrueNameA : BOOLEAN_TRUE_NAME_A; };
	const char* getBooleanFalseName() const { return util::assigned(booleanFalseNameA) ? booleanFalseNameA : BOOLEAN_FALSE_NAME_A; };
	const wchar_t* getBooleanTrueNameW() const { return util::assigned(booleanTrueNameW) ? booleanTrueNameW : BOOLEAN_TRUE_NAME_W; };
	const wchar_t* getBooleanFalseNameW() const { return util::assigned(booleanFalseNameW) ? booleanFalseNameW : BOOLEAN_FALSE_NAME_W; };
	const char* getTimeFormat() const { return util::assigned(datetimeA) ? datetimeA : DEFAULT_LOCALE_TIME_FORMAT_A; };
	const wchar_t* getWideTimeFormat() const { return util::assigned(datetimeW) ? datetimeW : DEFAULT_LOCALE_TIME_FORMAT_W; };
	ELocale getLocale() const { return locale; };
	ERegion getZone() const { return zone; };
	locale_t getObject() const { return sysloc; };
	locale_t operator () () const { return getObject(); };
	TLocale& operator = (const TLocale &value);
	bool operator == (const TLocale &value) const { return (locale == value.locale); };
	bool operator != (const TLocale &value) const { return (locale != value.locale); };
	void assign(const TLocale &value);
	bool isSystemLocale() const;

	static locale_t change(locale_t locale);
	static void restore(locale_t locale);

	void imbue(const std::string& name) const;

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

	void set(const ELocale locale);
	void use() const;

	TLocale();
	TLocale(const ELocale locale);
	virtual ~TLocale();
};



template<typename T>
class TLocationGuard
{
private:
	typedef T location_t;
	const location_t& newloc;
#ifdef IMBUE_LOCATION_GUARD
	std::string syslocale;
#endif
	locale_t locale;

public:
	void change() {
		if (!util::assigned(locale)) {
			if (!newloc.isSystemLocale()) {
				locale = location_t::change(newloc());
#ifdef IMBUE_LOCATION_GUARD
				TLocale::imbue(newloc.getName());
#endif
			}
		}
	}
	void restore() {
		if (util::assigned(locale)) {
			location_t::restore(locale);
#ifdef IMBUE_LOCATION_GUARD
			TLocale::imbue(syslocale);
#endif
			locale = nil;
		}
	}

	TLocationGuard& operator=(const TLocationGuard&) = delete;
	TLocationGuard(const TLocationGuard&) = delete;
	explicit TLocationGuard(const location_t& newloc)
		: newloc(newloc), locale(nil) {
#ifdef IMBUE_LOCATION_GUARD
		syslocale = TLocale::getSystemLocale();
#endif
	}
	~TLocationGuard() { restore(); }
};


static app::TLocale en_US (app::ELocale::en_US);
static app::TLocale en_GB (app::ELocale::en_GB);
static app::TLocale fr_FR (app::ELocale::fr_FR);
static app::TLocale de_DE (app::ELocale::de_DE);
static app::TLocale it_IT (app::ELocale::it_IT);
static app::TLocale es_ES (app::ELocale::es_ES);
static app::TLocale si_SI (app::ELocale::siloc);
static app::TLocale cc_CC (app::ELocale::cloc);

} /* namespace app */

#endif /* LOCALIZATION_H_ */
