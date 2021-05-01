/*
 * database.cpp
 *
 *  Created on: 04.06.2015
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include "ansi.h"
#include "json.h"
#include "ASCII.h"
#include "database.h"
#include "datetime.h"
#include "exception.h"
#include "fileutils.h"

#include "../config.h"

#ifdef USE_SQLITE3
#  include "sqlite.h"
#endif
#ifdef USE_POSTGRES
#  include "postgres.h"
#endif

namespace sql {


TDataSet::TDataSet() : TPersistent() {
	name = "";
	owner = nil;
	defVal.setType(util::EVT_INVALID);
	clear();
}

TDataSet::TDataSet(TContainer& owner) : TPersistent() {
	setOwner(owner);
}

TDataSet::TDataSet(PContainer owner) : TPersistent() {
	setOwner(*owner);
}

TDataSet::TDataSet(const std::string& name, app::TLogFile& infoLog, app::TLogFile& exceptionLog) : TPersistent() {
	this->owner = nil;
	this->name = name;
	db.setType(EDB_UNKNOWN);
	setInfoLog(&infoLog);
	setErrorLog(&exceptionLog);
	init();
}

TDataSet::~TDataSet() {
}

void TDataSet::setOwner(TContainer& owner) {
	this->owner = &owner;
	setName(owner.getName());
	db.setType(owner.getType());
	setInfoLog(owner.getInfoLog());
	setErrorLog(owner.getErrorLog());
	init();
}

void TDataSet::setOwner(PContainer owner) {
	this->owner = owner;
	if (util::assigned(owner)) {
		setName(owner->getName());
		db.setType(owner->getType());
		setInfoLog(owner->getInfoLog());
		setErrorLog(owner->getErrorLog());
	}
	init();
}

void TDataSet::init() {
	session = nil;
	databaseNeeded();
	clear();
}

void TDataSet::clear() {
	prepared = false;
	buffered = false;
	opened = false;
	colBound = false;
	paraBound = false;
	cursor = end();
	hash = 0;
	colCount = 0;
	rowCount = 0;
	recordCount = 0;
	paraCount = 0;
	if (!header.empty())
		header.clear();
	if (!params.empty())
		params.clear();
	if (!table.empty())
		table.clear();
	if (!json.empty())
		json.clear();
	if (!html.empty())
		html.clear();
	if (!text.empty())
		text.clear();
}

void TDataSet::databaseNeeded() {
#ifdef USE_SQLITE3
	if (db.getType() == EDB_SQLITE3 && !util::assigned(db.db3)) {
		if (util::assigned(owner)) {
			if (util::isClass<sqlite::TDatabase>(owner)) {
				sqlite::PDatabase o = util::asClass<sqlite::TDatabase>(owner);
				if (util::assigned(o)) {
					db.db3 = o;
					std::cout << "TDataSet::databaseNeeded(OK)" << std::endl;
				}
			}
		}
	}
#endif

#ifdef USE_MSSQL_ODBC
	if (db.getType() == EDB_MSSQL && !util::assigned(db.msdb)) {
		if (util::assigned(owner)) {
			if (util::isClass<TDatabase>(owner)) {
				PDatabase o = util::asClass<TDatabase>(owner);
				if (util::assigned(o)) {
					db.msdb = o;
					std::cout << "TDataSet::databaseNeeded(OK)" << std::endl;
				}
			}
		}
	}
#endif
}


void TDataSet::sessionNeeded() {
	if (!util::assigned(session)) {
		app::PObject o = getOwner();
		if (util::assigned(o)) {
			if (util::isClass<TContainer>(o)) {
				PContainer p = util::asClass<TContainer>(o);
				app::PObject q = p->getOwner();
				if (util::isClass<TSession>(q)) {
					PSession s = util::asClass<TSession>(q);
					if (util::assigned(s)) {
						session = s;
					}
				}
			}
		}
	}
}

bool TDataSet::validCursor() const {
	return (cursor != end());
}

void TDataSet::sanitize() {
	// Terminate SQL statement with trailing ';'
	if (!text.empty()) {
		if (';' != text[util::pred(text.size())])
			text += ";";
	}
}

util::hash_type TDataSet::calcHash() {
	return util::calcHash(text);
}

void TDataSet::debugOutputColumns(const std::string& preamble) {
	if (!header.empty()) {
		std::cout << preamble << "Header of query [" << name << "]";
		if (!text.empty())
			std::cout << app::blue << " \"" << text << "\"" << app::reset << std::endl;
		else
			std::cout << std::endl;
		std::cout << std::endl;
		header.debugOutput(preamble + "  ");
		std::cout << std::endl;
	}
}

void TDataSet::debugOutputParams(const std::string& preamble) {
	if (!params.empty()) {
		std::cout << preamble << "Parameter for query [" << name << "]";
		if (!text.empty())
			std::cout << app::blue << " \"" << text << "\"" << app::reset << std::endl;
		else
			std::cout << std::endl;
		std::cout << std::endl;
		params.debugOutput(preamble, "Parameter");
		std::cout << std::endl;
	}
}

void TDataSet::debugOutputData(const std::string& preamble, size_t count) {
	table.debugOutputData(preamble, count);
}

void TDataSet::asJSON(util::TStringList& json, const util::EJsonArrayType type, size_t rows) const {
	table.asJSON(json, type, rows);
}

util::TStringList& TDataSet::asJSON(const util::EJsonArrayType type, size_t rows) const {
	if (json.empty())
		table.asJSON(json, type, rows);
	return json;
}

void TDataSet::asHTML(util::TStringList& html) const {
	table.asHTML(html);
}

util::TStringList& TDataSet::asHTML() const {
	if (html.empty())
		table.asHTML(html);
	return html;
}


void TDataSet::saveToFile(const std::string& fileName, const bool addHeader, const char delimiter) {
	table.saveToFile(fileName, addHeader, delimiter);
}

void TDataSet::loadFromFile(const std::string& fileName, const app::ECodepage codepage, const bool hasHeader, const char delimiter) {
	table.loadFromFile(fileName, codepage, hasHeader, delimiter);
}






TDataQuery::~TDataQuery() {
	header.clear();
}

void TDataQuery::prime() {
	paramFormatter = '@';
	uuid = util::fastCreateUUID(true, true);
}

size_t TDataQuery::fetchAll() {
	if (!buffered) {
		// Fetch all record from beginning or current position to end of data set
		// Remark: next() implicitly calls first() if appropriate!
		// --> first() and next() adds current record to buffered record set!
		do {} while (next());
	}
	return getRecordCount();
}

void TDataQuery::clear() {
	TDataSet::clear();
//	if (!SQL.empty())
//		SQL.clear();
}

size_t TDataQuery::parseParameters() {
	// std::cout << "TDataQuery::parseParameters() Parameters for \"" << text << "\"" << std::endl;
	size_t retVal = 0;
	if (!text.empty()) {
		std::string name;
		char p = ' ';
		int state = 0;
		size_t start = 0, len = 0;
		for (size_t i=0; i<text.size(); ++i) {
			char c = text[i];
			// std::cout << "TDataQuery::parseParameters() Parse [" << c << "] state = " << state << " start = " << start << std::endl;
			switch (state) {
				case 0:
					// Ignore qouted strings
					if (p != '\\' && c == '\'') {
						state = 10;
						break;
					}
					// Look for parameter value
					// e.g. "xyz col=$1"
					if (c == paramFormatter && (p == ' ' || p == '=' || p == '(' || p == ',' || p == '\n' || p == '\r')) {
						state = 20;
						start = i;
						break;
					}
					break;

				case 10:
					// Wait for end of quoted string
					if (p != '\\' && c == '\'') {
						state = 0;
					}
					break;

				case 20:
					// Wait for next whitespace
					if (!util::isUnsigned(c)) {
						len = i - start;
						// std::cout << "TDataQuery::parseParameters() Parameter found, length = " << len << std::endl;
						if (len > 1) {
							name = text.substr(start, len);
							// std::cout << "TDataQuery::parseParameters() Parameter \"" << name << "\" found." << std::endl;
							if (!name.empty()) {
								addParameter(retVal, name);
								++retVal;
							}
						}
						start = 0;
						state = 0;
					}
					break;

				default:
					state = 0;
					break;
			}
			p = c;
		}
		if (start > 0) {
			// Statement ends with last parameter
			name = text.substr(start);
			if (name.size() > 1) {
				addParameter(retVal, name);
				++retVal;
			}
		}
	}
	return retVal;
}

void TDataQuery::addColumn(const std::string& name, util::EVariantType type) {
	if (app::nsizet == header.find(name)) {
		data::PColumn o = new data::TColumn;
		o->name = name;
		o->variant = type;
		o->type = data::variantTypeToFieldType(type);
		header.addColumn(o);
	}
}

util::EVariantType TDataQuery::getColumn(const std::string& name) {
	size_t idx = header.find(name);
	if (idx != app::nsizet) {
		data::PColumn o = header[idx];
		if (util::assigned(o))
			return o->variant;
	}
	return util::EVT_INVALID;
}

const data::PRecord TDataQuery::getRecord() const {
	if (!validCursor())
		throw util::app_error("TQuery::getRecord() : Invalid cursor value for field <" + name + ">");
	return *cursor;
}

const data::PField TDataQuery::getField(const std::string name) const {
	data::PRecord o = getRecord();
	if (!util::assigned(o))
		throw util::app_error("TQuery::getField() : No cursor content for field <" + name + ">");
	return o->getField(name);
}

const data::PField TDataQuery::getField(const size_t index) const {
	data::PRecord o = getRecord();
	if (!util::assigned(o))
		throw util::app_error("TQuery::getField() : No cursor content for field (" + std::to_string((size_s)index) + ")");
	return o->getField(index);
}

const data::TField& TDataQuery::field(const std::string name) const {
	data::PField o = getField(name);
	if (!util::assigned(o))
		return defField;
	return *o;
}

const data::TField& TDataQuery::field(const size_t index) const {
	data::PField o = getField(index);
	if (!util::assigned(o))
		return defField;
	return *o;
}

const util::TVariant& TDataQuery::value(const std::string name) const {
	data::PField o = getField(name);
	if (util::assigned(o))
		return o->value;
	return defVal;
}

const util::TVariant& TDataQuery::value(const size_t index) const {
	data::PField o = getField(index);
	if (util::assigned(o))
		return o->value;
	return defVal;
}

const data::TRecord& TDataQuery::record() const {
	if (!validCursor())
		throw util::app_error("TQuery::record() : Invalid cursor.");
	return **cursor;
}

util::TVariant& TDataQuery::param(const std::string name, const EParameterType type) {
	if (params.empty())
		throw util::app_error("TQuery::param() : Parameter list empty.");

	data::PField o = params.getField(name);
	if (util::assigned(o)) {
		o->setParamterType(type);
		return o->value;
	}

	throw util::app_error("TQuery::param() : Invalid parameter name <" + name + ">");
}

util::TVariant& TDataQuery::param(const size_t index, const EParameterType type) {
	if (params.empty())
		throw util::app_error("TQuery::param() : Parameter list empty.");

	data::PField o = params.getField(index);
	if (util::assigned(o)) {
		o->setParamterType(type);
		return o->value;
	}

	throw util::app_error("TQuery::param() : Invalid parameter index (" + std::to_string((size_s)index) + ")");
}

const util::TVariant& TDataQuery::operator[] (const std::string& name) const {
	return value(name);
}

const util::TVariant& TDataQuery::operator[] (const std::size_t index) const {
	return value(index);
}

void TDataQuery::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	table.imbue(locale);
};

void TDataQuery::setTimeFormat(const util::EDateTimeFormat value) {
	table.setTimeFormat(value);
}

void TDataQuery::setTimePrecision(const util::EDateTimePrecision value) {
	table.setTimePrecision(value);
}





TSession::TSession(const std::string& configFolder, const std::string& dataFolder, app::TLogFile& infoLog, app::TLogFile& exceptionLog) {
	locale = nil;
	setInfoLog(&infoLog);
	setErrorLog(&exceptionLog);
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "database.conf";
	config = new app::TIniFile(configFile);
	this->dataFolder = dataFolder + "db/";
	importFolder = this->dataFolder + "import/";
	backupFolder = this->dataFolder + "backup/";
	reWriteConfig();

	// Try to create data folders
	if (!util::folderExists(this->dataFolder))
		if (!util::createDirektory(this->dataFolder))
			throw util::app_error("app::TSession::TSession() : Creating data folder failed <" + this->dataFolder + ">");
	if (!util::folderExists(importFolder))
		if (!util::createDirektory(importFolder))
			throw util::app_error("app::TSession::TSession() : Creating import folder failed <" + importFolder + ">");
	if (!util::folderExists(backupFolder))
		if (!util::createDirektory(backupFolder))
			throw util::app_error("app::TSession::TSession() : Creating backup folder failed <" + backupFolder + ">");
}

TSession::~TSession() {
	config->flush();
	close();
	clear();
	util::freeAndNil(config);
}


void TSession::imbue(const app::TLocale& locale) {
	this->locale = &locale;
};


void TSession::readConfig()
{
	config->setSection("Global");
	dataFolder   = config->readPath("DataFolder", dataFolder);
	importFolder = config->readPath("ImportFolder", importFolder);
	backupFolder = config->readPath("BackupFolder", backupFolder);
	openOnInit   = config->readBool("OpenOnInit", openOnInit);
}

void TSession::writeConfig()
{
	config->setSection("Global");
	config->writePath("DataFolder", dataFolder);
	config->writePath("ImportFolder", importFolder);
	config->writePath("BackupFolder", backupFolder);
	config->writeBool("OpenOnInit", openOnInit, app::INI_BLYES);
}

void TSession::reWriteConfig()
{
	readConfig();
	writeConfig();
}


size_t TSession::find(const std::string& name)
{
	PContainer o;
	size_t i, n = dbList.size();
	for (i=0; i<n; i++) {
		o = dbList[i];
		if (util::assigned(o)) {
			if (o->getName() == name)
				return i;
		}
	}
	return app::nsizet;
}


PContainer TSession::open(const std::string& name, EDatabaseType type)
{
	size_t idx;
	PContainer o = nil;
	std::string datadir;

	idx = find(name);
	if (idx == app::nsizet) {
		std::string database;
		switch (type) {

#ifdef USE_SQLITE3
			case EDB_SQLITE3:
				// Try to create database sub folder
				datadir = util::validPath(dataFolder + name);
				if (!util::folderExists(datadir))
					if (!util::createDirektory(datadir))
						throw util::app_error("app::TSession::addDatabase() : Creating database folder failed <" + datadir + ">");
				database = datadir + name + ".db";

				o = new TContainer;
				o->setName(name);
				o->setType(type);
				o->setOwner(this);
				o->setErrorLog(getErrorLog());
				o->setInfoLog(getInfoLog());
				dbList.push_back(o);

				o->db3 = new sqlite::TDatabase(*this, name, type, database);
				if (openOnInit) {
					o->db3->open();
				} else {
					if (util::assigned(infoLog))
						infoLog->write("Open on init disabled for <" + name + "::" + database + ">");
				}
				break;
#endif

#ifdef USE_POSTGRES
			case EDB_PGSQL:
				database = name;
				o = new TContainer;
				o->setName(name);
				o->setType(type);
				o->setOwner(this);
				o->setErrorLog(getErrorLog());
				o->setInfoLog(getInfoLog());
				dbList.push_back(o);

				o->pgdb = new postgres::TDatabase(*this, name, type, database);
				o->pgdb->setLocale(getLocale());
				if (openOnInit) {
					o->pgdb->open();
				} else {
					if (util::assigned(infoLog))
						infoLog->write("Open on init disabled for <" + name + "::" + database + ">");
				}
				break;
#endif

			default:
				throw util::app_error("TSession::addDatabase(" + name + ") : Database type " + dataBaseTypeToStr(type) + " not supported.");
				break;
		}
	} else {
		throw util::app_error("TSession::addDatabase() failed: Database <" + name + "> exists.");
	}

	return o;
}


void TSession::close()
{
	PContainer o;
	size_t i,n;
	n = dbList.size();
	for (i=0; i<n; i++) {
		o = dbList[i];
		if (util::assigned(o)) {
			switch (o->getType()) {

#ifdef USE_SQLITE3
				case EDB_SQLITE3:
					if (o->db3->isOpen()) {
						o->db3->interrupt();
						o->db3->close();
					}
					break;
#endif
#ifdef USE_POSTGRES
				case EDB_PGSQL:
					if (o->pgdb->isOpen()) {
						o->pgdb->interrupt();
						o->pgdb->close();
					}
					break;
#endif
				default:
					throw util::app_error("TSession::close() : Database type " + dataBaseTypeToStr(o->getType()) + " not supported.");
					break;
			}
		}
	}
}


void TSession::clear() {
	util::clearObjectList(dbList);
}


} /* namespace sql */
