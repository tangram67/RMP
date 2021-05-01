/*
 * tables.h
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef TABLES_H_
#define TABLES_H_

#include "templates.h"
#include "tabletypes.h"
#include "stringutils.h"
#include "dataclasses.h"
#include "datetime.h"
#include "classes.h"
#include "variant.h"
#include "json.h"

namespace data {

class TContent {
public:
	virtual bool empty() const = 0;
	operator bool() const { return !empty(); };

	virtual ~TContent() = default;
};


class TColumn {
public:
	size_t index;
	std::string name;
	EFieldType type;
	int modifier;
	util::EVariantType variant;
	size_t size;
	bool enabled;

	void clear();
	void assign(const TColumn &value);
	TColumn& operator=(const TColumn& value);
	TColumn& operator=(const TColumn&& value);

	void debugOutput(const std::string& preamble = "");

	TColumn();
	TColumn(TColumn &value);
	TColumn(const TColumn &value);
	TColumn(TColumn &&value) = delete;
	TColumn(const TColumn &&value) = delete;
	virtual ~TColumn() = default;
};


class TColumns : public TColumnList, public app::TObject {
public:
	bool validIndex(const size_t index) const;

	TColumns();
	virtual ~TColumns();
};


class THeader : public data::TContent, app::TObject {
private:
	TColumns columns;
	TColumnMap map;

public:
	void clear();
	bool validIndex(const size_t index) const;

	size_t size() const { return columns.size(); };
	bool empty() const { return (columns.size() <= 0); };

	void addColumn(PColumn& column);
	void addColumn(const TColumn& column);
	PColumn getColumn(const size_t index) const;
	PColumn getColumn(const std::string& name) const;
	size_t find(const std::string& name) const;
	size_t find(const size_t index) const;

	void asHTML(util::TStringList& html) const;
	std::string text(const char delimiter = ';');

	void assign(const THeader &value);
	THeader& operator=(const THeader &value);
	THeader& operator=(const THeader &&value);
	PColumn operator[] (const std::size_t index) const;
	PColumn operator[] (const std::string& name) const;
	PColumn operator[] (const char* name) const;
	void operator() (PColumn& column);
	void operator() (const TColumn& column);

	void debugOutput(const std::string& preamble = "", const std::string& name = "Column") const;

	THeader();
	THeader(THeader &value);
	THeader(const THeader &value);
	THeader(THeader &&value) = delete;
	THeader(const THeader &&value) = delete;
	virtual ~THeader();
};



class TField : public data::TContent {
private:
	size_t size;
	PColumn header;
	bool ownsHeader;
	const app::TLocale* locale;
	util::EDateTimeFormat dateTimeType;
	util::EDateTimePrecision dateTimePrecision;
	sql::EParameterType parameterType;
	void clear();
	bool empty() const { return isValid(); };
	std::string defVal;

public:
	util::TVariant value;

	PColumn getHeader() { return header; };
	bool hasHeader() const { return util::assigned(header); };
	bool isValid() const { return (value.getType() != util::EVT_INVALID && value.getType() != util::EVT_UNKNOWN); };
	const std::string& getName() const;
	size_t getIndex() const;
	EFieldType getFieldType() const;
	sql::EParameterType getParameterType() const { return parameterType; };
	void setParamterType(const sql::EParameterType value) { parameterType = value; };
	size_t getSize() const { return size; };
	void setSize(const size_t size) { this->size = size; };
	void setHeader(PColumn column, const bool isOwner = false);
	void assign(const TField& value);

	void imbue(const app::TLocale& locale);
	void setTimeFormat(const util::EDateTimeFormat value);
	void setTimePrecision(const util::EDateTimePrecision value);

	TField& operator=(const TField &value);
	TField& operator=(const TField &&value);

	void debugOutput(const std::string& preamble = "");
	std::string asJSON(const std::string& preamble = "");

	TField();
	TField(TField& value);
	TField(const TField& value);
	TField(TField &&value) = delete;
	TField(const TField &&value) = delete;
	virtual ~TField();
};


class TFields : public TFieldList, public app::TObject {
public:
	bool validIndex(const size_t index) const;

	TFields();
	virtual ~TFields();
};


class TRecord : public data::TContent, public app::TObject {
private:
	TFields fields;
	TFieldMap map;
	data::TField defField;
	const app::TLocale* locale;
	mutable util::TVariant defVar;
	util::EDateTimeFormat dateTimeType;
	util::EDateTimePrecision dateTimePrecision;

public:
	void clear();

	size_t size() const { return fields.size(); };
	bool empty() const { return fields.empty(); };
	bool validIndex(const size_t index) const;

	size_t find(const size_t index) const;
	size_t find(const std::string& name) const;
	size_t find(const char* name) const;

	bool filter(const std::string& filter) const;

	void asJSON(const std::string& preamble, util::TStringList& json, const bool separator = false);
	std::string text(const char delimiter = ';');

	void imbue(const app::TLocale& locale);
	void setTimeFormat(const util::EDateTimeFormat value);
	void setTimePrecision(const util::EDateTimePrecision value);

	void addField(PField& field);
	void addField(TField& field);

	PField getField(const size_t index) const;
	PField getField(const std::string& name) const;
	PField getField(const char* name) const;

	const TField& field(const size_t index) const;
	const TField& field(const std::string& name) const;
	const TField& field(const char* name) const;

	const util::TVariant& value(const size_t index) const;
	const util::TVariant& value(const std::string& name) const;
	const util::TVariant& value(const char* name) const;

	const util::TVariant& operator[] (const std::size_t index) const;
	const util::TVariant& operator[] (const std::string& name) const;
	const util::TVariant& operator[] (const char* name) const;

	void assign(const data::TRecord& value);
	TRecord& operator= (const TRecord &value);
	TRecord& operator= (const TRecord &&value);
	void operator() (PField& field);

	void debugOutput(const std::string& preamble = "", const std::string& name = "Field") const;

	TRecord();
	TRecord(TRecord& value);
	TRecord(const TRecord& value);
	TRecord(TRecord &&value) = delete;
	TRecord(const TRecord &&value) = delete;
	virtual ~TRecord();
};


class TRecords : public TRecordList, public app::TObject {
private:
	mutable util::TStringList json;
	mutable util::TStringList html;

public:
	bool validIndex(const size_t index) const;

	void clear();

	void asJSON(util::TStringList& json, const util::EJsonArrayType type = util::EJT_DEFAULT, size_t rows = 0) const;
	const util::TStringList& asJSON(size_t rows = 0) const;

	void asHTML(util::TStringList& html) const;
	const util::TStringList& asHTML() const;

	TRecords();
	virtual ~TRecords();
};


class TTable : public data::TContent, public app::TObject, private util::TJsonParser {
private:
	THeader header;
	TRecords records;
	TPrimeMap index;
	std::string primary;
	const app::TLocale* locale;
	util::EDateTimeFormat dateTimeType;
	util::EDateTimePrecision dateTimePrecision;
	mutable TRecord defRec;
	mutable TRecords filtered;
	mutable util::TStringList html;

	void onJsonHeaderColumn(const std::string& column, const util::EVariantType type);
	void onJsonDataField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type);
	void addColumn(const std::string& column, const util::EVariantType type);
	void addField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type);
	void addPrime(const std::string& value, PRecord record);
	PRecord recordNeeded(const size_t row);

	void copy(const TTable& table);
	void move(TTable& table);

public:
	typedef TRecordList::const_iterator const_iterator;

	const_iterator begin() const { return records.begin(); };
	const_iterator end() const { return records.end(); };
	size_t size() const { return records.size(); };
	bool empty() const { return records.empty(); };

	PRecord getRecord(const size_t index) const;
	const TRecords& getRecords() const { return records; };
	void addRecord(PRecord& record);
	void addRecord(TRecord& record);

	void setHeader(THeader& header);
	const THeader& getHeader() { return header; };
	bool hasHeader() { return !header.empty(); };

	void imbue(const app::TLocale& locale);
	void setTimeFormat(const util::EDateTimeFormat value);
	void setTimePrecision(const util::EDateTimePrecision value);

	const TRecords& filter(const std::string& filter) const;

	void asJSON(util::TStringList& json, const util::EJsonArrayType type = util::EJT_DEFAULT, size_t rows = 0) const;
	const util::TStringList& asJSON(size_t rows = 0) const;

	void asHTML(util::TStringList& html) const;
	const util::TStringList& asHTML() const;

	void parseJSON(const std::string& json);
	void loadAsJSON(const std::string& fileName, const app::ECodepage codepage = app::ECodepage::CP_DEFAULT);
	void saveAsJSON(const std::string& fileName);

	void saveToFile(const std::string& fileName, const bool addHeader = true, const char delimiter = '\t');
	void loadFromFile(const std::string& fileName,
			const app::ECodepage codepage = app::ECodepage::CP_DEFAULT, const bool hasHeader = true, const char delimiter = '\t');

	void prime(const std::string& column);
	PRecord find(const std::string& value) const;

	bool validIndex(const size_t index) const;
	void clear();

	TRecord& operator [] (const std::size_t index) const;
	TRecord& operator [] (const std::string value) const;

	TTable& operator = (TTable& table);
	TTable& operator = (const TTable& table);
	TTable& operator = (std::string& json);
	TTable& operator = (const std::string& json);

	void operator () (TRecord& record);
	void operator () (PRecord& record);

	void debugOutputData(const std::string& preamble = "", size_t count = 0) const;
	void debugOutputColumns(const std::string& preamble = "") const;

	TTable();
	TTable(TTable& table);
	TTable(const TTable& table);
	TTable(TTable &&value);
	TTable(const TTable &&value);
	TTable(std::string& json);
	TTable(const std::string& json);
	TTable(std::string&& json);
	TTable(const std::string&& json);
	virtual ~TTable();
};

} /* namespace sql */

#endif /* TABLES_H_ */
