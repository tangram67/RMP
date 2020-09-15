/*
 * webdirectory.cpp
 *
 *  Created on: 30.12.2015
 *      Author: Dirk Brinkmeier
 */

#include "webdirectory.h"
#include "templates.h"
#include "inifile.h"

namespace app {

TWebDirectory::TWebDirectory(const CWebDirectory& data) {
	requested = 0;
	timestamp = util::now();
	std::string s = data.alias;
	util::replace(s, "/", "");
	this->data.alias = s;
	this->data.directory = data.directory;
	this->data.enabled = data.enabled;
	this->data.execCGI = data.execCGI;
	this->data.redirect = data.redirect;
	this->data.scaleJPG = data.scaleJPG;
	this->data.useExif = data.useExif;
}

TWebDirectory::~TWebDirectory() {
}

std::string TWebDirectory::getFileName(const std::string& URL) {
	if (URL.size() > 1) {
		size_t pos = (URL[0] == '/') ? 1 : 0;
		size_t idx = URL.find_first_of('/', pos);

		// Example: URL = /home/index.html
		// idx = 5, pos = 1 --> valid size must be > 6
		// to contain a valid filename in URL!
		if (std::string::npos != idx && URL.size() > (idx + pos)) {
			return data.directory + URL.substr(idx + 1, std::string::npos);
		}

	}
	return "";
}

void TWebDirectory::debugOutput() {
	std::cout << "[" << data.alias << "]" << std::endl;
	std::cout << "  Alias       : " << data.alias << std::endl;
	std::cout << "  Directory   : " << data.directory << std::endl;
	std::cout << "  Redirect    : " << data.redirect.text(',') << std::endl;
	std::cout << "  Rescale JPG : " << data.scaleJPG << std::endl;
	std::cout << "  Execute CGI : " << TIniFile::writeBoolValueForType(data.execCGI, INI_BLYES) << std::endl;
	std::cout << "  Use Exif    : " << TIniFile::writeBoolValueForType(data.useExif, INI_BLYES) << std::endl;
	std::cout << "  Enabled     : " << TIniFile::writeBoolValueForType(data.enabled, INI_BLYES) << std::endl;
}



TWebDirectoryList::TWebDirectoryList() {
}

TWebDirectoryList::~TWebDirectoryList() {
	clear();
}

PWebDirectory TWebDirectoryList::find(const std::string& key)
{
	TWebDirectoryMap::const_iterator it = map.find(key);
	if (it != map.end())
		return it->second;
	return nil;
}

std::string TWebDirectoryList::getKey(const std::string& URL) {
	if (URL.size() > 1) {
		size_t pos = (URL[0] == '/') ? 1 : 0;
		size_t idx = URL.find_first_of('/', pos);
		std::string key;
		if (std::string::npos != idx) {
			key = URL.substr(pos, idx - pos);
		} else {
			// URL is alias only: e.g. /home
			// Trailing slash missing, but valid!
			key = URL.substr(pos, std::string::npos);
		}
		return key;
	}
	return "";
}

std::string TWebDirectoryList::getAlias(const std::string& URL) {
	if (URL.size() > 1) {
		std::string key = getKey(URL);
		if (!key.empty()) {
			PWebDirectory o = find(key);
			if (util::assigned(o)) {
				return o->alias();
			}
		}
	}
	return "";
}

PWebDirectory TWebDirectoryList::getDirectory(const std::string& URL) {
	if (URL.size() > 1) {
		std::string key = getKey(URL);
		if (!key.empty()) {
			PWebDirectory o = find(key);
			if (util::assigned(o)) {
				return o;
			}
		}
	}
	return nil;
}

void TWebDirectoryList::debugOutput() {
	if (!map.empty()) {
		TWebDirectoryMap::const_iterator it = map.begin();
		for (; it != map.end(); ++it) {
			it->second->debugOutput();
			std::cout << std::endl;
		}
	}
}

PWebDirectory TWebDirectoryList::add(const CWebDirectory& data) {
	std::string key = getKey(data.alias);
	if (!key.empty()) {
		PWebDirectory o = find(key);
			if (!util::assigned(o)) {
			o = new TWebDirectory(data);
			map[key] = o;
			return o;
		}
	}
	return nil;
}

void TWebDirectoryList::clear() {
	if (!map.empty()) {
		PWebDirectory o;
		TWebDirectoryMap::const_iterator it = map.begin();
		while (it != map.end()) {
			o = it->second;
			util::freeAndNil(o);
			it++;
		}
		map.clear();
	}
}

} /* namespace app */
