/*
 * stringutil.h
 *
 *  Created on: 30.05.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include "templates.h"
#include "localizations.h"
#include "hash.h"
#include "classes.h"
#include "functors.h"
#include "localization.h"
#include "stringtypes.h"
#include "stringtemplates.h"

namespace util {

enum ECompareType {
	EC_COMPARE_FULL,
	EC_COMPARE_PARTIAL,
	EC_COMPARE_HEADING,
	EC_COMPARE_VALUE_IN_LIST,
	EC_COMPARE_LIST_IN_VALUE
};

enum EExportType {
	ET_EXPORT_PLAIN,
	ET_EXPORT_TRIMMED,
	ET_EXPORT_HTML
};

enum EImportType {
	ET_IMPORT_PLAIN,
	ET_IMPORT_QUOTED
};

std::string  strToStr(const std::string& str, const char *defValue = "<empty>");
std::wstring strToStr(const std::wstring& str, const wchar_t *defValue = L"<empty>");
const std::string&  strToStr(const std::string& str, const std::string& defValue);
const std::wstring& strToStr(const std::wstring& str, const std::wstring& defValue);

std::string  charToStr(const char *str, const char *defValue = "");
std::wstring charToStr(const wchar_t *str, const wchar_t *defValue = L"");
std::string  charToStr(const char *str, const std::string& defValue);
std::wstring charToStr(const wchar_t *str, const std::wstring& defValue);

const char * charToChar(const char *str, const char *defValue);
const wchar_t * charToChar(const wchar_t *str, const wchar_t *defValue);

void fill(std::string& str, const char fillchar, const size_t size);
void fill(std::wstring& str, const wchar_t fillchar, const size_t size);

std::string  ellipsis(const std::string& str, const size_t size = 55);
std::wstring ellipsis(const std::wstring& str, const size_t size = 55);
std::string  shorten(const std::string& str, const size_t size);
std::wstring shorten(const std::wstring& str, const size_t size);

std::string& trimLeft(std::string& str, TIsWhiteSpaceA isSpace = isWhiteSpaceA);
std::string& trimRight(std::string& str, TIsWhiteSpaceA isSpace = isWhiteSpaceA);
std::string& trim(std::string& str, TIsWhiteSpaceA isSpace = isWhiteSpaceA);
std::string  trim(const std::string&, TIsWhiteSpaceA isSpace = isWhiteSpaceA);

std::wstring& trimLeft(std::wstring& str, TIsWhiteSpaceW isSpace = isWhiteSpaceW);
std::wstring& trimRight(std::wstring& str, TIsWhiteSpaceW isSpace = isWhiteSpaceW);
std::wstring& trim(std::wstring& str, TIsWhiteSpaceW isSpace = isWhiteSpaceW);
std::wstring  trim(const std::wstring&, TIsWhiteSpaceW isSpace = isWhiteSpaceW);

int replacer(std::string& str, char oldChar, char newChar, const size_t startPos = 0);
int replacer(std::wstring& str, wchar_t oldChar, wchar_t newChar, const size_t startPos = 0);
int replacer(std::string& str, const std::string& oldStr, const std::string& newStr, const size_t startPos = 0);
int replacer(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr, const size_t startPos = 0);

std::string  replace(const std::string& str, char oldChar, char newChar, const size_t startPos = 0);
std::wstring replace(const std::wstring& str, wchar_t oldChar, wchar_t newChar, const size_t startPos = 0);
std::string  replace(const std::string& str, const std::string& oldStr, const std::string& newStr, const size_t startPos = 0);
std::wstring replace(const std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr, const size_t startPos = 0);

std::string  quote(const std::string &str);
std::wstring quote(const std::wstring &str);
std::string  unquote(const std::string &str);
std::wstring unquote(const std::wstring &str);

std::string  escape(const std::string& value);
std::wstring escape(const std::wstring& value);
std::string  unescape(const std::string& value);
std::wstring unescape(const std::wstring& value);

std::string  capitalize(const std::string &str, const std::locale& locale = std::locale());
std::wstring capitalize(const std::wstring &str, const std::locale& locale = std::locale());
std::string  tolower(const std::string &str, const std::locale& locale = std::locale());
std::wstring tolower(const std::wstring &str, const std::locale& locale = std::locale());
std::string  toupper(const std::string &str, const std::locale& locale = std::locale());
std::wstring toupper(const std::wstring &str, const std::locale& locale = std::locale());

void split(const std::string& str, app::TStringVector& list);
void split(const std::string& str, app::TStringVector& list, const char delimiter);
void tokenize(const std::string& str, app::TStringVector& list, const char* delimiters, const EImportType type = ET_IMPORT_QUOTED, TIsWhiteSpaceA isSpace = isWhiteSpaceA);

std::string extractQuotedStr(const std::string& str, std::string::const_iterator& pos, const char quote = '"');
std::string extractQuotedStr(const std::string& str, size_t& pos, const char quote = '"');
std::string extractQuotedStr(const std::string& str, const char quote, TIsWhiteSpaceA isSpace = isWhiteSpaceA);

void join(const std::vector<std::string>& list, std::string& s, const char delimiter = ';');

std::string  cprintf(const char* fmt, ...);
std::string  cprintf(const std::string &fmt, ...);
std::wstring cprintf(const wchar_t* fmt, ...);
std::wstring cprintf(const std::wstring &fmt, ...);
std::string  cprintf(const app::TLocale& locale, const char* fmt, ...);
std::string  cprintf(const app::TLocale& locale, const std::string &fmt, ...);
std::wstring cprintf(const app::TLocale& locale, const wchar_t* fmt, ...);
std::wstring cprintf(const app::TLocale& locale, const std::wstring &fmt, ...);

void loadFromFile(app::TStringVector& list, const std::string& fileName, const app::ECodepage codepage = app::ECodepage::CP_DEFAULT);
void saveToFile(const app::TStringVector& list, const std::string& fileName);
void debugOutput(const app::TStringVector& list);


class TStringList : public app::TStringVector, public app::TObject {
private:
	mutable std::string strText;
	mutable std::string strRaw;
	mutable std::string strCSV;
	mutable std::string strTSV;
	mutable std::string strHTML;
	mutable std::string strTable;

public:
	typedef app::TStringVector::iterator iterator;
	typedef app::TStringVector::const_iterator const_iterator;

	void clear();
	void invalidate();

	void add(const std::string& value);
	void add(const util::TStringList& list);
	void add(const char* data, const size_t size);
	void push_front(const std::string& value);
	bool remove(const std::string& value);

	void assign(const std::string& csv);
	void assign(const std::string& csv, const char delimiter);
	void assign(const std::string& csv, const char* delimiters);
	void assign(const std::string& csv, const std::string& delimiters);
	void assign(const app::TStringVector& vector);
	void assign(const util::TStringList& list);

	size_t readText(void const * const data, size_t const size);
	size_t readBinary(void const * const data, size_t const size);

	std::string asString(const char delimiter = ' ',  const EExportType type = ET_EXPORT_PLAIN) const;
	const std::string& csv(const char delimiter = ';') const;
	const std::string& tsv(const char delimiter = VT) const;
	const std::string& text(const char delimiter = ' ') const;
	const std::string& raw(const char delimiter = ' ') const;
	const std::string& html() const;
	const std::string& table(const size_t emphasize = std::string::npos) const;
	size_t length() const;

	void saveToFile(const std::string fileName) const;
	void loadFromFile(const std::string fileName, const app::ECodepage codepage = app::ECodepage::CP_DEFAULT);
	void select(const size_t index, const size_t count, util::TStringList& target);
	void debugOutput() const;

	void split(const std::string& csv, const char delimiter);
	void split(const std::string& csv, const char* delimiters, TIsWhiteSpaceA isSpace = isWhiteSpaceA);
	void split(const std::string& csv, const std::string& delimiters, TIsWhiteSpaceA isSpace = isWhiteSpaceA);

	bool validIndex(size_t index) const;
	size_t find(const std::string& value, const util::ECompareType partial = EC_COMPARE_FULL) const;

	size_t compress();
	void shrink(size_t rows);
	void distinct(const util::ECompareType partial = EC_COMPARE_FULL);
	void sort(util::ESortOrder order, TStringSorter asc, TStringSorter desc);
	void sort(util::ESortOrder order = SO_ASC);
	void sort(TStringSorter sorter);

	inline const_iterator first() const { return begin(); };
	inline const_iterator last() const { return util::pred(end()); };

	TStringList& operator = (const app::TStringVector& vector);
	TStringList& operator = (const TStringList& list);
	TStringList& operator = (const std::string& csv);
	operator bool () const { return !empty(); };

	TStringList();
	TStringList(const TStringList& list);
	TStringList(const app::TStringVector& vector);
	TStringList(const std::string& csv, const char delimiter);
	virtual ~TStringList();
};

template<typename T>
class TObjectList : public app::TObjectVector<T>, public app::TObject {
private:
	bool owns;

#ifdef STL_HAS_TEMPLATE_ALIAS
	using list_t = app::TObjectVector<T>;
	using object_t = app::TObjectItem<T>;
#else
	typedef typename app::TObjectVector<T>::type_t list_t;
	typedef typename app::TObjectItem<T>::type_t object_t;
#endif

public:

#ifdef STL_HAS_TEMPLATE_ALIAS
	using iterator = typename list_t::iterator;
	using const_iterator = typename list_t::const_iterator;
#else
	typedef typename list_t::iterator iterator;
	typedef typename list_t::const_iterator const_iterator;
#endif

	void add(const std::string& key, T* value) {
		push_back(object_t(key, value));
	}

	void clear() {
		if (owns) {
			for (auto it : this) {
				if (util::assigned(it.second))
					util::freeAndNil(it.second);
			}
		}
		list_t::clear();
	}

	operator bool () const { return !list_t::empty(); };

	TObjectList() { owns = false; };
	TObjectList(const bool owns) : owns(owns) {};
	virtual ~TObjectList() { clear(); };
};

} /* namespace util */

#endif /* STRINGUTIL_H_ */
