/*
 * tables.cpp
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#include "ansi.h"
#include "tables.h"
#include "nullptr.h"
#include "compare.h"
#include "exception.h"

namespace data {


TColumn::TColumn() {
	clear();
}

TColumn::TColumn(TColumn &value) {
	assign(value);
}

TColumn::TColumn(const TColumn &value) {
	assign(value);
}


void TColumn::clear() {
	enabled = true;
	type = SFT_UNKNOWN;
	variant = util::EVT_UNKNOWN;
	modifier = 0;
	name.clear();
	index = -1;
	size = 0;
}

void TColumn::assign(const TColumn &value) {
	variant = value.variant;
	type = value.type;
	name = value.name;
	index = value.index;
	size = value.size;
}

TColumn& TColumn::operator=(const TColumn &value) {
	if (&value != this)
		assign(value);
	return *this;
}

TColumn& TColumn::operator=(const TColumn &&value) {
	if (&value != this)
		assign(value);
	return *this;
}

void TColumn::debugOutput(const std::string& preamble) {
	std::cout << preamble << "Name  : " << app::blue << name << app::reset << std::endl;
	std::cout << preamble << "Index : " << index << std::endl;
	std::cout << preamble << "Type  : " << fieldTypeAsString(type) << std::endl;
	std::cout << preamble << "Size  : " << size << std::endl;
}



THeader::THeader() {
	columns.setOwner(this);
}

THeader::THeader(THeader &value) {
	columns.setOwner(this);
	assign(value);
}

THeader::THeader(const THeader &value) {
	columns.setOwner(this);
	assign(value);
}

THeader::~THeader() {
	clear();
}

void THeader::assign(const THeader &value) {
	PColumn o, column;

	// Read fields from source record
	for (size_t idx=0; idx<value.size(); idx++) {
		// Create new header column
		o = value[idx];
		if (util::assigned(o)) {
			// Assign new header values from source header column
			column = new TColumn();
			*column = *o;
			addColumn(column);
		}
	}
}

bool THeader::validIndex(const size_t index) const {
	return columns.validIndex(index);
}

void THeader::clear() {
	map.clear();
	util::clearObjectList(columns);
}

PColumn THeader::getColumn(const size_t index) const {
	if (validIndex(index))
		return columns[index];
	return nil;
}

PColumn THeader::getColumn(const std::string& name) const {
/*
	size_t idx = find(name);
	if (idx != app::nsizet)
		return columns[idx];
*/
	TColumnMap::const_iterator it = map.find(util::tolower(name));
	if (it != map.end())
		return it->second;
	return nil;
}

size_t THeader::find(const std::string& name) const
{
/*
	PColumn o;
	size_t i,n;
	n = columns.size();
	for (i=0; i<n; i++) {
		o = columns[i];
		if (util::assigned(o)) {
			// Ignore case...
			if ((name.size() == o->name.size()) && (0 == util::strncasecmp(o->name, name, name.size())))
				return i;
		}
	}
*/
	TColumnMap::const_iterator it = map.find(util::tolower(name));
	if (it != map.end())
		return it->second->index;
	return app::nsizet;
}

size_t THeader::find(const size_t index) const
{
/*
	PColumn o;
	size_t i,n;
	n = columns.size();
	for (i=0; i<n; i++) {
		o = columns[i];
		if (util::assigned(o)) {
			if (o->index == index)
				return i;
		}
	}
*/
	if (validIndex(index))
		return index;
	return app::nsizet;
}

THeader& THeader::operator=(const THeader &value) {
	if (&value != this)
		assign(value);
	return *this;
}

THeader& THeader::operator=(const THeader &&value) {
	if (&value != this)
		assign(value);
	return *this;
}

PColumn THeader::operator[] (const std::size_t index) const {
	return getColumn(index);
}

PColumn THeader::operator[] (const std::string& name) const {
	return getColumn(name);
}


PColumn THeader::operator[] (const char* name) const {
	return getColumn(name);
}

void THeader::operator() (PColumn& column) {
	addColumn(column);
}

void THeader::operator() (const TColumn& column) {
	addColumn(column);
}

void THeader::addColumn(PColumn& column) {
	// Add column to header list
	if (util::assigned(column)) {
		column->index = columns.size();
		columns.push_back(column);
		if (!column->name.empty())
			map[util::tolower(column->name)] = column;
	}
}

void THeader::addColumn(const TColumn& column) {
	// Create new column to add in list
	PColumn o = new TColumn;
	*o = column;
	addColumn(o);
}

void THeader::debugOutput(const std::string& preamble, const std::string& name) const {
	if (!empty()) {
		for (size_t idx=0; idx<size(); idx++) {
			PColumn o = getColumn(idx);
			std::cout << preamble << app::white << util::succ(idx) << ". " << name << app::reset << std::endl;
			o->debugOutput(preamble + "  ");
			std::cout << std::endl;
		}
	}
}

std::string THeader::text(const char delimiter) {
	std::string retVal("");
	if (!empty()) {
		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			PColumn o = getColumn(idx);
			if (idx < last)
				retVal += o->name + delimiter;
			else
				retVal += o->name;
		}
	}
	return retVal;
}

void THeader::asHTML(util::TStringList& html) const {

	// Add header columns
	if (!empty()) {
		html.add("  <thead>");
		html.add("    <tr>");
		html.add("      <th>#</th>");
		for (size_t idx=0; idx<size(); ++idx) {
			PColumn column = getColumn(idx);
			if (util::assigned(column)) {
				html.add("      <th>" + column->name + "</th>");
			}
		}
		html.add("    </tr>");
		html.add("  </thead>");
	}
}



TField::TField() {
	dateTimeType = util::EDT_DEFAULT;
	dateTimePrecision = util::ETP_DEFAULT;
	parameterType = sql::EPT_DEFAULT;
	ownsHeader = false;
	locale = nil;
	header = nil;
	size = 0;
	value.clear();
	defVal = "<unknown>";
}

TField::TField(TField& value) {
	assign(value);
	defVal = "<unknown>";
}

TField::TField(const TField& value) {
	assign(value);
	defVal = "<unknown>";
}

TField::~TField() {
	clear();
}

void TField::clear() {
	if (ownsHeader && util::assigned(header))
		util::freeAndNil(header);
}

TField& TField::operator= (const TField &value) {
	assign(value);
	return *this;
}

TField& TField::operator= (const TField &&value) {
	assign(value);
	return *this;
}

void TField::assign(const TField& value) {
	clear();
	dateTimeType = value.dateTimeType;
	dateTimePrecision = value.dateTimePrecision;
	parameterType = value.parameterType;
	ownsHeader = value.ownsHeader;
	locale = value.locale;
	size = value.size;
	header = value.header;
	this->value = value.value;
}

void TField::setHeader(PColumn column, const bool isOwner) {
	clear();
	ownsHeader = isOwner;
	this->header = column;
}

const std::string& TField::getName() const {
	if (util::assigned(header))
		return header->name;
	return defVal;
}

size_t TField::getIndex() const {
	if (util::assigned(header))
		return header->index;
	return std::string::npos;
}

EFieldType TField::getFieldType() const {
	if (util::assigned(header))
		return header->type;
	return SFT_UNKNOWN;
}

void TField::debugOutput(const std::string& preamble) {
	std::string s = value.asString();
	std::cout << preamble << "Name    : " << (util::assigned(header) ? header->name : "<not assigned>") << std::endl;
	std::cout << preamble << "Value   : " << app::blue << (s.empty() ? "<empty>" : s) << app::reset << std::endl;
	std::cout << preamble << "Variant : " << value.getTypeAsString() << std::endl;
	std::cout << preamble << "Type    : " << (util::assigned(header) ? fieldTypeAsString(header->type) : "<not assigned>") << std::endl;
	std::cout << preamble << "Size    : " << size << std::endl;
}

std::string TField::asJSON(const std::string& preamble) {
	return util::TJsonValue::valueToStr(
					preamble,
					(hasHeader() ? header->name : "Unknown key"),
					value.asString(),
					value.getType()
				);
}

void TField::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	this->value.imbue(locale);
};

void TField::setTimeFormat(const util::EDateTimeFormat value) {
	this->dateTimeType = value;
	this->value.setTimeFormat(value);
}

void TField::setTimePrecision(const util::EDateTimePrecision value) {
	this->dateTimePrecision = value;
	this->value.setTimePrecision(value);
}



TTable::TTable() {
	locale = nil;
	records.setOwner(this);
	filtered.setOwner(this);
	dateTimeType = util::EDT_DEFAULT;
	dateTimePrecision = util::ETP_DEFAULT;
	bindColumnHandler(&TTable::onJsonHeaderColumn, this);
	bindDataHandler(&TTable::onJsonDataField, this);
}

TTable::TTable(TTable& table) {
	*this = table;
}

TTable::TTable(const TTable& table) {
	*this = table;
}


TTable::TTable(const std::string& json) : TTable() {
	*this = json;
};

TTable::~TTable() {
	clear();
}


bool TTable::validIndex(const size_t index) const {
	return records.validIndex(index);
}

void TTable::clear() {
	if (!index.empty())
		index.clear();
	if (!html.empty())
		html.clear();
	primary.clear();
	header.clear();
	TJsonParser::clear();
	util::clearObjectList(records);
}

PRecord TTable::getRecord(const size_t index) const {
	if (validIndex(index))
		return records[index];
	return nil;
}

//PRecord TTable::operator[] (const std::size_t index) {
//	return getRecord(index);
//}

TRecord& TTable::operator [] (const std::size_t index) const {
	PRecord o = getRecord(index);
	if (util::assigned(o))
		return *o;
	return defRec;
}

TRecord& TTable::operator [] (const std::string value) const {
	PRecord o = find(value);
	if (util::assigned(o))
		return *o;
	else
		return defRec;
}


void TTable::operator () (PRecord& record) {
	addRecord(record);
}

void TTable::operator () (TRecord& record) {
	addRecord(record);
}


TTable& TTable::operator = (const std::string &json) {
	parseJSON(json);
	return *this;
}

TTable& TTable::operator = (const TTable &table) {
	clear();
	this->locale = table.locale;
	this->dateTimeType = table.dateTimeType;
	this->dateTimePrecision = table.dateTimePrecision;
	if (!table.empty()) {
		PRecord record;
		for (size_t idx=0; idx<size(); idx++) {
			record = table.getRecord(idx);
			if (util::assigned(record)) {
				records.push_back(new TRecord(*record));
			}
		}
	}
	return *this;
}


void TTable::addRecord(PRecord& record) {
	if (util::assigned(locale))
		record->imbue(*locale);
	if (dateTimeType != util::EDT_DEFAULT)
		record->setTimeFormat(dateTimeType);
	records.push_back(record);
}

void TTable::addRecord(TRecord& record) {
	// Create new record to add in list
	if (record.size() > 0) {
		// Create new record to add to table
		PRecord o = new TRecord;
		*o = record;
		addRecord(o);
	}
}

void TTable::setHeader(THeader& header) {
	this->header = header;
}

void TTable::debugOutputData(const std::string& preamble, size_t count) const {
	if (!empty()) {
		size_t max = size();
		if (count > 0 && count < size()) {
			max = count;
		}
		for (size_t idx=0; idx<max; idx++) {
			PRecord record = getRecord(idx);
			std::cout << app::blue << preamble << util::succ(idx) << ". Row ................................" << app::reset << std::endl;
			std::cout << std::endl;
			if (record->size() > 0) {
				record->debugOutput(preamble + "  ");
			}
		}
	}
}

const TRecords& TTable::filter(const std::string& filter) const {
	filtered.clear();

	// Filter all records
	if (!empty()) {
		PRecord record;
		for (size_t idx=0; idx<size(); idx++) {
			record = getRecord(idx);
			if (util::assigned(record)) {
				if (record->filter(filter)) {
					filtered.push_back(record);
				}
			}
		}
	}

	return filtered;
}

void TTable::asJSON(util::TStringList& json, const util::EJsonArrayType type, size_t rows) const {
	records.asJSON(json, type, rows);
}

const util::TStringList& TTable::asJSON(size_t rows) const {
	return records.asJSON(rows);
}

void TTable::asHTML(util::TStringList& html) const {
	records.asHTML(html);
}

const util::TStringList& TTable::asHTML() const {
	return records.asHTML();
}


void TTable::debugOutputColumns(const std::string& preamble) const {
	if (!header.empty()) {
		if (!name.empty())
			std::cout << preamble << "Header of table [" << name << "]" << std::endl;
		else
			std::cout << preamble << "Header of table:" << std::endl;
		std::cout << std::endl;
		header.debugOutput(preamble + "  ");
		std::cout << std::endl;
	}
}

void TTable::saveToFile(const std::string& fileName, const bool addHeader, const char delimiter) {
	if (!empty()) {
		util::TStringList list;
		if (addHeader)
			list.add(header.text(delimiter));
		for (size_t i=0; i<size(); i++) {
			PRecord o = getRecord(i);
			list.add(o->text(delimiter));
		}
		list.saveToFile(fileName);
	}
}

void TTable::loadFromFile(const std::string& fileName, const app::ECodepage codepage, const bool hasHeader, const char delimiter) {
	if (!empty())
		clear();

	if(!util::fileExists(fileName))
		return;

	util::TStringList list;
	list.loadFromFile(fileName, codepage);

	if(list.empty())
		return;

	util::TStringList csv;
	PColumn column;
	std::string row;
	size_t col;

	// Load header from first row
	row = list[0];
	if (!row.empty()) {
		csv.split(row, delimiter);
		if (!csv.empty()) {
			// Add all columns found in row
			for (col=0; col<csv.size(); col++) {
				column = new TColumn();
				column->index = col;

				// Take column name from header row if present
				if (hasHeader)
					column->name = csv[col];
				else
					column->name = "Row[" + std::to_string((size_s)col) + "]";

				header.addColumn(column);
			}
		}
	}

	// Only header row present
	if (list.size() <= 1 && hasHeader)
		return;

	// Ignore header?
	size_t start = 0;
	if (hasHeader)
		start = 1;

	// Load data from rows
	size_t idx;
	PField field;
	PRecord record;
	for (idx=start; idx<list.size(); idx++) {
		row = list[idx];
		if (!row.empty()) {
			csv.split(row, delimiter);
			if (!csv.empty()) {
				record = new TRecord();

				// Add all fields found in row
				for (col=0; col<header.size(); col++) {
					const std::string& s = csv[col];
					column = header[col];
					field = new TField();

					if (csv.validIndex(col)) {

						// Guess header column type from given field value
						if (column->type == SFT_UNKNOWN) {
							util::EVariantType type = util::TVariant::guessType(s);
							column->type = variantTypeToFieldType(type);
							column->variant = type;
						}

						// Set field value for given variant type
						field->value.setValue(s, column->variant);

					} else {
						field->value = "";
					}

					field->setHeader(column);
					record->addField(field);
				}

				addRecord(record);
			}
		}
	}

}

void TTable::parseJSON(const std::string& json) {
	clear();
	parse(json);
}

void TTable::prime(const std::string& column) {
	if (hasHeader()) {
		size_t col = header.find(column);
		if (col != app::nsizet) {
			primary = column;
			index.clear();
			size_t rows = records.size();

			// Construct mapped index:
			// Key field as string + record object
			for (size_t n=0; n<rows; n++) {
				PRecord o = records[n];
				if (util::assigned(o)) {
					std::string s = o->value(col).asString();
					if (!s.empty()) {
						addPrime(s, o);
					} else
						throw util::app_error("TTable::prime(" + column + ") : Missing field content for at position (" + std::to_string((size_u)n) + ")");
				} else
					throw util::app_error("TTable::prime(" + column + ") : Invalid field at position (" + std::to_string((size_u)n) + ")");
			}

		} else
			throw util::app_error("TTable::prime(" + column + ") : Invalid column name.");
	} else
		throw util::app_error("TTable::prime(" + column + ") : Missing header.");
}

void TTable::addPrime(const std::string& value, PRecord record) {
	if (util::assigned(record)) {
		TPrimeMap::const_iterator it = index.find(value);
		if (it == index.end()) {
			index.insert(TPrimeItem(value, record));
		} else
			throw util::app_error("TTable::addPrime(" + value + ") : Primary key violation.");
	} else
		throw util::app_error("TTable::addPrime(" + value + ") : Record not assigned.");
}

PRecord TTable::find(const std::string& value) const {
	if (!index.empty()) {
		TPrimeMap::const_iterator it = index.find(value);
		if (it != index.end()) {
			return it->second;
		} else
			return nil;
	} else
		throw util::app_error("TTable::find(" + value + ") : Primary index is empty.");
}

void TTable::onJsonHeaderColumn(const std::string& column, const util::EVariantType type) {
	addColumn(column, type);
}

void TTable::onJsonDataField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type) {
	addField(key, value, row, type);
}

void TTable::addColumn(const std::string& column, const util::EVariantType type) {
	PColumn o = new TColumn();
	o->name = column;
	o->type = variantTypeToFieldType(type);
	o->variant = type;
	header.addColumn(o);
}

void TTable::addField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type) {
	// Ignore data for non existent columns!
	PColumn column = header.getColumn(key);
	if (!util::assigned(column))
		return;

	// Get existing record or create new one
	PRecord record = recordNeeded(row);
	if (!util::assigned(record))
		return;

	// Set variant value of new or existing field
	PField field = record->getField(key);
	if (!util::assigned(field)) {
		field = new TField;
		if (util::TVariant::isValidType(type)) {
			if (column->type == SFT_UNKNOWN) {
				column->type = variantTypeToFieldType(type);
				column->variant = type;
			}
			// TODO No need to set type, overwritten by = operator!
			//field->value.setType(column->variant);
		}
		field->value = value;
		field->setHeader(column);
		record->addField(field);
	} else {
		field->value = value;
	}
}

PRecord TTable::recordNeeded(const size_t row) {
	PRecord retVal = nil;
	if (!validIndex(row)) {
		PRecord o = nil;
		do {
			o = new TRecord;
			addRecord(o);
		} while (!validIndex(row));
		retVal = o;
	} else {
		retVal = getRecord(row);
	}	
	return retVal;
}


void TTable::loadAsJSON(const std::string& fileName, const app::ECodepage codepage) {
	clear();
	TJsonParser::loadFromFile(fileName, codepage);
	parse();
}

void TTable::saveAsJSON(const std::string& fileName) {
	asJSON().saveToFile(fileName);
}

void TTable::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	PRecord o;
	for (size_t i=0; i<size(); ++i) {
		o = records[i];
		if (util::assigned(o))
			o->imbue(locale);
	}
};

void TTable::setTimeFormat(const util::EDateTimeFormat value) {
	dateTimeType = value;
	PRecord o;
	for (size_t i=0; i<size(); ++i) {
		o = records[i];
		if (util::assigned(o))
			o->setTimeFormat(value);
	}
}

void TTable::setTimePrecision(const util::EDateTimePrecision value) {
	dateTimePrecision = value;
	PRecord o;
	for (size_t i=0; i<size(); ++i) {
		o = records[i];
		if (util::assigned(o))
			o->setTimePrecision(value);
	}
}



TRecord::TRecord() {
	// Clear default variants and set to invalid type!
	clear();
	locale = nil;
	fields.setOwner(this);
	dateTimeType = util::EDT_DEFAULT;
	dateTimePrecision = util::ETP_DEFAULT;
	defVar.setType(util::EVT_INVALID);
	defField.value.setType(util::EVT_INVALID);
}


TRecord::TRecord(TRecord& value) {
	assign(value);
	fields.setOwner(this);
	defVar.setType(util::EVT_INVALID);
	defField.value.setType(util::EVT_INVALID);
}

TRecord::TRecord(const TRecord& value) {
	assign(value);
	fields.setOwner(this);
	defVar.setType(util::EVT_INVALID);
	defField.value.setType(util::EVT_INVALID);
}


TRecord::~TRecord() {
	clear();
}

void TRecord::clear() {
	map.clear();
	util::clearObjectList(fields);
}

//PField TRecord::operator[] (const std::size_t index) {
//	return getField(index);
//}
//
//PField TRecord::operator[] (const std::string& name) {
//	return getField(name);
//}

void TRecord::assign(const data::TRecord& value) {
	// Assign properties from source
	locale = value.locale;
	dateTimeType = value.dateTimeType;
	dateTimePrecision = value.dateTimePrecision;

	// Assign fields from source
	clear();
	const TField* o;
	PField field;

	// Read fields from source record
	for (size_t idx=0; idx<value.size(); idx++) {
		// Create new field
		o = value.getField(idx);
		if (util::assigned(o)) {
			// Assign new field values from source field
			field = new TField;
			*field = *o;
			addField(field);
		}
	}
}


const util::TVariant& TRecord::operator[] (const std::size_t index) const {
	PField o = getField(index);
	if (util::assigned(o))
		return o->value;
	return defVar;
}

const util::TVariant& TRecord::operator[] (const std::string& name) const {
	PField o = getField(name);
	if (util::assigned(o))
		return o->value;
	return defVar;
}

const util::TVariant& TRecord::operator[] (const char* name) const {
	PField o = getField(name);
	if (util::assigned(o))
		return o->value;
	return defVar;
}

TRecord& TRecord::operator= (const TRecord &value) {
	if (&value != this)
		assign(value);
	return *this;
}

TRecord& TRecord::operator= (const TRecord &&value) {
	if (&value != this)
		assign(value);
	return *this;
}

void TRecord::operator() (PField& field) {
	return addField(field);
}

void TRecord::addField(PField& field) {
	if (util::assigned(locale))
		field->imbue(*locale);
	if (dateTimeType != util::EDT_DEFAULT)
		field->setTimeFormat(dateTimeType);
	fields.push_back(field);
}

void TRecord::addField(TField& field) {
	// Create new column to add in list
	PField o = new TField;
	*o = field;
	addField(o);
}

PField TRecord::getField(const size_t index) const {
	if (validIndex(index)) {
		return fields[index];
	}
	return nil;
}

PField TRecord::getField(const std::string& name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		return fields[idx];
	}
	return nil;
}

PField TRecord::getField(const char* name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		return fields[idx];
	}
	return nil;
}

const TField& TRecord::field(const size_t index) const {
	if (validIndex(index)) {
		PField o = fields[index];
		if (util::assigned(o))
			return *o;
	}
	return defField;
}

const TField& TRecord::field(const std::string& name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		PField o = fields[idx];
		if (util::assigned(o))
			return *o;
	}
	return defField;
}

const TField& TRecord::field(const char* name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		PField o = fields[idx];
		if (util::assigned(o))
			return *o;
	}
	return defField;
}

const util::TVariant& TRecord::value(const size_t index) const {
	if (validIndex(index)) {
		return fields[index]->value;
	}
	return defVar;
}

const util::TVariant& TRecord::value(const std::string& name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		return fields[idx]->value;
	}
	return defVar;
}

const util::TVariant& TRecord::value(const char* name) const {
	size_t idx = find(name);
	if (idx != app::nsizet) {
		return fields[idx]->value;
	}
	return defVar;
}

bool TRecord::validIndex(const size_t index) const {
	return fields.validIndex(index);
}

bool TRecord::filter(const std::string& filter) const {
	PField o;
	size_t i,n;
	n = fields.size();
	for (i=0; i<n; i++) {
		o = fields[i];
		if (util::assigned(o)) {
			// Filter character fields only...
			if (util::isMemberOf(o->getFieldType(), SFT_VARCHAR,SFT_WIDECHAR)) {
				const std::string& value = o->value.asString();
				if (value.size() >= filter.size()) {
					if (util::strcasestr(value, filter)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}


size_t TRecord::find(const char* name) const {
	if (util::assigned(name)) {
		PField o;
		size_t i,n,len;
		n = fields.size();
		len = strnlen(name, 255);
		for (i=0; i<n; i++) {
			o = fields[i];
			if (util::assigned(o)) {
				// Ignore case...
				if ((len == o->getName().size()) && (0 == util::strncasecmp(o->getName(), name, len)))
					return i;
			}
		}
	}
	return app::nsizet;
}

size_t TRecord::find(const std::string& name) const {
	PField o;
	size_t i,n;
	n = fields.size();
	for (i=0; i<n; i++) {
		o = fields[i];
		if (util::assigned(o)) {
			// Ignore case...
			if ((name.size() == o->getName().size()) && (0 == util::strncasecmp(o->getName(), name, name.size())))
				return i;
		}
	}
	return app::nsizet;
}

size_t TRecord::find(const size_t index) const {
	PField o;
	size_t i,n;
	n = fields.size();
	for (i=0; i<n; i++) {
		o = fields[i];
		if (util::assigned(o)) {
			if (o->getIndex() == index)
				return i;
		}
	}
	return app::nsizet;
}

void TRecord::debugOutput(const std::string& preamble, const std::string& name) const {
	if (!empty()) {
		for (size_t idx=0; idx<size(); idx++) {
			PField o = getField(idx);
			if (!name.empty())
				std::cout << app::white << preamble << util::succ(idx) << ". " << name << app::reset << std::endl;
			else
				std::cout << app::white << preamble << util::succ(idx) << ". Field" << app::reset << std::endl;
			o->debugOutput(preamble + "  ");
			std::cout << std::endl;
		}
	}
}

void TRecord::asJSON(const std::string& preamble, util::TStringList& json, const bool separator) {
	if (!empty()) {

		// Begin new JSON object
		json.add(preamble + "{");

		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			PField o = getField(idx);
			if (util::assigned(o)) {
				if (idx < last)
					json.add(o->asJSON(preamble + "  ") + ",");
				else
					json.add(o->asJSON(preamble + "  "));
			}
		}

		// Close JSON object
		if (separator)
			json.add(preamble + "},");
		else
			json.add(preamble + "}");
	}
}

std::string TRecord::text(const char delimiter) {
	std::string retVal("");
	if (!empty()) {
		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			PField o = getField(idx);
			if (idx < last)
				retVal += o->value.asString() + delimiter;
			else
				retVal += o->value.asString();
		}
	}
	return retVal;
}

void TRecord::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	PField o;
	for (size_t i=0; i<size(); ++i) {
		o = fields[i];
		if (util::assigned(o))
			o->imbue(locale);
	}
};

void TRecord::setTimeFormat(const util::EDateTimeFormat value) {
	dateTimeType = value;
	PField o;
	for (size_t i=0; i<size(); ++i) {
		o = fields[i];
		if (util::assigned(o))
			o->setTimeFormat(value);
	}
}

void TRecord::setTimePrecision(const util::EDateTimePrecision value) {
	dateTimePrecision = value;
	PField o;
	for (size_t i=0; i<size(); ++i) {
		o = fields[i];
		if (util::assigned(o))
			o->setTimePrecision(value);
	}
}


TRecords::TRecords() {
}


TRecords::~TRecords() {
}

void TRecords::clear() {
	if (!json.empty())
		json.clear();
	if (!html.empty())
		html.clear();
	TRecordList::clear();
}

bool TRecords::validIndex(const size_t index) const {
	return (index >= 0 && index < size());
}

void TRecords::asJSON(util::TStringList& json, const util::EJsonArrayType type, size_t rows) const {
	json.clear();

	if (!empty()) {
		bool asObject = util::EJT_OBJECT == type;

		// Begin new JSON object
		std::string preamble;
		if (asObject) {
			preamble = "  ";
			json.add("{");
			json.add(preamble + "\"total\": " + std::to_string((size_u)(rows > 0 ? rows : size())) + ",");
			json.add(preamble + "\"rows\": [");
		} else {
			preamble = "";
			json.add(preamble + "[");
		}

		PRecord record;
		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			record = at(idx);
			if (util::assigned(record)) {
				record->asJSON(preamble + "  ", json, (idx < last));
			}
		}

		// Close JSON array and object
		json.add(preamble + "]");
		if (asObject)
			json.add("}");
	}
}

const util::TStringList& TRecords::asJSON(size_t rows) const {
	if (json.empty())
		asJSON(json, util::EJT_DEFAULT, rows);
	return json;
}

void TRecords::asHTML(util::TStringList& html) const {

	// Open HTML table object
	html.add("<table class=\"table table-bordered\">");

	// Add header if owner is TTable object
	if (hasOwner()) {
		TTable* table = util::asClass<TTable>(getOwner());
		if (util::assigned(table)) {
			table->getHeader().asHTML(html);
		}
	}

	// Open table body
	html.add("  <tbody>");

	// Add table rows
	if (!empty()) {
		html.add("    <tr>");
		size_t line = 1;
		for (size_t idx=0; idx<size(); idx++) {
			PRecord record = at(idx);
			if (util::assigned(record)) {
				html.add("      <th>" + std::to_string((size_u)line++) + "</th>");
				for (size_t fields=0; fields<record->size(); ++fields) {
					PField field = record->getField(fields);
					if (util::assigned(field)) {
						html.add("      <th>" + field->value.asString() + "</th>");
					}
				}
			}
		}
		html.add("    </tr>");
	}

	// Close table body
	html.add("  </tbody>");

	// Close HTML table object
	html.add("</table>");

}

const util::TStringList& TRecords::asHTML() const {
	if (html.empty())
		asHTML(html);
	return html;
}



TFields::TFields() {
}

TFields::~TFields() {
}

bool TFields::validIndex(const size_t index) const {
	return (index >= 0 && index < size());
}


TColumns::TColumns() {
}

TColumns::~TColumns() {
}

bool TColumns::validIndex(const size_t index) const {
	return (index >= 0 && index < size());
}


} /* namespace data */
