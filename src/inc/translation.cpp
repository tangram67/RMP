/*
 * translate.cpp
 *
 *  Created on: 06.03.2021
 *      Author: dirk
 */

#include "translation.h"

#include "fileutils.h"
#include "exception.h"

namespace app {

TTranslator::TTranslator() {
	prime();
	file.setDebug(debug);
}

TTranslator::~TTranslator() {
	close();
}

void TTranslator::prime() {
	clear();
	locale = ELocale::de_DE;
	enabled = true;
	debug = false;
}

void TTranslator::clear() {
	sectionOK = false;
}

std::string TTranslator::nlsFileName(const ELocale locale) {
	TLanguage language;
	if (app::TLocale::find(locale, language)) {
		 std::string name(language.name);
		 if (!name.empty()) {
			 return path + "nls-" + name + ".dat";
		 }
	}
	return std::string();
}

void TTranslator::reopen() {
	close();
	open();
}

void TTranslator::open() {
	fileName = nlsFileName(locale);
	if (debug) {
		std::cout << "TTranslate::open() Open file \"" << fileName << "\" for locale (" << (int)locale << ") Enabled = " << enabled << std::endl;
	}

	// Do not open any file when disabled...
	if (!enabled) {
		return;
	}

	// Try some fallbacks...
	bool ok = util::fileExists(fileName);
	if (!ok) {
		fileName = nlsFileName(ELocale::de_DE);
		ok = util::fileExists(fileName);
	}
	if (!ok) {
		fileName = nlsFileName(ELocale::en_US);
		ok = util::fileExists(fileName);
	}

	// Create default locale file
	if (!ok) {
		fileName = nlsFileName(ELocale::de_DE);
	}

	// Read language entries for given locale
	if (debug) {
		std::cout << "TTranslate::open() Open file <" << fileName << ">" << std::endl;
	}
	file.open(fileName);
	if (debug) {
		file.debugOutput();
	}
	setSection();
}

void TTranslator::close() {
	clear();
	if (!fileName.empty()) {
		if (debug) {
			std::cout << "TTranslate::close() Close file <" << fileName << ">" << std::endl;
			file.debugOutput();
		}
		if (enabled) {
			file.sort("Text");
			file.flush();
		}
		fileName.clear();
	}
	file.close();
}

const std::string& TTranslator::getFileName() const {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
	return fileName;
};

void TTranslator::setSection() {
	if (!sectionOK) {
		file.setSection("Text");
		sectionOK = true;
	}
}

void TTranslator::setEnabled(const bool value) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
	enabled = value;
	if (debug) {
		std::cout << "TTranslate::setEnabled() Enabled = " << enabled << std::endl;
	}
};

void TTranslator::setPath(const std::string& configPath) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
	if (util::folderExists(configPath)) {
		path = util::validPath(configPath);
	} else {
		throw util::app_error_fmt("TTranslate::setPath() Path $ does not exists.", configPath);
	}
}

void TTranslator::setLanguage(const ELocale locale) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
	this->locale = locale;
	if (debug) {
		std::cout << "TTranslate::setLanguage() Set locale (" << (int)this->locale << ")" << std::endl;
	}
	reopen();
}

std::string TTranslator::text(size_t id, const std::string& defValue) {
	if (enabled) {
		return text(std::to_string((size_u)id), defValue);
	}
	return defValue;
}

std::string TTranslator::text(const std::string id, const std::string& defValue) {
	if (enabled) {
		bool create = false;
		std::string s;
		if (!id.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_READ);
			if (!file.readText(id, s)) {
				s = defValue;
				create = true;
			}
		}
		if (create) {
			// Text was not yet found in file
			// --> Create new text entry with given default value
			app::TReadWriteGuard<app::TReadWriteLock> lock(rwl, RWL_WRITE);
			setSection();
			file.writeString(id, s);
			if (debug) std::cout << "TTranslate::text() Write [" << id << "] = \"" << s << "\"" << std::endl;
		}
		return s;
	}
	return defValue;
}

} /* namespace app */
