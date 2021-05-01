/*
 * database.h
 *
 *  Created on: 04.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include <vector>
#include "templates.h"
#include "dataclasses.h"
#include "stringutils.h"
#include "localizations.h"
#include "variant.h"
#include "classes.h"
#include "inifile.h"
#include "tables.h"
#include "logger.h"
#include "ASCII.h"
#include "hash.h"
#include "gcc.h"

namespace sql {


class TDataSet : public TPersistent {
private:
	void init();

protected:
	TContainer db;
	PSession session;

	std::string text;
	util::hash_type hash;
	data::TTable table;
	data::THeader header;
	data::TRecord params;
	bool prepared;
	bool buffered;
	bool colBound;
	bool paraBound;
	bool opened;
	int colCount;
	int rowCount;
	int recordCount;
	int paraCount;
	data::TTable::const_iterator cursor;
	util::TVariant defVal;

	mutable util::TStringList json;
	mutable util::TStringList html;

	void clear();
	bool validCursor() const;
	void databaseNeeded();
	void sessionNeeded();
	util::hash_type calcHash();
	void sanitize();

public:
	bool empty() const { return recordCount == 0; };

	void setOwner(PContainer owner);
	void setOwner(TContainer& owner);

	// Getters
	size_t getRowsAffected() const { return rowCount; };
	size_t getRowsReceived() const { return recordCount; };
	int getColumnCount() const { return colCount; };
	int getParameterCount() const { return paraCount; };

	// Get datset objects
	const data::TTable& getTable() const { return table; };
	const data::TRecords& getRecords() const { return getTable().getRecords(); };

	// Data cursor handling
	data::TTable::const_iterator begin() const { return getTable().getRecords().begin(); };
	data::TTable::const_iterator end() const { return getTable().getRecords().end(); };

	// Debugging and informational output methods
	void debugOutputColumns(const std::string& preamble = "");
	void debugOutputParams(const std::string& preamble = "");
	void debugOutputData(const std::string& preamble = "", size_t count = 0);

	// Data import/export helpers
	void saveToFile(const std::string& fileName, const bool addHeader = true, const char delimiter = '\t');
	void loadFromFile(const std::string& fileName,
			const app::ECodepage codepage = app::ECodepage::CP_DEFAULT, const bool hasHeader = true, const char delimiter = '\t');

	void asJSON(util::TStringList& json, const util::EJsonArrayType type = util::EJT_DEFAULT, size_t rows = 0) const;
	util::TStringList& asJSON(const util::EJsonArrayType type = util::EJT_DEFAULT, size_t rows = 0) const;

	void asHTML(util::TStringList& html) const;
	util::TStringList& asHTML() const;

	// Generic owner database types
	TDataSet();
	TDataSet(PContainer owner);
	TDataSet(TContainer& owner);
	TDataSet(const std::string& name, app::TLogFile& infoLog, app::TLogFile& exceptionLog);

	virtual ~TDataSet();
};



class TDataQuery : public TDataSet {
private:
	const app::TLocale* locale;
	data::THeader columns;
	data::TField defField;
	std::string uuid;

	void prime();

protected:
	virtual void init() = 0;
	virtual void open() = 0;
	virtual void close() = 0;
	virtual void getColumns() = 0;
	virtual void bindColumns() = 0;
	virtual void bindParameters() = 0;
	virtual void clearParameters() = 0;
	virtual void getParameters() = 0;
	virtual void addParameter(size_t index, const std::string& name) = 0;
	virtual data::PColumn addColumn(size_t index, const std::string& name) = 0;
	virtual bool assignFields() = 0;

	char paramFormatter;
	size_t parseParameters();

public:
	// Transact SQL text
	util::TStringList SQL;

	// Object properties
	const std::string& getUUID() const { return uuid; };

	// Specialized methods depending on database type
	// --> To be implemented in derived class
	virtual void prepare() = 0;
	virtual void unPrepare() = 0;
	virtual bool first() = 0;
	virtual bool next() = 0;
	virtual bool execSQL() = 0;
	size_t fetchAll();
	void clear();

	// Add header columns for formatting purposes
	void addColumn(const std::string& name, util::EVariantType type);
	util::EVariantType getColumn(const std::string& name);

	// Database type independent getters and setters
	bool isPrepred() const { return prepared; };
	bool isBuffered() const { return buffered; };
	bool hasData() const { return !table.empty(); };
	size_t getRecordCount() const { return table.size(); };

	// Localization support
	void imbue(const app::TLocale& locale);
	void setTimeFormat(const util::EDateTimeFormat value);
	void setTimePrecision(const util::EDateTimePrecision value);

	// Result set access methods (const)
	const data::PRecord getRecord() const;
	const data::TRecord& record() const;

	const data::PField getField(const std::string name) const;
	const data::PField getField(const size_t index) const;
	const data::TField& field(const std::string name) const;
	const data::TField& field(const size_t index) const;

	const util::TVariant& value(const std::string name) const;
	const util::TVariant& value(const size_t index) const;

	const util::TVariant& operator[] (const std::string& name) const;
	const util::TVariant& operator[] (const std::size_t index) const;

	// Query parameter setters (non-const, no explicit const getters)
	util::TVariant& param(const std::string name, const EParameterType type = EPT_DEFAULT);
	util::TVariant& param(const size_t index, const EParameterType type = EPT_DEFAULT);

	// No copy/move constructors
	TDataQuery(TDataQuery& value) = delete;
	TDataQuery(const TDataQuery& value) = delete;
	TDataQuery(TDataQuery &&value) = delete;
	TDataQuery(const TDataQuery &&value) = delete;

	// Generic owner defined database types
	TDataQuery() : TDataSet(), locale(nil) { prime(); };
	TDataQuery(PContainer owner) : TDataSet(owner), locale(nil) { prime(); };
	TDataQuery(TContainer& owner) : TDataSet(owner), locale(nil) { prime(); };
	TDataQuery(const std::string& name, app::TLogFile& infoLog, app::TLogFile& exceptionLog) :
		TDataSet(name, infoLog, exceptionLog), locale(nil) { prime(); };
	virtual ~TDataQuery();
};


class TDataConnector: public TPersistent {
private:
	void clear() {
		opened = false;
		inTransaction = false;
	}

protected:
	bool opened;
	bool inTransaction;
	mutable app::TMutex mtx;
	mutable app::TMutex transact;

public:
	app::TMutex& mutex() { return mtx; };

	// Specialized methods depending on database type
	// --> To be implemented in derived class
	virtual void open() = 0;
	virtual void close() = 0;

	virtual void execute(const std::string& SQL) = 0;
	virtual void transaction() = 0;
	virtual void rollback() = 0;
	virtual void commit() = 0;

	bool isOpen() const { return opened; };

	bool isInTransaction() const {
		app::TLockGuard<app::TMutex> lock(transact);
		return inTransaction;
	};
	bool checkInTransaction() const {
		app::TLockGuard<app::TMutex> lock(transact);
		if (inTransaction) {
			throw util::app_error("sql::TDataConnector::checkInTransaction() : Connection is in transaction.");
		}
		return inTransaction;
	};

	// No copy/move constructors
	TDataConnector(TDataConnector& value) = delete;
	TDataConnector(const TDataConnector& value) = delete;
	TDataConnector(TDataConnector &&value) = delete;
	TDataConnector(const TDataConnector &&value) = delete;

	TDataConnector() : TPersistent() { clear(); };
	virtual ~TDataConnector() = default;
};


class TSession : public TPersistent {
private:
	const app::TLocale* locale;

	TContainerList dbList;
	app::PIniFile config;
	std::string configFolder;
	std::string configFile;
	std::string dataFolder;
	std::string importFolder;
	std::string backupFolder;
	bool openOnInit;

	void readConfig();
	void writeConfig();
	void reWriteConfig();
	size_t find(const std::string& name);
	void clear();

public:
	app::PIniFile getConfig() const { return config; };

	const std::string& getDataFolder() const { return dataFolder; };
	const std::string& getImportFolder() const { return importFolder; };
	const std::string& getBackupFolder() const { return backupFolder; };
	const std::string& getConfigFolder() const { return configFolder; };

	void imbue(const app::TLocale& locale);
	const app::TLocale* getLocale() const { return locale; };

	PContainer open(const std::string& name, EDatabaseType type);
	void close();

	// No copy/move constructors
	TSession(TSession& value) = delete;
	TSession(const TSession& value) = delete;
	TSession(TSession &&value) = delete;
	TSession(const TSession &&value) = delete;

	TSession(const std::string& configFolder, const std::string& dataFolder, app::TLogFile& infoLog, app::TLogFile& exceptionLog);
	virtual ~TSession();
};


template<typename T>
class TQueryGuard
{
private:
	typedef T query_t;
	bool prepared;
	query_t& instance;

public:
	TQueryGuard& operator=(const TQueryGuard&) = delete;
	void prepare() {
		if (!prepared)
			instance.prepare();
		prepared = true;
	}
	void unPrepare() {
		if (prepared)
			instance.unPrepare();
		prepared = false;
	}
	explicit TQueryGuard(query_t& F) : instance(F) {
		prepared = false;
	}
	TQueryGuard(const TQueryGuard&) = delete;
	~TQueryGuard() { unPrepare(); }
};



template<typename T>
class TTransactionGuard
{
private:
	typedef T object_t;
	object_t& instance;
	TDataConnector* database;

public:
	TTransactionGuard& operator=(const TTransactionGuard&) = delete;
	void transaction() {
		if (!database->isInTransaction())
			database->transaction();
	}
	void commit() {
		if (database->isInTransaction())
			database->commit();
	}
	void rollback() {
		if (database->isInTransaction())
			database->rollback();
	}
	explicit TTransactionGuard(object_t& F) : instance(F) {
		database = instance.getDatabase();
	}
	TTransactionGuard(const TTransactionGuard&) = delete;
	~TTransactionGuard() { rollback(); }
};



} /* namespace sql */

#endif /* DATABASE_H_ */
