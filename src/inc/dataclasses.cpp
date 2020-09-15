/*
 * datatypes.cpp
 *
 *  Created on: 24.07.2015
 *      Author: Dirk Brinkmeier
 */

#include "dataclasses.h"
#include "typeid.h"
#include "gcc.h"

namespace sql {


TPersistent::TPersistent() : app::TObject() {
	infoLog = nil;
	exceptionLog = nil;
}

TPersistent::~TPersistent() {
}

void TPersistent::writeLog(const std::string& s, const std::string& location) {
	if (name.empty())
		name = util::nameOf(this);
	if (util::assigned(infoLog)) {
		if (location.empty())
			infoLog->write("[" + name + "] " + s);
		else
			infoLog->write("[" + location + "] [" + name + "] " + s);
	}
}

void TPersistent::errorLog(const std::string& s, const std::string& location) {
	if (name.empty())
		name = util::nameOf(this);
	if (util::assigned(exceptionLog)) {
		if (location.empty())
			exceptionLog->write("[" + name + "] " + s);
		else
			exceptionLog->write("[" + location + "] [" + name + "] " + s);
	}
	if (util::assigned(infoLog)) {
		if (location.empty())
			infoLog->write("[" + name + "] " + s);
		else
			infoLog->write("[" + location + "] [" + name + "] " + s);
	}
}



TContainer::TContainer() : TPersistent() {
	clear();
}

TContainer::~TContainer() {
}

void TContainer::clear() {
	type = EDB_UNKNOWN;
#ifdef USE_SQLITE3
	db3 = nil;
#endif
#ifdef USE_MSSQL_ODBC
	msdb = nil;
#endif
#ifdef USE_POSTGRES
	pgdb = nil;
#endif
}

bool TContainer::isValid() const {
	bool retVal = false;
	switch (type) {
#ifdef USE_SQLITE3
		case EDB_SQLITE3:
			retVal = util::assigned(db3);
			break;
#endif
#ifdef USE_POSTGRES
		case EDB_PGSQL:
			retVal = util::assigned(pgdb);
			break;
#endif
		default:
			throw util::app_error("TContainer::isValid() : Database type " + dataBaseTypeToStr(type) + " not supported.");
			break;
	}
	return retVal;
}

//bool TContainer::isOpen() const {
//	switch (type) {
//#ifdef USE_SQLITE3
//		case EDB_SQLITE3:
//			if (util::assigned(db3))
//				return db3->isOpen();
//			break;
//#endif
//#ifdef USE_POSTGRES
//		case EDB_PGSQL:
//			if (util::assigned(db3))
//				return pgdb->isOpen();
//			break;
//#endif
//		default:
//			throw util::app_error("TContainer::isOpen() : Database type " + dataBaseTypeToStr(type) + " not supported.");
//			break;
//	}
//	return false;
//}



TParameterBuffer::TParameterBuffer() {
}

TParameterBuffer::~TParameterBuffer() {
	clear();
}

TParameterBufferType TParameterBuffer::add(const void* data, const size_t size, const EParameterFormat format, const EParameterType type, const EOwnerType owner) {
	TParameterBufferType p = nil;
	if (util::assigned(data) && size > 0) {
		TParameterData* o = nil;
		o = new TParameterData;
		o->size = size;
		o->format = format;
		o->type = type;
		o->owner = owner;
		if (util::assigned(data) && size > 0) {
			if (owner == EPO_OWNS_OBJECT) {
				// Set data to given owner data pointer
				o->data = (TParameterBufferType)data;
			} else {
				// Copy data for shared object (always append a trailing '\0')
				o->data = new TParameterOrdinalType[size+1];
				memcpy(o->data, data, size);
				(o->data)[size] = 0;
			}
		} else {
			// Add NULL object
			o->data = nil;
		}
		params.push_back(o);
		p = o->data;
	}
	return p;
}

TParameterBufferType TParameterBuffer::at(const size_t index) {
	TParameterBufferType p = nil;
	if (validIndex(index)) {
		TParameterData* o = params.at(index);
		if (util::assigned(o)) {
			p = o->data;
		}
	}
	return p;
}

void TParameterBuffer::clear() {
	if (!params.empty()) {
#ifndef STL_HAS_RANGE_FOR
		TParameterData* o;
		size_t i,n;
		n = params.size();
		for (i=0; i<n; i++) {
			o = params[i];
			if (util::assigned(o)) {
				if (o->owner == EPO_SHARED_OPJECT) {
					TParameterBufferType data = o->data;
					if (util::assigned(data)) {
						delete[] data;
						data = nil;
					}
					util::freeAndNil(o);
				}
			}
		}
#else
		for (auto o : params) {
			if (util::assigned(o)) {
				if (o->owner == EPO_SHARED_OPJECT) {
					TParameterBufferType data = o->data;
					if (util::assigned(data)) {
						delete[] data;
						data = nil;
					}
					util::freeAndNil(o);
				}
			}
		}
#endif
		params.clear();
	}
}

} // namespace sql
