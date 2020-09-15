/*
 * credentials.h
 *
 *  Created on: 03.12.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef CREDENTIALS_H_
#define CREDENTIALS_H_

#include <string>
#include "semaphores.h"
#include "datatypes.h"
#include "logtypes.h"
#include "webtypes.h"
#include "stringutils.h"
#include "credentialtypes.h"

namespace app {

static const std::string CREDENTIAL_TABLE_NAME = "T_Credentials2";

class TCredentials {
private:
	enum EOutputFormat { EOF_HTML, EOF_JSON };

	mutable app::TMutex mtx;
	mutable std::string html;
	mutable std::string json;
	std::string realm;
	EHttpAuthType auth;
	app::PLogFile logger;
	sql::PContainer database;
	TCredentialMap users;
	std::string defVal;

	void prime();
	void invalidate();
	void writeLog(const std::string& text) const;

	const std::string& getHashWithNolock(const std::string username);
	const std::string& getDigestWithNolock(const std::string username);

	bool createCredentialTable();
	size_t hasCredentialTable();
	size_t hasCredentialTableWithNolock();
	size_t readCredentialTable(util::TStringList& list, const std::string filter, const EOutputFormat format) const;
	size_t readUserCredentials();

	size_t hasUserName(const std::string username);
	bool insertUserRecord(const TCredential& user);
	bool updateUserRecord(const TCredential& user);
	bool deleteUserRecord(const std::string& username);

public:
	typedef TCredentialMap::const_iterator const_iterator;

	const_iterator begin() const { return users.begin(); };
	const_iterator end() const { return users.end(); };

	void setDigestType(const EHttpAuthType type) { auth = type; };
	EHttpAuthType getDigestType() const { return auth; };

	static std::string getPriviledge(int level);
	static int getUserLevel(const std::string& priviledge);

	std::string getRealm() const { return realm; };
	void setRealm(const std::string& value) { realm = value; };

	size_t size() const;
	std::string asHTML(const std::string filter = "") const;
	std::string asJSON(const std::string filter = "") const;

	void getCredentialMap(TCredentialMap& users) const;
	bool getUserCredentials(const std::string username, TCredential& credential) const;

	static bool hash(TCredential& user, const EHttpAuthType algorithm, const std::string& realm);

	const std::string& getHash(const std::string username);
	const std::string& getDigest(const std::string username);

	bool insert(TCredential& user);
	bool modify(TCredential& user);
	bool remove(const std::string& username);

	void setDatabase(sql::PContainer database);
	void setLogger(app::PLogFile logger);

	bool initialize();
	void debugOutput(const std::string& preamble);

	TCredentials();
	TCredentials(sql::PContainer database);
	virtual ~TCredentials();
};

} /* namespace app */

#endif /* CREDENTIALS_H_ */
