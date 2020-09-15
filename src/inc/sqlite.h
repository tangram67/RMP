/*
 * sqlite.h
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef SQLITE_H_
#define SQLITE_H_

#include "gcc.h"
#include "dataclasses.h"
#include "database.h"
#include "localizations.h"
#include "../config.h"

#ifdef USE_SQLITE3
#  include "sqlite3/sqlite3.h"
#  include "sqlitetypes.h"
#endif

namespace sqlite {

class TError {
public:
	void raise(const std::string& s, int result);
};


class TDatabase : public sql::TDataConnector, public TError {
private:
	sqlite3* dbo;

	app::PIniFile config;
	sql::EDatabaseType type;
	std::string database;
	TQueryList queryList;

	bool syncMode;
	bool memJournal;
	bool allowTransactions;
	bool doubleIsDateTime;
	bool useExtendedErrors;
	sqlite3_int64 heapSize;

	void readConfig();
	void writeConfig();
	void reWriteConfig();
	void init();
	void clear();

public:
	void open();
	void close();
	void interrupt();

	void execute(const std::string& SQL);
	void transaction();
	void rollback();
	void commit();

	void setMemoryJournal(const bool value);
	void setSynchronousMode(const bool value);
	bool setExtendedErrorMessageMode(const bool value);
	sqlite3_int64 setHeapSize(sqlite3_int64 heapSize);
	sqlite3_int64 getHeapSize();
	bool useDoubleAsDateTime() const { return doubleIsDateTime; };

	const sql::EDatabaseType getType() const { return type; };
	sqlite3* getDBO() const { return dbo; };

	PQuery addQuery();

	TDatabase(sql::TSession& owner, const std::string& name, sql::EDatabaseType type, const std::string& database);
	TDatabase(const std::string& name, sql::EDatabaseType type, const std::string& database,
			app::TIniFile& config, app::TLogFile& infoLog, app::TLogFile& exceptionLog);
	virtual ~TDatabase();
};


class TQuery : public sql::TDataQuery, public TError {
private:
	sqlite3* dbo;
	sqlite3_stmt* stmt;
	bool doubleIsDateTime;

	void init();
	void open();
	void close();
	void getColumns();
	void bindColumns();
	void updateColumns();
	void bindParameters();
	void clearParameters();
	void getParameters();
	void addParameter(size_t index, const std::string& name);
	data::PColumn addColumn(size_t index, const std::string& name);
	bool assignFields();

public:
	void setOwner(sql::PContainer owner);
	void setOwner(sql::TContainer& owner);

	PDatabase getDatabase() const { return db.db3; };
	bool isConnected() const;

	void prepare();
	void unPrepare();
	bool first();
	bool next();
	bool execSQL();

	TQuery();
	TQuery(sql::PContainer owner);
	TQuery(sql::TContainer& owner);
	TQuery(const std::string& name, sqlite3*& dbo, app::TLogFile& infoLog, app::TLogFile& exceptionLog);
	virtual ~TQuery();
};


class TImport : public sql::TDataSet, public TError {
private:
	sqlite3* dbo;
	TQuery query;
	std::string file;
	util::TTimePart duration;
	util::TDateTime time;
	void init();
	void clearTable();
	bool insertRecord(data::PRecord record);
	bool importTable();

public:
	void clear();
	const std::string& getFile() const { return file; };

	void setOwner(sql::PContainer owner);
	void setOwner(sql::TContainer& owner);

	size_t loadFromFile(const std::string& tableName, const std::string& fileName,
			const app::ECodepage codepage = app::ECodepage::CP_DEFAULT, const bool hasHeader = true, const char delimiter = '\t');

	util::TTimePart getDuration() const;
	util::TTimePart getTimePerRow() const;

	TImport(sql::PContainer owner);
	TImport(sql::TContainer& owner);
	TImport(const std::string& name, sqlite3*& dbo, app::TLogFile& infoLog, app::TLogFile& exceptionLog);
	virtual ~TImport();
};


} /* namespace sqlite */

#endif /* SQLITE_H_ */
