/*
 * webdirectory.h
 *
 *  Created on: 30.12.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBDIRECTORY_H_
#define WEBDIRECTORY_H_

#include <map>
#include <string>
#include "stringutils.h"
#include "semaphores.h"
#include "datetime.h"
#include "gcc.h"

namespace app {

class TWebDirectory;
class TWebDirectoryList;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PWebDirectory = TWebDirectory*;
using PWebDirectoryList = TWebDirectoryList*;
using TWebDirectoryMap = std::map<std::string, app::PWebDirectory>;
using TWebDirectoryMapItem = std::pair<std::string, app::PWebDirectory>;

#else

typedef TWebDirectory* PWebDirectory;
typedef TWebDirectoryList* PWebDirectoryList;
typedef std::map<std::string, app::PWebDirectory> TWebDirectoryMap;
typedef std::pair<std::string, app::PWebDirectory> TWebDirectoryMapItem;

#endif


struct CWebDirectory {
	std::string directory;
	std::string alias;
	util::TStringList redirect;
	bool enabled;
	bool execCGI;
	int scaleJPG;
	bool useExif;

	void clear() {
		if (!directory.empty())
			directory.clear();
		if (!alias.empty())
			alias.clear();
		if (!redirect.empty())
			redirect.clear();
		enabled = false;
		execCGI = false;
		useExif = false;
		scaleJPG = 0;
	}

	CWebDirectory() {
		clear();
	}
};

class TWebDirectory {
	CWebDirectory data;
	app::TMutex localMtx;
	mutable size_t requested;
	mutable util::TTimePart timestamp;

public:
	const std::string& directory() const { return data.directory; };
	const std::string& alias() const { return data.alias; };
	const util::TStringList& redirect() const { return data.redirect; };
	bool enabled() const { return data.enabled; };
	bool execCGI() const { return data.execCGI; };
	int  scaleJPG() const { return data.scaleJPG; };
	bool useExif() const { return data.useExif; };
	size_t getRequested() const { return requested; };
	void incRequested() const { requested++; timestamp = util::now(); };
	util::TTimePart getTimeStamp() const { return timestamp; };
	app::TMutex& getMutex() { return localMtx; };

	std::string getFileName(const std::string& URL);
	void debugOutput();

	TWebDirectory(const CWebDirectory& data);
	virtual ~TWebDirectory();
};


class TWebDirectoryList {
	TWebDirectoryMap map;

public:
	typedef TWebDirectoryMap::const_iterator const_iterator;

	size_t size() const { return map.size(); };
	bool empty() const { return map.empty(); };

	const_iterator begin() const { return map.begin(); };
	const_iterator end() const { return map.end(); };

	PWebDirectory add(const CWebDirectory& data);
	void clear();

	PWebDirectory getDirectory(const std::string& URL);
	std::string getAlias(const std::string& URL);
	std::string getKey(const std::string& URL);
	PWebDirectory find(const std::string& key);
	PWebDirectory operator[] (const std::string& key) { return find(key); };

	void debugOutput();

	TWebDirectoryList();
	virtual ~TWebDirectoryList();
};

} /* namespace app */

#endif /* WEBDIRECTORY_H_ */
