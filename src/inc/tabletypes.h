/*
 * tabletypes.h
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef TABLETYPES_H_
#define TABLETYPES_H_

#include "gcc.h"
#include "vartypes.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace data {

class TTable;
class TRecord;
class TField;
class THeader;
class TColumn;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PColumn = TColumn*;
using PHeader = THeader*;
using PField = TField*;
using PTable = TTable*;
using PRecord = TRecord*;
using TFieldList = std::vector<data::PField>;
using TFieldMap = std::unordered_map<std::string, data::PField>;
using TRecordList = std::vector<data::PRecord>;
using TColumnList = std::vector<data::PColumn>;
using TColumnMap = std::unordered_map<std::string, data::PColumn>;
using TPrimeMap = std::unordered_map<std::string, data::PRecord>;
using TPrimeItem = std::pair<std::string, data::PRecord>;

#else

typedef TColumn* PColumn;
typedef THeader* PHeader;
typedef TField* PField;
typedef TTable* PTable;
typedef TRecord* PRecord;
typedef std::vector<data::PField> TFieldList;
typedef std::unordered_map<std::string, data::PField> TFieldMap;
typedef std::vector<data::PRecord> TRecordList;
typedef std::vector<data::PColumn> TColumnList;
typedef std::unordered_map<std::string, data::PColumn> TColumnMap;
typedef std::unordered_map<std::string, data::PRecord> TPrimeMap;
typedef std::pair<std::string, data::PRecord> TPrimeItem;

#endif


enum EFieldType {
	SFT_UNKNOWN = 0,
	SFT_INTEGER8 = 1,
	SFT_INTEGER16 = 2,
	SFT_INTEGER32 = 3,
	SFT_INTEGER64 = 4,
	SFT_UNSIGNED8 = 8,
	SFT_UNSIGNED16 = 9,
	SFT_UNSIGNED32 = 10,
	SFT_UNSIGNED64 = 11,
	SFT_DOUBLE = 16,
	SFT_VARCHAR = 32,
	SFT_WIDECHAR = 33,
	SFT_BLOB = 64,
	SFT_BLOB64 = 65,
	SFT_DATETIME = 128,
	SFT_BOOLEAN = 232,
	SFT_NULL = 254
};


inline std::string fieldTypeAsString(const EFieldType type) {
	switch (type)
	{
		case SFT_INTEGER8:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int8_t))) + " Bit)";
		case SFT_INTEGER16:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int16_t))) + " Bit)";
		case SFT_INTEGER32:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int32_t))) + " Bit)";
		case SFT_INTEGER64:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int64_t))) + " Bit)";
		case SFT_UNSIGNED8:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint8_t))) + " Bit)";
		case SFT_UNSIGNED16:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint16_t))) + " Bit)";
		case SFT_UNSIGNED32:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint32_t))) + " Bit)";
		case SFT_UNSIGNED64:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint64_t))) + " Bit)";
		case SFT_DOUBLE:
			return "Numeric Value (" + std::to_string((size_u)(8*sizeof(double))) + " Bit)";
		case SFT_VARCHAR:
			return "ASCII UTF-8 String";
		case SFT_WIDECHAR:
			return "Wide UTF-16 String";
		case SFT_BLOB:
			return "Binary large object";
		case SFT_BLOB64:
			return "Binary large object (64 Bit)";
		case SFT_DATETIME:
			return "Date time value";
		case SFT_BOOLEAN:
			return "Boolean value (" + std::to_string((size_u)(8*sizeof(bool))) + " Bit)";
		case SFT_NULL:
			return "NULL";
		default:
			break;
	}
	return "Unknown";
}


inline data::EFieldType variantTypeToFieldType(const util::EVariantType type) {
	data::EFieldType retVal = data::SFT_UNKNOWN;

	// Convert variant types to enumerated SQL field types
	switch (type) {
		case util::EVT_INTEGER8:
			retVal = data::SFT_INTEGER8;
			break;
		case util::EVT_INTEGER16:
			retVal = data::SFT_INTEGER16;
			break;
		case util::EVT_INTEGER32:
			retVal = data::SFT_INTEGER32;
			break;
		case util::EVT_INTEGER64:
			retVal = data::SFT_INTEGER64;
			break;
		case util::EVT_UNSIGNED8:
			retVal = data::SFT_UNSIGNED8;
			break;
		case util::EVT_UNSIGNED16:
			retVal = data::SFT_UNSIGNED16;
			break;
		case util::EVT_UNSIGNED32:
			retVal = data::SFT_UNSIGNED32;
			break;
		case util::EVT_UNSIGNED64:
			retVal = data::SFT_UNSIGNED64;
			break;
		case util::EVT_BOOLEAN:
			retVal = data::SFT_BOOLEAN;
			break;
		case util::EVT_STRING:
			retVal = data::SFT_VARCHAR;
			break;
		case util::EVT_WIDE_STRING:
			retVal = data::SFT_WIDECHAR;
			break;
		case util::EVT_DOUBLE:
			retVal = data::SFT_DOUBLE;
			break;
		case util::EVT_TIME:
			retVal = data::SFT_DATETIME;
			break;
		case util::EVT_BLOB:
			retVal = data::SFT_BLOB;
			break;
		case util::EVT_NULL:
			retVal = data::SFT_NULL;
			break;
		default:
			break;
	}

	return retVal;
}

inline void variantTypeToFieldType(const util::EVariantType type, data::EFieldType& field, size_t& size) {
	field = data::SFT_UNKNOWN;

	// 0 = indeterminable
	// --> depends on server or client implementation or given by the data size itsel (e.g. varchar)
	size = 0;

	// Convert variant types to enumerated SQL field types
	switch (type) {
		case util::EVT_INTEGER8:
			field = data::SFT_INTEGER8;
			size = sizeof(int8_t);
			break;
		case util::EVT_INTEGER16:
			field = data::SFT_INTEGER16;
			size = sizeof(int16_t);
			break;
		case util::EVT_INTEGER32:
			field = data::SFT_INTEGER32;
			size = sizeof(int32_t);
			break;
		case util::EVT_INTEGER64:
			field = data::SFT_INTEGER64;
			size = sizeof(int64_t);
			break;
		case util::EVT_UNSIGNED8:
			field = data::SFT_UNSIGNED8;
			size = sizeof(uint8_t);
			break;
		case util::EVT_UNSIGNED16:
			field = data::SFT_UNSIGNED16;
			size = sizeof(uint16_t);
			break;
		case util::EVT_UNSIGNED32:
			field = data::SFT_UNSIGNED32;
			size = sizeof(uint32_t);
			break;
		case util::EVT_UNSIGNED64:
			field = data::SFT_UNSIGNED64;
			size = sizeof(uint64_t);
			break;
		case util::EVT_BOOLEAN:
			field = data::SFT_BOOLEAN;
			size = 0;
			break;
		case util::EVT_STRING:
			field = data::SFT_VARCHAR;
			size = 0;
			break;
		case util::EVT_WIDE_STRING:
			field = data::SFT_WIDECHAR;
			size = 0;
			break;
		case util::EVT_DOUBLE:
			field = data::SFT_DOUBLE;
			size = 0;
			break;
		case util::EVT_TIME:
			field = data::SFT_DATETIME;
			size = 0;
			break;
		case util::EVT_BLOB:
			field = data::SFT_BLOB;
			size = 0;
			break;
		case util::EVT_NULL:
			field = data::SFT_NULL;
			size = 0;
			break;
		default:
			break;
	}
}

} // namespace data

#endif /* TABLETYPES_H_ */
