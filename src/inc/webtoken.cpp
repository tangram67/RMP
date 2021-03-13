/*
 * webtoken.cpp
 *
 *  Created on: 14.03.2015
 *      Author: Dirk Brinkmeier
 */

#include "webtoken.h"
#include "datetime.h"


namespace app {


TWebToken::TWebToken() {
	init();
}

TWebToken::TWebToken(const std::string& key) {
	init();
	m_key = key;
}

TWebToken::~TWebToken() {
	clear();
}

void TWebToken::init() {
	m_variant.bindOnChanged(&app::TWebToken::onChange, this);
}

void TWebToken::addFile(const util::PFile file) {
	std::lock_guard<std::mutex> lock(listMtx);
	files.push_back(file);
}

void TWebToken::setValue(const char* value, bool invalidate) {
	std::lock_guard<std::mutex> lock(valueMtx);
	m_value = value;
	update(invalidate);
}

void TWebToken::setValue(const std::string& value, bool invalidate) {
	std::lock_guard<std::mutex> lock(valueMtx);
	m_value = value;
	update(invalidate);
}

const std::string& TWebToken::getValue() const {
	std::lock_guard<std::mutex> lock(valueMtx);
	return m_value;
}

void TWebToken::setKey(const std::string& key) {
	std::lock_guard<std::mutex> lock(valueMtx);
	m_key = key;
}

const std::string& TWebToken::getKey() const {
	std::lock_guard<std::mutex> lock(valueMtx);
	return m_key;
}

void TWebToken::update(bool invalidate) const {
	std::lock_guard<std::mutex> lock(listMtx);
	if (!m_key.empty()) {
		util::PFile o;
		for (size_t i=0; i<files.size(); i++) {
			o = files[i];
			if (util::assigned(o)) {
				if (o->hasParser())
					o->getParser()->setTokenValue(m_key, m_value, invalidate);
			}
		}
	}
}

void TWebToken::invalidate() const {
	std::lock_guard<std::mutex> lock(listMtx);
	if (!m_key.empty()) {
		util::PFile o;
		for (size_t i=0; i<files.size(); i++) {
			o = files[i];
			if (util::assigned(o)) {
				if (o->hasParser())
					o->getParser()->invalidate();
			}
		}
	}
}

void TWebToken::onChange(const util::TVariant& variant) {
	// Do NOT lock here, operator "=" has done that before
	// std::lock_guard<std::mutex> lock(valueMtx);
	m_value = variant.asString();
	update(false);
}

void TWebToken::clear() {
	std::lock_guard<std::mutex> lock(listMtx);
	files.clear();
}





TWebTokenList::TWebTokenList() {
	fileList = nil;
}

TWebTokenList::TWebTokenList(util::TFileList& fileList) {
	setFiles(fileList);
}

void TWebTokenList::setFiles(util::TFileList& fileList) {
	this->fileList = &fileList;
}

TWebTokenList::~TWebTokenList() {
	clear();
}


void TWebTokenList::clear() {
	TWebTokenMap::const_iterator it = tokenMap.begin();
	PWebToken o;
	while (it != tokenMap.end()) {
		// Short: freeAndNil(it++->second);
		o = it->second;
		util::freeAndNil(o);
		++it;
	}
	tokenMap.clear();
}

void TWebTokenList::invalidate() {
	TWebTokenMap::const_iterator it = tokenMap.begin();
	PWebToken o;
	while (it != tokenMap.end()) {
		// Short: freeAndNil(it++->second);
		o = it->second;
		if (util::assigned(o))
			o->clear();
		++it;
	}
}


PWebToken TWebTokenList::addToken(std::string key) {
	if (!util::assigned(fileList))
		throw util::app_error("TWebTokenList::addToken(): No file list to add web token.");
	PWebToken token = nil;
	if (!fileList->empty()) {
		util::PFile file;
		util::PTokenParser parser;
		util::PFileMap fm = fileList->getFiles();
		util::TFileMap::const_iterator it = fm->begin();
		util::TFileMap::const_iterator end = fm->end();
		while (it != end) {
			file = it->second;
			if (util::assigned(file)) {
				if (file->hasParser()) {
					parser = file->getParser();
					if (parser->hasToken()) {
						// Check if file has token key entry
						if (util::assigned(token) || util::assigned(parser->getToken(key))) {
							if (!util::assigned(token))
								token = new TWebToken(key);
							token->addFile(file);
						}
					}
				}
			}
			it++;
		}
	}
	if (util::assigned(token)) {
		tokenMap.insert(TWebTokenMapItem(key, token));
		token->invalidate();
	}
	return token;
}

bool TWebTokenList::hasToken(std::string key) const {
	TWebTokenMap::const_iterator it = tokenMap.find(key);
	if (it != tokenMap.end())
		return true;
	return false;
}

PWebToken TWebTokenList::getToken(std::string key) const {
	PWebToken token = nil;
	TWebTokenMap::const_iterator it = tokenMap.find(key);
	if (it != tokenMap.end())
		token = it->second;
	return token;
}


void TWebTokenList::getTokenValues(TWebTokenValueMap& values) {
	values.clear();
	PWebToken token;
	TWebTokenMap::const_iterator wt = tokenMap.begin();
	while (wt != tokenMap.end()) {
		token = wt->second;
		if (util::assigned(token)) {
			const std::string& key = token->getKey();
			const std::string& value = token->getValue();
			if (!key.empty()) {
				values.insert(TWebTokenValueItem(key, value));
			}
		}
		++wt;
	}
}

void TWebTokenList::setTokenValues(const TWebTokenValueMap& values) {
	if (!values.empty()) {
		PWebToken token;
		TWebTokenValueMap::const_iterator vm = values.begin();
		while (vm != values.end()) {
			const std::string& key = vm->first;
			const std::string& value = vm->second;
			if (!key.empty()) {
				TWebTokenMap::const_iterator wt = tokenMap.find(key);
				if (wt != tokenMap.end()) {
					token = wt->second;
					if (util::assigned(token)) {
						token->setValue(value, true);
					}
				}
			}
			++vm;
		}
	}
}


void TWebTokenList::update() {
	if (!tokenMap.empty()) {
		PWebToken token;
		util::PToken fileToken;
		util::PFile file;

		// Iterate through all created web token by application
		TWebTokenMap::const_iterator wt = tokenMap.begin();
		while (wt != tokenMap.end()) {
			token = wt->second;
			if (util::assigned(token)) {

				// Clear vector list of all assigned files for web token
				token->files.clear();

				if (!fileList->empty()) {

					// Iterate through all files to find key of web token
					util::PFileMap fm = fileList->getFiles();
					util::TFileMap::const_iterator it = fm->begin();
					util::TFileMap::const_iterator end = fm->end();
					while (it != end) {
						file = it->second;
						if (util::assigned(file)) {

							// Reassign file containing key to web token
							if (file->hasParser()) {
								fileToken = file->getParser()->getToken(token->getKey());
								if (util::assigned(fileToken)) {
									token->addFile(file);
									//std::cout << "TWebTokenList::update() Add file \"" << file->getName() << "\" to web token [\"" << webToken->getKey() << "\":\"" << webToken->getValue() << "\"]" << std::endl;
								}
							}
						}
						++it;
					}
				}
			}
			++wt;
		}
	}
}


} /* namespace app */
