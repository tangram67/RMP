/*
 * inifile.h
 *
 *  Created on: 21.01.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef INIFILE_H_
#define INIFILE_H_

#include <iostream>
#include <map>
#include "gcc.h"
#include "hash.h"
#include "classes.h"
#include "variant.h"

namespace app {

class TIniValue;
class TIniFile;
struct CHashIndex;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PIniFile = TIniFile*;
using PIniValue = TIniValue*;
using TIniValues = std::vector<app::PIniValue>;
using TSectionMap = std::map<std::string, size_t>;
using TSectionMapItem = std::pair<std::string, size_t>;
using THashIndex = CHashIndex;

#else

typedef TIniFile* PIniFile;
typedef TIniValue* PIniValue;
typedef std::vector<app::PIniValue> TIniValues;
typedef std::map<std::string, size_t> TSectionMap;
typedef std::pair<std::string, size_t> TSectionMapItem;
typedef CHashIndex THashIndex;

#endif


STATIC_CONST char EQUAL_SIGN[] = " = ";


enum EEntryType {
	INI_NONE, INI_INT, INI_DOUBLE, INI_STRING, INI_PATH, INI_EMPTY, \
	INI_BLYES, INI_BLTRUE, INI_BL01, INI_BLON, INI_BLDEFAULT, \
	INI_COMMENT, INI_SECTION
};


struct CHashIndex : public util::CHashString {
	size_t index;

	void setIndex(const size_t value) { index = value;	}
	size_t getIndex() const { return index; }

	void clear() {
		util::CHashString::clear();
		index = std::string::npos;
	}

	CHashIndex() { clear(); }
	~CHashIndex() {}
};


class TIniValue {
private:
	bool bEmpty;
	bool bSection;

	std::string line;
	size_t index;
	std::string value;
	EEntryType type;

	util::THashString section;
	util::THashString key;

	void parseLine();
	bool extractValue();
	bool extractSection(const size_t& pos = std::string::npos);
	bool extractSection(const std::string::const_iterator& pos);

public:
	void rebuildLine();

	bool isEmpty() const { return bEmpty; }
	void setEmpty(bool value) { bEmpty = value; }
	bool isSection() const { return bSection; }
	void setSection(bool value) { bSection = value; }

	util::hash_type getKeyHash() const { return key.getHash(); }
	util::hash_type getSectionHash() const { return section.getHash(); }
	util::THashKey& getKeyHashKey() { return key.value; }
	util::THashKey& getSectionHashKey() { return section.value; }
	std::string getSection() const { return section.getString(); }
	std::string getKey() const { return key.getString(); }
	std::string getValue() const { return value; }
	void setValue(const std::string& value);
	void setIndex(size_t value) { index = value; }
	size_t getIndex() const { return index; }
	const std::string& getLine() const { return line; }
	EEntryType getType() const { return type; }
	void setType(EEntryType value) { type = value; }

	TIniValue(const std::string& line, const size_t index, const PIniValue anchor);
	virtual ~TIniValue();
};


class TIniFile {
private:
	std::string fileName;
	std::string filePath;
	bool fileExists;

	TIniValues lines;
	THashIndex section;
	TSectionMap sections;

	void clear();
	void readIniFile();
	PIniValue addLineValue(std::string& line, const size_t index, const PIniValue anchor);
	void addSectionMap(const PIniValue o);
	void writeIniFile(const std::string fileName, bool compress);
	size_t findKey(const std::string& key);
	size_t findSection(const std::string& section);
	size_t createSection(const std::string& section);
	std::string createLine(const std::string& Key, const std::string& value);
	void writeItem(const std::string& Key, const std::string& value, const EEntryType type);
	void insertNewLine(const std::string& line, size_t pos, const EEntryType type);
	void addNewLine(const std::string& line, const EEntryType type);
	void addEmptyLine();
	size_t findInsertIndex();
	void rebuildIndex(size_t pos);
	std::string validPath(const std::string& path);
	std::string printf(const std::string &fmt, ...) const;

public:
	typedef TSectionMap::const_iterator const_iterator;

	// Section cursor iterator handling
	const TSectionMap& getSections() const { return sections; };
	const_iterator begin() const { return getSections().begin(); };
	const_iterator end() const { return getSections().end(); };
	bool validCursor(const_iterator& cursor) { return (cursor != end()); }
	
	// General section handling
	void setSection(const std::string& section);
	int deleteSection(const std::string& section);
	int readSection(const std::string& section, util::TVariantValues& values);

	// Ini file content properties
	size_t size() const { return sections.size(); };
	bool empty() const { return size() <= 0; };

	// Boolean string conversion methods
	static bool readBoolValueAndType(std::string value, EEntryType& type);
	static std::string writeBoolValueForType(const bool value, const EEntryType type);

	// Ini value reader
	int readInteger(const std::string& key, int defValue);
	size_t readSize(const std::string& key, size_t defValue);
	uint64_t readHex(const std::string& key, uint64_t defValue);
	bool readBool(const std::string& key, bool defValue);
	double readDouble(const std::string& key, double defValue);
	std::string readString(const std::string& key, const std::string& defValue);
	std::string readPath(const std::string& key, const std::string& defValue);

	// Ini value writer
	void writeInteger(const std::string& key, int value);
	void writeSize(const std::string& key, size_t value);
	void writeHex(const std::string& key, uint64_t value);
	void writeBool(const std::string& key, bool value, const EEntryType type = INI_BLDEFAULT);
	void writeDouble(const std::string& key, double value);
	void writeString(const std::string& key, const std::string& value);
	void writePath(const std::string& key, const std::string& value);

	// Delete entry by key in current section
	bool deleteKey(const std::string& key);

	// General getters
	const std::string& getFilePath() const { return filePath; }
	const std::string& getFileName() const { return fileName; }

	// Write ini file to disk
	void flush(const bool compress = true);
	void open(const std::string& fileName);
	
	// Console output of ini objects
	void debugOutput();

	TIniFile();
	TIniFile(const std::string& fileName);
	virtual ~TIniFile();
};

} /* namespace app */

#endif /* INIFILE_H_ */
