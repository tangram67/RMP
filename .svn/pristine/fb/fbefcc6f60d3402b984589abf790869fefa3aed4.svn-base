/*
 * webtoken.h
 *
 *  Created on: 14.03.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBTOKEN_H_
#define WEBTOKEN_H_

#include "gcc.h"
#include "parser.h"
#include "classes.h"
#include "variant.h"
#include "fileutils.h"
#include <string>
#include <vector>
#include <map>


namespace app {


class TWebToken;
class TWebTokenList;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PWebToken = TWebToken*;
using PWebTokenList = TWebTokenList*;
using TWebTokenMap = std::map<std::string, app::PWebToken>;
using TWebTokenMapItem = std::pair<std::string, app::PWebToken>;
using TWebTokenValueMap = std::map<std::string, std::string>;
using TWebTokenValueItem = std::pair<std::string, std::string>;

#else

typedef TWebToken* PWebToken;
typedef TWebTokenList* PWebTokenList;
typedef std::map<std::string, app::PWebToken> TWebTokenMap;
typedef std::pair<std::string, app::PWebToken> TWebTokenMapItem;
typedef std::map<std::string, std::string> TWebTokenValueMap;
typedef std::pair<std::string, std::string> TWebTokenValueItem;

#endif


class TWebToken {
friend class TWebTokenList;
private:
	mutable std::mutex valueMtx;
	mutable std::mutex listMtx;

	std::string m_key;
	std::string m_value;
	util::TVariant m_variant;

	util::TFolderContent files;

	void init();
	void addFile(const util::PFile file);
	void update(bool invalidate = true) const;
	void onChange(const util::TVariant& variant);

public:
	void clear();
	void invalidate() const;

	// Do not destroy assigned web page buffer by default when value set
	void setValue(const char* value, bool invalidate = false);
	void setValue(const std::string& value, bool invalidate = false);
	const std::string& getValue() const;

	void setKey(const std::string& key);
	const std::string& getKey() const;

	/*
	 * Examples for use of operator "="
	 *
	 * TWebToken token("key");
	 * token = "1234";
	 * token = 1234;
	 * token = 1.234;
	 *
	 * PWebToken token = new TWebToken("key");
	 * *token = "1234";
	 * *token = 1234;
	 * *token = 1.234;
	 */
	TWebToken& operator = (const char* value) {
		setValue(value, false);
		return *this;
	}

	TWebToken& operator = (const std::string& value) {
		setValue(value, false);
		return *this;
	}

	TWebToken& operator = (std::string& value) {
		setValue(value, false);
		return *this;
	}

	template<typename type_t>
		TWebToken& operator = (type_t&& value) {
			std::lock_guard<std::mutex> lock(valueMtx);
			m_variant = value;
			return *this;
		}

	TWebToken();
	TWebToken(const std::string& key);
	virtual ~TWebToken();
};


class TWebTokenList {
private:
	TWebTokenMap tokenMap;
	util::PFileList fileList;

public:
	void clear();
	void invalidate();
	void update();

	void getTokenValues(TWebTokenValueMap& values);
	void setTokenValues(const TWebTokenValueMap& values);

	PWebToken addToken(std::string key);
	PWebToken getToken(std::string key);
	void setFiles(util::TFileList& fileList);

	bool empty() const { return tokenMap.empty(); }
	size_t size() const { return tokenMap.size(); }

	TWebTokenList();
	TWebTokenList(util::TFileList& fileList);
	virtual ~TWebTokenList();
};



} /* namespace app */

#endif /* WEBTOKEN_H_ */
