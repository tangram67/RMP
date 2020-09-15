/*
 * sqlite.cpp
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#include "sqlite.h"
#include "syslocale.h"
#include <math.h>
#include <cfloat>

namespace sqlite {


data::EFieldType sqlFieldTypeToType(const int type) {
	data::EFieldType retVal = data::SFT_UNKNOWN;

	// Convert internal SQLite3 field types to enumerated SQL field types
	switch (type) {
		case SQLITE_INTEGER:
			retVal = data::SFT_INTEGER64;
			break;
		case SQLITE_FLOAT:
			retVal = data::SFT_DOUBLE;
			break;
		case SQLITE_TEXT:
			retVal = data::SFT_VARCHAR;
			break;
		case SQLITE_BLOB:
			retVal = data::SFT_BLOB;
			break;
		case SQLITE_NULL:
			retVal = data::SFT_NULL;
			break;
		default:
			break;
	}

	return retVal;
}


void TError::raise(const std::string& s, int result) {
	throw util::app_error(s + " failed (" + std::to_string((size_s)result) + ") <" + std::string(sqlite3_errstr(result)) + ">");
}



TDatabase::TDatabase(const std::string& name, sql::EDatabaseType type, const std::string& database, app::TIniFile& config, app::TLogFile& infoLog, app::TLogFile& exceptionLog) {
	this->owner = nil;
	this->name = name;
	this->type = type;
	this->database = database;
	this->config = &config;
	setInfoLog(&infoLog);
	setErrorLog(&exceptionLog);
	init();
}

TDatabase::TDatabase(sql::TSession& owner, const std::string& name, sql::EDatabaseType type, const std::string& database) {
	this->owner = &owner;
	this->name = name;
	this->type = type;
	this->database = database;
	this->config = owner.getConfig();
	setInfoLog(owner.getInfoLog());
	setErrorLog(owner.getErrorLog());
	init();
}

TDatabase::~TDatabase() {
	// Sqlite assigns a value to dbo even if open failed
	// --> Cleanup anyway!
	if (isOpen() || util::assigned(dbo)) {
		close();
	}
	clear();
}

void TDatabase::init() {
	queryList.clear();
	opened = false;
	useExtendedErrors = true;
	doubleIsDateTime = false;
	allowTransactions = true;
	inTransaction = false;
	memJournal = false;
	syncMode = true;
	heapSize = 0;
	dbo = nil;
	reWriteConfig();
}


void TDatabase::open() {
	if (!database.empty()) {
		int retVal = sqlite3_open_v2(database.c_str(), &dbo, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nil);
		if (retVal == SQLITE_OK) {
			opened = true;
			setMemoryJournal(memJournal);
			setSynchronousMode(syncMode);
			setHeapSize(heapSize);
			setExtendedErrorMessageMode(useExtendedErrors);
			writeLog("Database <" + database + "> opened.");
			writeLog("Database <" + database + "> Heap size = " + std::to_string((size_s)getHeapSize()));
		} else {
			raise("sqlite::TDatabase::open()", retVal);
		}
	} else {
		raise("sqlite::TDatabase::open()", SQLITE_ERROR);
	}
}

void TDatabase::close() {
	if (util::assigned(dbo)) {
		if (inTransaction)
			commit();
		int retVal = sqlite3_close(dbo);
		opened = false;
		dbo = nil;
		if (retVal == SQLITE_OK) {
			writeLog("Database <" + database + "> closed.");
		} else {
			raise("sqlite::TDatabase::close()", retVal);
		}
	}
}

void TDatabase::interrupt() {
	if (util::assigned(dbo))
		sqlite3_interrupt(dbo);
}


// See http://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite
void TDatabase::transaction() {
	if (allowTransactions && !inTransaction) {
		execute("BEGIN TRANSACTION");
		inTransaction = true;
	}
}

void TDatabase::commit() {
	if (allowTransactions && inTransaction) {
		inTransaction = false;
		execute("END TRANSACTION");

	}
}

void TDatabase::rollback() {
	if (allowTransactions && inTransaction) {
		inTransaction = false;
		execute("ROLLBACK TRANSACTION");
	}
}


void TDatabase::execute(const std::string& SQL) {
	if (!SQL.empty()) {
		if (opened && util::assigned(dbo)) {
			char* p = nil;
			int retVal = sqlite3_exec(dbo, SQL.c_str(), nil, nil, &p);
			if (SQLITE_OK != retVal) {
				if (util::assigned(p))
					raise("sqlite::TDatabase::execute() [" + SQL + "] \"" + std::string(p) + "\"", retVal);
				else
					raise("sqlite::TDatabase::execute() [" + SQL + "]", retVal);
			}
		} else {
			throw util::app_error("sqlite::TDatabase::execute() : Empty SQL query string.");
		}
	}
}


void TDatabase::setMemoryJournal(const bool value) {
	std::string sql = value ? "PRAGMA journal_mode = MEMORY" : "PRAGMA journal_mode = DELETE";
	execute(sql);
}

void TDatabase::setSynchronousMode(const bool value) {
	std::string sql = value ? "PRAGMA synchronous = FULL" : "PRAGMA synchronous = OFF";
	execute(sql);
}

sqlite3_int64 TDatabase::getHeapSize() {
	return setHeapSize((sqlite3_int64)-1);
}

sqlite3_int64 TDatabase::setHeapSize(sqlite3_int64 heapSize) {
	return sqlite3_soft_heap_limit64((sqlite3_int64)heapSize);
}

bool TDatabase::setExtendedErrorMessageMode(const bool value) {
	return SQLITE_OK == sqlite3_extended_result_codes(dbo, value ? 1 : 0);
}


void TDatabase::readConfig()
{
	config->setSection(name);
	database = config->readString("Database", database);
	heapSize = (sqlite3_int64)config->readSize("HeapSize", heapSize);
	syncMode = config->readBool("SynchronousMode", syncMode);
	memJournal = config->readBool("MemoryJournal", memJournal);
	useExtendedErrors = config->readBool("UseExtendedErrors", useExtendedErrors);
	allowTransactions = config->readBool("AllowTransactions", allowTransactions);
	doubleIsDateTime = config->readBool("TreatDoubleAsDateTime", doubleIsDateTime);

}

void TDatabase::writeConfig()
{
	config->setSection(name);
	config->writeString("Database", database);
	config->writeSize("HeapSize", (size_t)heapSize);
	config->writeBool("SynchronousMode", syncMode, app::INI_BLON);
	config->writeBool("MemoryJournal", memJournal, app::INI_BLON);
	config->writeBool("UseExtendedErrors", useExtendedErrors, app::INI_BLYES);
	config->writeBool("AllowTransactions", allowTransactions, app::INI_BLYES);
	config->writeBool("TreatDoubleAsDateTime", doubleIsDateTime, app::INI_BLYES);
}

void TDatabase::reWriteConfig()
{
	readConfig();
	writeConfig();
}


PQuery TDatabase::addQuery() {
	PQuery o = new TQuery();
	queryList.push_back(o);
	return o;
}

void TDatabase::clear() {
	util::clearObjectList(queryList);
}




TQuery::TQuery() : TDataQuery() {
	init();
}

TQuery::TQuery(sql::TContainer& owner) : sql::TDataQuery(owner) {
	init();
	setOwner(owner);
}

TQuery::TQuery(sql::PContainer owner) : sql::TDataQuery(owner) {
	init();
	setOwner(owner);
}

TQuery::TQuery(const std::string& name, sqlite3*& dbo, app::TLogFile& infoLog, app::TLogFile& exceptionLog)
		: TDataQuery(name, infoLog, exceptionLog) {
	init();
	db.setType(sql::EDB_SQLITE3);
	this->dbo = dbo;
}

TQuery::~TQuery() {
	close();
}

void TQuery::init() {
	paramFormatter = '@';
	dbo = nil;
	stmt = nil;
	doubleIsDateTime = false;
}

void TQuery::setOwner(sql::PContainer owner) {
	if (util::assigned(owner)) {
		TDataSet::setOwner(owner);
		if (util::assigned(owner->db3)) {
			db.db3 = owner->db3;
			dbo = db.db3->getDBO();
			doubleIsDateTime = db.db3->useDoubleAsDateTime();
		}
	} else {
		throw util::app_error("sqlite::TQuery::setOwner() : Database not assigned.");
	}
}

void TQuery::setOwner(sql::TContainer& owner) {
	TDataSet::setOwner(owner);
	if (util::assigned(owner.db3)) {
		db.db3 = owner.db3;
		dbo = db.db3->getDBO();
	}
}

void TQuery::open() {
	if (util::assigned(db.db3)) {
		if (!db.db3->isOpen()) {
			db.db3->open();
			dbo = db.db3->getDBO();
			opened = true;
		}
	} else
		throw util::app_error("sqlite::TQuery::open() : No database to open.");
}

void TQuery::close() {
	// Commit transaction
	if (prepared)
		unPrepare();
	if (!SQL.empty())
		SQL.clear();
	clear();
}

bool TQuery::isConnected() const {
	if (util::assigned(db.db3)) {
		return db.db3->isOpen();
	}
	return false;
}

void TQuery::prepare() {
	open();
	text = util::trim(SQL.raw('\n'));
	if (!text.empty()) {
		int retVal;

		// Terminate SQL statement with trailing ';'
		sanitize();

		// calcHash() is case insensitive like T-SQL by design!
		// --> Unprepare Query if statement text changed!
		util::hash_type h = calcHash();
		if (prepared && h != hash) {
			// SQL text changed
			// --> Prepared statement is invalid
			unPrepare();
		}
		if (!prepared) {
			// Statement was unprepared until now...
			retVal = sqlite3_prepare_v2(dbo, text.c_str(), util::succ(text.size()), &stmt, nil);
			if (retVal == SQLITE_OK) {
				prepared = true;
				hash = h;
				getColumns();
				getParameters();
			} else {
				std::string sExcept = "Statement <" + text + ">";
				if (!params.empty())
					sExcept = sExcept + " Parameter <" + params.text('\n') + ">";
				raise("sqlite::TQuery::prepare::sqlite3_prepare_v2() " + sExcept, retVal);
			}
		} else {
			// Statement was prepared before
			// --> Reset prior to next execution, release parameters
			clearParameters();
			// See https://www.sqlite.org/c3ref/stmt.html
			retVal = sqlite3_reset(stmt);
			if (SQLITE_OK != retVal) {
				std::string sExcept = "Statement <" + text + ">";
				if (!params.empty())
					sExcept = sExcept + " Parameter <" + params.text('\n') + ">";
				raise("sqlite::TQuery::prepare::sqlite3_reset() " + sExcept, retVal);
			}
			stmt = nil;
		}
	} else {
		throw util::app_error("sqlite::TQuery::prepare() : Missing SQL statement.");
	}
}

void TQuery::unPrepare() {
	if (prepared || util::assigned(stmt)) {
		// Release parameters
		clearParameters();

		// Release prepared statement
		int retVal = sqlite3_finalize(stmt);
		if (retVal != SQLITE_OK) {
			raise("sqlite::TQuery::unPrepare()", retVal);
		}

		clear();
	}
	if (opened) {
		if (util::assigned(db.db3))
			db.db3->close();
		opened = false;
	}
}

bool TQuery::first() {
	bool retVal = false;

	// Check if result set already loaded by fetchAll()
	if (buffered) {
		if (table.size() > 0) {
			cursor = begin();
		} else {
			cursor = end();
		}
		return validCursor();
	}

	// Check if called before
	if (colBound)
		return retVal;

	// Prepare statement if not yet done
	if (!prepared)
		prepare();

	// Bind parameters values to query
	if (paraCount > 0 && !paraBound)
		bindParameters();

	// Step into result set
	int r =  sqlite3_step(stmt);
	switch (r) {
		case SQLITE_ROW:
			bindColumns();
			assignFields();
			recordCount++;
			retVal = true;
			break;
		case SQLITE_DONE:
			// No data to bind here, so assume OK!
			colBound = true;
			buffered = true;
			cursor = end();
			break;
		case SQLITE_INTERRUPT:
			break;
		default:
			raise("sqlite::TQuery::first()", r);
			break;
	}

	return retVal;
}

bool TQuery::next() {
	bool retVal = false;

	// Check if result set already loaded by fetchAll()
	if (buffered) {
		if (table.size() > 0) {
			if (validCursor()) {
				cursor++;
			}
		} else {
			cursor = end();
		}
		return validCursor();
	}

	// Fields not bound, call first() instead!
	if (!colBound)
		return first();

	// Step into result set
	int r =  sqlite3_step(stmt);
	switch (r) {
		case SQLITE_ROW:
			updateColumns();
			assignFields();
			recordCount++;
			retVal = true;
			break;
		case SQLITE_DONE:
			buffered = true;
			break;
		case SQLITE_INTERRUPT:
			break;
		default:
			raise("sqlite::TQuery::next()", r);
			break;
	}

	return retVal;
}

void TQuery::getColumns() {
	if (util::assigned(stmt)) {
		colCount = sqlite3_column_count(stmt);
		for (int idx=0; idx<colCount; idx++) {
			// Read SQlite3 column names and store in record of fields
			std::string name;
			const char* p = sqlite3_column_name(stmt, idx);
			if (util::assigned(p))
				name = std::string(p);
			if (name.empty())
				throw util::app_error("sqlite::TQuery::getColumns() : Invalid column name.");
			addColumn(idx, name);
		}
	}
}


void TQuery::getParameters() {
	if (util::assigned(stmt)) {
		// Read SQlite3 parameter count and names
		paraCount = sqlite3_bind_parameter_count(stmt);
		// First parameter has index 1 (!)
		// See: https://www.sqlite.org/c3ref/bind_parameter_name.html
		for (int idx=1; idx<=paraCount; idx++) {
			std::string name;
			const char* p = sqlite3_bind_parameter_name(stmt, idx);
			if (util::assigned(p))
				name = std::string(p);
			if (name.empty())
				throw util::app_error("sqlite::TQuery::getParams() : Invalid parameter name.");
			addParameter(idx, name);
		}
	}
}


void TQuery::bindColumns() {
	if (util::assigned(stmt)) {
		// Add field information to record
		for (int idx=0; idx<colCount; idx++) {
			// Read SQlite3 columns:
			// SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL
			int type = sqlite3_column_type(stmt, idx);

			// Set field type in column record
			data::PColumn column = header[idx];
			if (util::assigned(column)) {
				column->type = sqlite::sqlFieldTypeToType(type);
				column->size = 0; // SQLite does not deliver data size
				//std::cout << "TQuery::bindColumns() Name \"" << column->name << "\" type = " << type << " field type = " << column->type << std::endl;
			}
		}
		table.setHeader(header);
		colBound = true;
	}
}


void TQuery::updateColumns() {
	if (util::assigned(stmt) && colBound) {
		// Add field information to record
		for (int idx=0; idx<colCount; idx++) {
			// Set field type in column record
			data::PColumn column = header[idx];
			if (util::assigned(column)) {
				if (column->type == data::SFT_NULL) {
					int type = sqlite3_column_type(stmt, idx);
					data::EFieldType fieldType = sqlite::sqlFieldTypeToType(type);
					if (fieldType != data::SFT_NULL) {
						column->type = fieldType;
						//std::cout << "TQuery::updateColumns() Name \"" << column->name << "\" type = " << type << " field type = " << fieldType << std::endl;
					}
				}
			}
		}
	}
}


void TQuery::bindParameters() {
	if (util::assigned(stmt)) {
		// Bind variant values to query
		if (paraCount > 0 && !paraBound) {
			double d, r;
			std::string s;
			std::wstring w;
			const char* p;
			const wchar_t* q;
			size_t size;
			data::PField param;
			data::EFieldType type;

			for (int idx=0; idx<paraCount; idx++) {

				param = params.getField(idx);
				type = data::variantTypeToFieldType(param->value.getType());
				if (param->hasHeader())
					param->getHeader()->type = type;

				int retVal = SQLITE_OK;
				switch (type) {
					case data::SFT_BOOLEAN:
						retVal = sqlite3_bind_int(stmt, param->getIndex(), param->value.asBoolean() ? (int32_t)1 : (int32_t)0);
						break;

					case data::SFT_INTEGER8:
					case data::SFT_INTEGER16:
					case data::SFT_INTEGER32:
					case data::SFT_UNSIGNED8:
					case data::SFT_UNSIGNED16:
					case data::SFT_UNSIGNED32:
						retVal = sqlite3_bind_int(stmt, param->getIndex(), param->value.asInteger());
						break;

					case data::SFT_INTEGER64:
					case data::SFT_UNSIGNED64:
						retVal = sqlite3_bind_int64(stmt, param->getIndex(), param->value.asInteger64());
						break;

					case data::SFT_DOUBLE:
						retVal = sqlite3_bind_double(stmt, param->getIndex(), param->value.asDouble());
						break;

					case data::SFT_VARCHAR:
						// p = param->value.getChar(size);
						size = 0;
						if (!util::assigned(param->value.getChar(&p, size))) {
							s = param->value.asString();
							if (!s.empty()) {
								p = s.c_str();
								size = s.size();
							}
						}
						if (size > 0) {
							retVal = sqlite3_bind_text(stmt, param->getIndex(), p, size, SQLITE_STATIC);
							param->setSize(size);
						}
						break;

					case data::SFT_WIDECHAR:
						// q = param->value.getWideChar(size);
						size = 0;
						if (!util::assigned(param->value.getWideChar(&q, size))) {
							w = param->value.asWideString();
							if (!w.empty()) {
								q = w.c_str();
								size = w.size();
							}
						}
						if (size > 0) {
							retVal = sqlite3_bind_text16(stmt, param->getIndex(), q, size, SQLITE_STATIC);
							param->setSize(size);
						}
						break;

					case data::SFT_BLOB:
						if (util::EVT_BLOB == param->value.getType()) {
							retVal = sqlite3_bind_blob(stmt, param->getIndex(), param->value.asBlob().data(), param->value.asBlob().size(), SQLITE_STATIC);
							param->setSize(param->value.asBlob().size());
						} else {
							retVal = sqlite3_bind_null(stmt, param->getIndex());
						}
						break;

					case data::SFT_BLOB64:
						retVal = sqlite3_bind_null(stmt, param->getIndex());
						break;

					case data::SFT_DATETIME:
						// Use floating point (double) time representation as date time
						d = (double)param->value.asTime().asNumeric();
						if (std::modf(d , &r) <= DBL_EPSILON) {
							// ATTENTION: Add lowest possible offset if value has no decimal places btw. no fractional part
							// --> SQLite3 will return the value as an interger value if no fraction part!
							d = std::nextafter(r, INFINITY);
						}
						retVal = sqlite3_bind_double(stmt, param->getIndex(), d);
						break;

					case data::SFT_NULL:
						retVal = sqlite3_bind_null(stmt, param->getIndex());
						break;

					case data::SFT_UNKNOWN:
						retVal = sqlite3_bind_null(stmt, param->getIndex());
						break;

				}
				if (retVal != SQLITE_OK)
					raise("sqlite::TQuery::bindParameters() failed for index (" + std::to_string((size_s)idx) + ") parameter <" + param->getName() + ">", retVal);
			}

			// All parameter values were bound now..
			paraBound = true;
		}
	}
}

void TQuery::clearParameters() {
	// Reset all parameter bindings
	if (paraBound) {
		paraBound = false;
		int retVal = sqlite3_clear_bindings(stmt);
		if (SQLITE_OK != retVal) {
			raise("sqlite::TQuery::clearParameters()", retVal);
		}
	}
}

data::PColumn TQuery::addColumn(size_t index, const std::string& name) {
	data::PColumn column = new data::TColumn();
	column->index = index;
	column->name = name;
	header(column);
	return column;
}

void TQuery::addParameter(size_t index, const std::string& name) {
	// Add parameter as TField value
	data::PField param = new data::TField();
	data::PColumn header = new data::TColumn();
	header->index = index;
	header->name = name;
	param->setHeader(header, true);
	params(param);
}

bool TQuery::assignFields() {
	if (util::assigned(stmt)) {

		// Create new record to store field values
		data::PRecord record = new data::TRecord();

		// Read all fields and store values in variant class
		for (size_t idx=0; idx<header.size(); idx++) {

			// Read field from record list
			data::PColumn column = header[idx];

			// Assign value to variant
			if (util::assigned(column)) {

				// Read column size
				size_t size = sqlite3_column_bytes(stmt, column->index);
				char* p;

				// First add column information to field
				// --> Stored as pointer reference, so add prior to first access!!!
				// Store field in record
				data::PField field = new data::TField();
				field->setHeader(column);
				field->setSize(size);
				record->addField(field);

				switch (column->type) {
					case data::SFT_BOOLEAN:
						// Read 32/64 Bit signed integer (size in bytes!)
						if (size > 4) {
							field->value = (bool)(sqlite3_column_int64(stmt, column->index) > 0);
							break;
						}
						if (size > 0) {
							field->value = (bool)(sqlite3_column_int(stmt, column->index) > 0);
							break;
						}
						field->value = false;
						break;

					case data::SFT_INTEGER8:
					case data::SFT_INTEGER16:
					case data::SFT_INTEGER32:
					case data::SFT_INTEGER64:
						// Read 32/64 Bit signed integer (size in bytes!)
						if (size > 4) {
							field->value = (int64_t)sqlite3_column_int64(stmt, column->index);
							column->type = data::SFT_INTEGER64;
							break;
						}
						if (size > 0) {
							field->value = sqlite3_column_int(stmt, column->index);
							break;
						}
						field->value = (int)0;
						break;

					case data::SFT_UNSIGNED8:
					case data::SFT_UNSIGNED16:
					case data::SFT_UNSIGNED32:
					case data::SFT_UNSIGNED64:
						// Read 32/64 Bit signed integer (size in bytes!)
						if (size > 4) {
							field->value = (int64_t)sqlite3_column_int64(stmt, column->index);
							column->type = data::SFT_UNSIGNED64;
							break;
						}
						if (size > 0) {
							field->value = sqlite3_column_int(stmt, column->index);
							break;
						}
						field->value = (int)0;
						break;

					case data::SFT_DOUBLE:
						if (doubleIsDateTime) {
							// Treat double as numerical date time value
							column->type = data::SFT_DATETIME;
							if (size > 0) {
								field->value.setDateTime((util::TTimeNumeric)sqlite3_column_double(stmt, column->index));
								break;
							}
							field->value.setDateTime(util::epoch());
						} else {
							// Read double precision floating point
							if (size > 0) {
								field->value = sqlite3_column_double(stmt, column->index);
								break;
							}
							field->value = (double)0.0;
						}
						break;

					case data::SFT_DATETIME:
						// Treat double as numerical date time value
						column->type = data::SFT_DATETIME;
						if (size > 0) {
							field->value.setDateTime((util::TTimeNumeric)sqlite3_column_double(stmt, column->index));
							break;
						}
						field->value.setDateTime(util::epoch());
						break;

					case data::SFT_VARCHAR:
						// Read varchar UTF-8 string
						p = reinterpret_cast<char*>(const_cast<unsigned char*>(sqlite3_column_text(stmt, column->index)));
						if (util::assigned(p) && size > 0) {
							field->value.setString(p, size);
							break;
						}
						field->value = "";
						break;

					case data::SFT_BLOB:
						p = reinterpret_cast<char*>(const_cast<void*>(sqlite3_column_blob(stmt, column->index)));
						if (util::assigned(p) && size > 0) {
							field->value.setBlob(p, size);
							break;
						}
						field->value.getBlob().clear();
						break;

					case data::SFT_NULL:
						// Set database NULL values to empty string value
						// field->value.setType(util::EVT_NULL);
						field->value = "";
						break;

					case data::SFT_UNKNOWN:
					case data::SFT_WIDECHAR:
					case data::SFT_BLOB64:
						break;

				} // switch (column->type)

			} // if (util::assigned(column))

		} // for (size_t idx=0; idx<header.size(); idx++)

		// Add current record to buffered result set if not empty
		if (record->size() > 0) {
			// Next append to table and set cursor to end() - 1
			table(record);
			cursor = util::pred(end());
			return true;
		} else {
			delete record;
		}
	}
	return false;
}

bool TQuery::execSQL() {
	bool retVal = false;

	// Prepare statement
	if (!prepared)
		prepare();

	// Bind parameters values to query
	if (paraCount > 0 && !paraBound)
		bindParameters();

	// Step into result
	int r;
	do {
		r =  sqlite3_step(stmt);
		switch (r) {
			case SQLITE_ROW:
				// No data expected here
				retVal = true;
				break;
			case SQLITE_DONE:
				// SQL script executed
				rowCount = sqlite3_changes(dbo);
				retVal = true;
				break;
			case SQLITE_INTERRUPT:
				retVal = false;
				break;
			default:
				raise("sqlite::TQuery::execSQL()", r);
				retVal = false;
				break;
		}
	} while (r == SQLITE_ROW);

	return retVal;
}




TImport::TImport(sql::TContainer& owner) : sql::TDataSet(owner) {
	init();
	setOwner(owner);
}

TImport::TImport(sql::PContainer owner) : sql::TDataSet(owner) {
	init();
	setOwner(owner);
}

TImport::TImport(const std::string& name, sqlite3*& dbo, app::TLogFile& infoLog, app::TLogFile& exceptionLog)
		: TDataSet(name, infoLog, exceptionLog) {
	init();
}

TImport::~TImport() {
}

void TImport::init() {
	session = nil;
	duration = 0;
	file.clear();
	dbo = nil;
}

void TImport::setOwner(sql::PContainer owner) {
	query.setOwner(owner);
}

void TImport::setOwner(sql::TContainer& owner) {
	query.setOwner(owner);
}

void TImport::clear() {
	time.clear();
	duration = 0;
	rowCount = 0;;
	if (!table.empty())
		table.clear();
	TDataSet::clear();
}

size_t TImport::loadFromFile(const std::string& tableName, const std::string& fileName,
		const app::ECodepage codepage, const bool hasHeader, const char delimiter)
{
	size_t retVal = 0;
	name = tableName;
	clear();

	// Look for valid file
	file = fileName;
	if (!util::fileExists(file)) {
		sessionNeeded();
		if (util::assigned(session)) {
			file = session->getImportFolder() + file;
		}
	}

	try {
		if (!util::fileExists(file))
			throw util::app_error("sqlite::TImport::loadFromFile() : File does not exists <" + fileName + ">");

		// Read content of file in table of query
		table.loadFromFile(file, codepage, hasHeader, delimiter);
		// table.debugOutputColumns("TImport::loadFromFile() Header ");
		// table.debugOutputData("TImport::loadFromFile() Data ", 1);
		if (table.empty())
			throw util::app_error("sqlite::TImport::loadFromFile() : File is empty <" + file + ">");

		// Clear existing entries
		clearTable();

		// Import table in database
		if (importTable())
			retVal = rowCount;

	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "sqlite::TImport::importTable() \n" + sExcept + "\n";
		sText += "Tablename <" + tableName + "> name <" + file + ">\n";
		errorLog(sText);
		throw e;
	}

	return retVal;
}

bool TImport::importTable() {
	data::PRecord record;
	size_t idx;

	if(!table.hasHeader() || table.empty())
		return false;

	// Get number for columns from table header
	// and record count from table
	size_t fieldCount = table.getHeader().size();
	size_t recCount = table.size();
	size_t last = util::pred(fieldCount);

	// Create SQL query for insert of all fields by parameter
	query.SQL.clear();
	query.SQL.add("INSERT INTO");
	query.SQL.add(name);
	query.SQL.add("VALUES (");
	for (idx = 0; idx<fieldCount; idx++) {
		if (idx < last)
			query.SQL.add("@" + std::to_string((size_s)idx) + ",");
		else
			query.SQL.add("@" + std::to_string((size_s)idx));
	}
	query.SQL.add(")");

	// Insert all rows as bulk insert via transaction
	time.start();
	sql::TTransactionGuard<TQuery> transact(query);
	transact.transaction();

	// Insert all records into SQL table
	for (idx=0; idx<recCount; idx++) {
		record = table.getRecord(idx);
		if (!insertRecord(record))
			return false;
	}

	// Commit all rows
	transact.commit();
	duration = time.stop(util::ETP_MICRON);

	return true;
}


bool TImport::insertRecord(data::PRecord record) {
	if (record->size() > 0) {

		// Execute insert statement
		sql::TQueryGuard<TQuery> dbguard(query);
		dbguard.prepare();

		// Bind parameter to each field by index
		size_t paraCount = query.getParameterCount();
		if (paraCount > 0) {
			for (size_t idx = 0; idx<paraCount; idx++) {
				if (record->validIndex(idx))
					query.param(idx) = record->value(idx);
			}
		}

		// Insert row data
		query.execSQL();
		rowCount += query.getRowsAffected();
		return true;

	}
	return false;
}


void TImport::clearTable() {
	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("DELETE FROM");
	query.SQL.add(name);

	// Standard execution of query
	sql::TQueryGuard<TQuery> dbguard(query);
	dbguard.prepare();
	query.execSQL();
}


util::TTimePart TImport::getDuration() const {
	return duration;
}

util::TTimePart TImport::getTimePerRow() const {
	if (rowCount > 0)
		return duration / (util::TTimePart)rowCount;
	return util::epoch();
}

} /* namespace sqlite */
