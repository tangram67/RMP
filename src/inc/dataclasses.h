/*
 * datatypes.h
 *
 *  Created on: 12.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef DATACLASSES_H_
#define DATACLASSES_H_


#include "variant.h"
#include "classes.h"
#include "logger.h"
#include "tabletypes.h"
#include "../config.h"

#ifdef USE_SQLITE3
#  include "sqlite3/sqlite3.h"
#  include "sqlitetypes.h"
#endif

#ifdef USE_POSTGRES
#  include <postgresql/libpq-fe.h>
#  include "postgrestypes.h"
#endif

#ifdef USE_MSSQL_ODBC
#  include <sql.h>
#  include <sqlext.h>
#endif

#include "datatypes.h"

namespace sql {

enum EParameterFormat {
	EPF_BINARY,
	EPF_VARCHAR
};

enum EDatabaseType {
	EDB_UNKNOWN,
	EDB_MSSQL,
	EDB_MYSQL,
	EDB_PGSQL,
	EDB_ORASQL,
	EDB_SQLITE3
};

enum EParameterType {
	EPT_INPUT,
	EPT_OUTPUT,
	EPT_DEFAULT = EPT_INPUT
};

enum EOwnerType {
	EPO_OWNS_OBJECT,
	EPO_SHARED_OPJECT
};


typedef struct CParameterData {
	size_t size;
	TParameterBufferType data;
	EParameterFormat format;
	EParameterType type;
	EOwnerType owner;

	CParameterData() {
		size = 0;
		data = nil;
		owner = EPO_SHARED_OPJECT;
		format = EPF_BINARY;
		type = EPT_INPUT;
	}
} TParameterData;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TParameterList = std::vector<TParameterData*>;

#else

typedef std::vector<TParameterData*> TParameterList;

#endif


class TPersistent : public app::TObject {
protected:
	app::PLogFile infoLog;
	app::PLogFile exceptionLog;

public:
	void writeLog(const std::string& s, const std::string& location = "");
	void errorLog(const std::string& s, const std::string& location = "");
	void setInfoLog(app::PLogFile log) { infoLog = log; };
	void setErrorLog(app::PLogFile log) { exceptionLog = log; };
	app::PLogFile getInfoLog() const { return infoLog; };
	app::PLogFile getErrorLog() const { return exceptionLog; };

	TPersistent();
	virtual ~TPersistent();
};


class TContainer : public TPersistent {
private:
	EDatabaseType type;
	void clear();
	void free();

public:

#ifdef USE_SQLITE3
	sqlite::PDatabase db3;
#endif
#ifdef USE_POSTGRES
	postgres::PDatabase pgdb;
#endif
#ifdef USE_MSSQL_ODBC
	// TODO Implement mssql:
	mssql::PDatabase msdb;
#endif

	EDatabaseType getType() const { return type; };
	void setType(const EDatabaseType type) { this->type = type; };

	bool isValid() const;
	//bool isOpen() const;

	TContainer();
	virtual ~TContainer();
};


class TParameterBuffer {
private:
	TParameterList params;

public:
	void clear();
	bool validIndex(const size_t index) { return util::validListIndex(params, index); };

	TParameterBufferType add(const void* data, const size_t size, const EParameterFormat format, const EParameterType type, const EOwnerType owner);
	TParameterBufferType at(const size_t index);

	TParameterBuffer();
	virtual ~TParameterBuffer();
};


struct THandle {
private:
	void clear() {
		type = EDB_UNKNOWN;
#ifdef USE_SQLITE3
		dbo3 = nil;
#endif
#ifdef USE_POSTGRES
		pgdb = nil;
#endif
#ifdef USE_MSSQL_ODBC
		hdbc = SQL_NULL_HDBC;
#endif
	}

public:
	EDatabaseType type;

#ifdef USE_SQLITE3
	sqlite3* dbo3;
#endif
#ifdef USE_POSTGRES
	PGconn* pgdb;
#endif
#ifdef USE_MSSQL_ODBC
	SQLHDBC hdbc = SQL_NULL_HDBC;
#endif

	THandle() {
		clear();
	}
};

inline std::string dataBaseTypeToStr(const EDatabaseType type) {
	switch (type) {
		case EDB_MSSQL:
			return "Microsoft SQL Server";
		case EDB_MYSQL:
			return "MySQL Database";
		case EDB_PGSQL:
			return "PostgreSQL Database";
		case EDB_ORASQL:
			return "Oracle SQL Database";
		case EDB_SQLITE3:
			return "SQLite3 Database";
			break;
		default:
			break;
	}
	return "<unknown>";
}

} // namespace sql

#endif /* DATACLASSES_H_ */
