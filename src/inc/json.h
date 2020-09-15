/*
 * json.h
 *
 *  Created on: 28.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef JSON_H_
#define JSON_H_

#include <functional>
#include <string>
#include "gcc.h"
#include "ASCII.h"
#include "vartypes.h"
#include "stringutils.h"
#include "localizations.h"

namespace util {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TOnHeaderColumn = std::function<void(const std::string&, const util::EVariantType)>;
using TOnDataField = std::function<void(const std::string&, const std::string&, const size_t, const EVariantType type)>;

#else

typedef std::function<void(const std::string&, const util::EVariantType)> TOnHeaderColumn;
typedef std::function<void(const std::string&, const std::string&, const size_t, const EVariantType type)> TOnDataField;

#endif


// Max. size from http get for bootstrap-table
// See http://wenzhixin.net.cn/p/bootstrap-table/docs/
// e.g. https://localhost:8099/json/T_Personal.json?order=asc&limit=10&offset=0&_=1436290313274
STATIC_CONST int64_t BOOTSTRAP_TABLE_INDEX = 1436290313274;
static const std::string JSON_EMPTY_TABLE = "{ \"total\": 0, \"rows\": [] }";

static const std::string JSON_NULL_A = "null";
static const std::wstring JSON_NULL_W = L"null";
static const std::string JSON_NULL = JSON_NULL_A;

static const std::string JSON_TRUE_A = "true";
static const std::wstring JSON_TRUE_W = L"true";
static const std::string JSON_TRUE = JSON_TRUE_A;

static const std::string JSON_FALSE_A = "false";
static const std::wstring JSON_FALSE_W = L"false";
static const std::string JSON_FALSE = JSON_FALSE_A;


enum EJsonArrayType {
	EJT_PLAIN,
	EJT_ARRAY,
	EJT_OBJECT,
	EJT_DEFAULT = EJT_OBJECT
};

enum EJsonEntryType {
	EJE_LIST,
	EJE_LAST,
	EJE_DEFAULT = EJE_LIST
};

enum EJsonEscapeMode {
	EEM_PLAIN,
	EEM_ESCAPED,
	EEM_DEFAULT = EEM_PLAIN
};


class TJsonValue {
private:
	std::string line;
	std::string value;
	std::string key;
	EVariantType type;

	void clear();
	bool compare(const std::string& s1, const std::string& s2);

public:
	bool isValid() const { return !line.empty(); };
	void invalidate();

	std::string getKey() const { return key; };
	std::string getValue() const { return value; };
	EVariantType getType() const { return type; };
	void setKey(const std::string& key);
	void setValue(const std::string& value);
	void setType(const EVariantType type);

	void update(const std::string& key, const std::string& value, const EVariantType type);

	static std::string valueToStr(const std::string& preamble, const std::string& key, const std::string& value, const EVariantType type);
	static std::string boolToStr(const bool value);

	static std::string escape(const std::string& value);
	static std::string unescape(const std::string& value);

	std::string asJSON(const std::string& preamble = "");

	TJsonValue();
	virtual ~TJsonValue();
};


class TJsonList : public TStringList {
private:
	EJsonArrayType type;
	EJsonEscapeMode mode;
	bool opened;

	void prime();
	void push_back(const std::string& key, const std::string& value, EJsonEntryType type, const std::string& preamble);

public:
	void clear();

	void open(const EJsonArrayType type = EJT_DEFAULT);
	void close(const EJsonEntryType type = EJE_LAST);
	void close(const bool last);

	void openArray(const std::string name = "");
	void openObject(const std::string name = "");
	void closeArray(const EJsonEntryType type = EJE_LIST);
	void closeObject(const EJsonEntryType type = EJE_LIST);
	void closeArray(const bool last);
	void closeObject(const bool last);

	void add(const std::string& value);
	void add(const char* value);

	void add(const std::string& key, const std::string& value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const char* value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void add(const std::string& key, const bool value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const double value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void add(const std::string& key, const int8_t value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const uint8_t  value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void add(const std::string& key, const int16_t value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const uint16_t  value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void add(const std::string& key, const int32_t value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const uint32_t  value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void add(const std::string& key, const int64_t value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");
	void add(const std::string& key, const uint64_t  value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	void append(const std::string& key, const std::string& value, EJsonEntryType type = EJE_DEFAULT, const std::string& preamble = "  ");

	size_t size() const { return TStringList::size(); };
	bool empty() const { return TStringList::empty(); };
	const std::string& text() const;

	void debugOutput() const;
	void saveToFile(const std::string fileName) const;

	TJsonList();
	virtual ~TJsonList();
};


class TJsonParser {
private:
	enum EParserState { ST_PREAMBLE, ST_OBJECT, ST_OBJECT_END, ST_ARRAY, ST_OBJECT_KEY,
						ST_OBJECT_VALUE, ST_OBJECT_KEY_END, ST_OBJECT_VALUE_END };

	TOnHeaderColumn onHeaderColumnMethod;
	TOnDataField onDataFieldMethod;
	std::string json;
	bool debug;
	void onHeaderColumn(const std::string& column, const util::EVariantType type);
	void onDataField(const std::string& key, const std::string& value, const size_t row, const EVariantType type);
	void init();
	std::string extractQuotedStr(const std::string& s, size_t& pos);
	std::string extractNativeStr(const std::string& s, size_t& pos);

public:
	bool empty() const { return json.empty(); };

	void clear();
	void parse(size_t rows = std::string::npos);
	void parse(const std::string& json, size_t rows = std::string::npos);

	const std::string& asString() const { return json; };
	void setJSON(const std::string& json) { this->json = json; };
	void loadFromFile(const std::string& fileName, const app::ECodepage codepage = app::ECodepage::CP_DEFAULT);

	template<typename column_t, typename class_t>
		inline void bindColumnHandler(column_t &&onColumn, class_t &&owner) {
			onHeaderColumnMethod = std::bind(onColumn, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onData, class_t &&owner) {
			onDataFieldMethod = std::bind(onData, owner,
					std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		}

	template<typename column_t, typename data_t, typename class_t>
		TJsonParser(column_t&& onColumn, data_t &&onData, class_t&& owner) {
			debug = false;
			bindColumnHandler(onColumn, owner);
			bindDataHandler(onData, owner);
		}

	TJsonParser();
	virtual ~TJsonParser();
};

} /* namespace app */

#endif /* JSON_H_ */
