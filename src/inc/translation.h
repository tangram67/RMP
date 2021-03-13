/*
 * translate.h
 *
 *  Created on: 06.03.2021
 *      Author: dirk
 */

#ifndef SRC_INC_TRANSLATION_H_
#define SRC_INC_TRANSLATION_H_

#include <string>
#include "inifile.h"
#include "semaphores.h"
#include "localizations.h"

namespace app {

class TTranslator;
#ifdef STL_HAS_TEMPLATE_ALIAS
using PTranslator = TTranslator*;
#else
typedef TTranslator* PTranslator;
#endif

class TTranslator {
	mutable TReadWriteLock rwl;
	std::string fileName;
	std::string path;
	ELocale locale;
	TIniFile file;
	bool sectionOK;
	bool enabled;
	bool debug;

	void prime();
	void clear();

	void reopen();
	void open();
	void close();

	void setSection();
	std::string nlsFileName(const ELocale locale);

public:
	bool getEnabled() const { return enabled; };
	void setEnabled(const bool value);

	void setPath(const std::string& configPath);
	void setLanguage(const ELocale locale);
	const std::string& getFileName() const;

	std::string text(size_t id, const std::string& defValue);
	std::string text(const std::string id, const std::string& defValue);

	std::string operator () (size_t id, const std::string& defValue) { return text(id, defValue); };
	std::string operator () (const std::string id, const std::string& defValue) { return text(id, defValue); };

	TTranslator();
	virtual ~TTranslator();
};

} /* namespace app */

#endif /* SRC_INC_TRANSLATION_H_ */
