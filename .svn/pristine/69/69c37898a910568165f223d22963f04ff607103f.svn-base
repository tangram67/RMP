/*
 * credentials.cpp
 *
 *  Created on: 03.12.2017
 *      Author: Dirk Brinkmeier
 */

#include "credentials.h"
#include "compare.h"
#include "nullptr.h"
#include "sqlite.h"
#include "ssl.h"

namespace app {

TCredentials::TCredentials() {
	database = nil;
	prime();
}

TCredentials::TCredentials(sql::PContainer database) : database(database) {
	prime();
}

TCredentials::~TCredentials() {
};


void TCredentials::prime() {
	auth = HAT_DIGEST_MD5;
	logger = nil;
}


void TCredentials::setDatabase(sql::PContainer database) {
	if (util::assigned(database)) {
		this->database = database;
	}
}

void TCredentials::setLogger(app::PLogFile logFile) {
	if (util::assigned(logFile)) {
		logger = logFile;
	}
}

void TCredentials::writeLog(const std::string& text) const {
	if (util::assigned(logger)) {
		logger->write(text);
	}
}


bool TCredentials::initialize() {
	app::TLockGuard<app::TMutex> lock(mtx);
	size_t r = hasCredentialTableWithNolock();
	if (r == std::string::npos) {
		return false;
	}
	if (r <= 0) {
		return createCredentialTable();
	}
	if (r > 0) {
		readUserCredentials();
		return true;
	}
	return false;
}

void TCredentials::invalidate() {
	json.clear();
	html.clear();
}

std::string TCredentials::getPriviledge(int level) {
	switch (level) {
		case 1:
			return "User";
			break;
		case 2:
			return "Advanced";
			break;
		case 3:
			return "Administrator";
			break;
	}
	return "Invalid";
}

int TCredentials::getUserLevel(const std::string& priviledge) {
	if (0 == util::strcasecmp("Administrator", priviledge)) {
		return 3;
	}
	if (0 == util::strcasecmp("Advanced", priviledge)) {
		return 2;
	}
	if (0 == util::strcasecmp("User", priviledge)) {
		return 1;
	}
	return 0;
}

const std::string& TCredentials::getHashWithNolock(const std::string username) {
	const_iterator it = users.find(username);
	if (it != end()) {
		switch (auth) {
			case HAT_DIGEST_MD5:
				return it->second.hashMD5;
				break;
			case HAT_DIGEST_SHA256:
				return it->second.hashSHA256;
				break;
			case HAT_DIGEST_SHA512:
				return it->second.hashSHA512;
				break;
			default:
				break;
		}
	}
	return defVal;
}

const std::string& TCredentials::getDigestWithNolock(const std::string username) {
	const_iterator it = users.find(username);
	if (it != end()) {
		switch (auth) {
			case HAT_DIGEST_MD5:
				return it->second.digestMD5;
				break;
			case HAT_DIGEST_SHA256:
				return it->second.digestSHA256;
				break;
			case HAT_DIGEST_SHA512:
				return it->second.digestSHA512;
				break;
			default:
				break;
		}
	}
	return defVal;
}

const std::string& TCredentials::getHash(const std::string username) {
	app::TLockGuard<app::TMutex> lock(mtx);
	return getHashWithNolock(username);
}

const std::string& TCredentials::getDigest(const std::string username) {
	app::TLockGuard<app::TMutex> lock(mtx);
	return getDigestWithNolock(username);
}


size_t TCredentials::hasCredentialTableWithNolock() {
	if (!util::assigned(database))
		return std::string::npos;

	size_t retVal = 0;
	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("SELECT count(*)");
	query.SQL.add("FROM sqlite_master");
	query.SQL.add("WHERE type = 'table'");
	query.SQL.add("AND name = '" + CREDENTIAL_TABLE_NAME + "'");

	// Read first row and first column as result for (filtered) row count of table
	try {
		dbguard.prepare();
		if (query.first()) {
			retVal = query[0].asInteger64();
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::hasCredentialTable()\n" + sExcept;
		writeLog(sText);
		retVal = std::string::npos;
	}

	return retVal;
}

size_t TCredentials::hasCredentialTable() {
	app::TLockGuard<app::TMutex> lock(mtx);
	return hasCredentialTableWithNolock();
}

bool TCredentials::createCredentialTable() {
	bool retVal = false;

	if (!util::assigned(database))
		return retVal;

	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("CREATE TABLE IF NOT EXISTS " + CREDENTIAL_TABLE_NAME + " (");
	query.SQL.add("  C_Username      TEXT PRIMARY KEY NOT NULL,");
	query.SQL.add("  C_Lastname      TEXT NOT NULL,");
	query.SQL.add("  C_Givenname     TEXT NOT NULL,");
	query.SQL.add("  C_Password      TEXT NOT NULL,");
	query.SQL.add("  C_Description   TEXT,");
	query.SQL.add("  C_Gecos         TEXT,");
	query.SQL.add("  C_Realm         TEXT NOT NULL,");
	query.SQL.add("  C_Hash_MD5      CHAR(32) NOT NULL,");
	query.SQL.add("  C_Hash_SHA256   CHAR(64) NOT NULL,");
	query.SQL.add("  C_Hash_SHA512   CHAR(128) NOT NULL,");
	query.SQL.add("  C_Digest_MD5    CHAR(32) NOT NULL,");
	query.SQL.add("  C_Digest_SHA256 CHAR(64) NOT NULL,");
	query.SQL.add("  C_Digest_SHA512 CHAR(128) NOT NULL,");
	query.SQL.add("  C_Level         INT NOT NULL");
	query.SQL.add(")");

	// Standard execution of query
	try {
		dbguard.prepare();
		retVal = query.execSQL();
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::createCredentialTable()\n" + sExcept;
		writeLog(sText);
	}

	return retVal;
}


size_t TCredentials::readUserCredentials() {

	// Rebuild user mapping
	users.clear();
	invalidate();

	if (!util::assigned(database))
		return std::string::npos;

	// Prepare request
	size_t retVal = 0;
	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("SELECT *");
	query.SQL.add("FROM " + CREDENTIAL_TABLE_NAME);

	// Read first row and first column as result for (filtered) row count of table
	try {
		TCredential user;
		dbguard.prepare();
		if (query.first()) {
			do {

				// Read user properties
				user.username     = query["C_Username"].asString();
				user.lastname     = query["C_Lastname"].asString();
				user.givenname    = query["C_Givenname"].asString();
				user.password     = query["C_Password"].asString();
				user.description  = query["C_Description"].asString();
				user.gecos        = query["C_Gecos"].asString();
				user.realm        = query["C_Realm"].asString();
				user.hashMD5      = query["C_Hash_MD5"].asString();
				user.hashSHA256   = query["C_Hash_SHA256"].asString();
				user.hashSHA512   = query["C_Hash_SHA512"].asString();
				user.digestMD5    = query["C_Digest_MD5"].asString();
				user.digestSHA256 = query["C_Digest_SHA256"].asString();
				user.digestSHA512 = query["C_Digest_SHA512"].asString();
				user.level        = query["C_Level"].asInteger(0);

				// Add user to map
				if (!user.username.empty())
					users[user.username] = user;

			} while (query.next());

			// All users read from table
			retVal = query.getRecordCount();

		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::readUserCredentials()\n" + sExcept;
		writeLog(sText);
	}

	return retVal;
}


size_t TCredentials::readCredentialTable(util::TStringList& list, const std::string filter, const EOutputFormat format) const {
	list.clear();

	if (!util::assigned(database))
		return std::string::npos;

	size_t retVal = 0;
	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("SELECT *");
	query.SQL.add("FROM " + CREDENTIAL_TABLE_NAME);

	// Read first row and first column as result for (filtered) row count of table
	try {
		dbguard.prepare();
		if (query.first()) {

			// All users read from table
			retVal = query.fetchAll();

			// Apply filter and store user credential as HTML or JSON
			if (retVal > 0) {
				switch (format) {
					case EOF_HTML:
						if (!filter.empty()) {
							query.getTable().filter(filter).asHTML(list);
						} else {
							query.asHTML(list);
						}
						break;
					case EOF_JSON:
						if (!filter.empty()) {
							query.getTable().filter(filter).asJSON(list);
						} else {
							query.asJSON(list);
						}
						break;
				}
			}
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::readCredentialTable()\n" + sExcept;
		writeLog(sText);
	}

	return retVal;
}


std::string TCredentials::asHTML(const std::string filter) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	if (html.empty()) {
		util::TStringList list;
		readCredentialTable(list, filter, EOF_HTML);
		if (!list.empty())
			html = list.text();
	}
	return html;
}

std::string TCredentials::asJSON(const std::string filter) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	json.clear();
	util::TStringList list;
	readCredentialTable(list, filter, EOF_JSON);
	if (!list.empty()) {
		json = list.text();
	}
	return json;
}

void TCredentials::getCredentialMap(TCredentialMap& users) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	users = this->users;
}

bool TCredentials::getUserCredentials(const std::string username, TCredential& credential) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	credential.clear();
	const_iterator it = users.find(username);
	if (it != end()) {
		credential = it->second;
		return true;
	}
	return false;
}

size_t TCredentials::size() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return users.size();
}


size_t TCredentials::hasUserName(const std::string username) {
	if (!util::assigned(database))
		return std::string::npos;

	size_t retVal = 0;
	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Set transact SQL statement
	query.SQL.clear();
	query.SQL.add("SELECT count(*)");
	query.SQL.add("FROM " + CREDENTIAL_TABLE_NAME);
	query.SQL.add("WHERE C_Username = '" + username + "'");

	// Read first row and first column as result for (filtered) row count of table
	try {
		dbguard.prepare();
		if (query.first()) {
			retVal = query[0].asInteger64();
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::hasUserName()\n" + sExcept;
		writeLog(sText);
		retVal = std::string::npos;
	}

	return retVal;
}

bool TCredentials::insertUserRecord(const TCredential& user) {
	bool retVal = false;
	invalidate();

	if (!util::assigned(database))
		return retVal;

	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Create SQL query for insert of all fields by parameter
	query.SQL.clear();
	query.SQL.add("INSERT INTO " + CREDENTIAL_TABLE_NAME + " VALUES (");
	query.SQL.add("@username,");
	query.SQL.add("@lastname,");
	query.SQL.add("@givenname,");
	query.SQL.add("@password,");
	query.SQL.add("@description,");
	query.SQL.add("@gecos,");
	query.SQL.add("@realm,");
	query.SQL.add("@hashMD5,");
	query.SQL.add("@hashSHA256,");
	query.SQL.add("@hashSHA512,");
	query.SQL.add("@digestMD5,");
	query.SQL.add("@digestSHA256,");
	query.SQL.add("@digestSHA512,");
	query.SQL.add("@level");
	query.SQL.add(")");

	// Execute insert statement
	try {
		dbguard.prepare();

		// Bind parameter to each field
		size_t paraCount = query.getParameterCount();
		if (paraCount >= 10) {
			query.param("@username")     = user.username;
			query.param("@lastname")     = user.lastname;
			query.param("@givenname")    = user.givenname;
			query.param("@password")     = user.password;
			query.param("@description")  = user.description;
			query.param("@gecos")        = user.gecos;
			query.param("@realm")        = user.realm;
			query.param("@hashMD5")      = user.hashMD5;
			query.param("@hashSHA256")   = user.hashSHA256;
			query.param("@hashSHA512")   = user.hashSHA512;
			query.param("@digestMD5")    = user.digestMD5;
			query.param("@digestSHA256") = user.digestSHA256;
			query.param("@digestSHA512") = user.digestSHA512;
			query.param("@level")        = user.level;

			// Insert row data
			retVal = query.execSQL();
		}

	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::insertUserRecord() \n" + sExcept + "\n";
		writeLog(sText);
	}

	return retVal;
}

bool TCredentials::updateUserRecord(const TCredential& user) {
	bool retVal = false;
	invalidate();

	if (!util::assigned(database))
		return retVal;

	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Create SQL query for insert of all fields by parameter
	query.SQL.clear();
	query.SQL.add("UPDATE " + CREDENTIAL_TABLE_NAME);
	query.SQL.add("  SET C_Lastname = @lastname,");
	query.SQL.add("  C_Givenname = @givenname,");
	query.SQL.add("  C_Password = @password,");
	query.SQL.add("  C_Description = @description,");
	query.SQL.add("  C_Gecos = @gecos,");
	query.SQL.add("  C_Realm = @realm,");
	query.SQL.add("  C_Hash_MD5 = @hashMD5,");
	query.SQL.add("  C_Hash_SHA256 = @hashSHA256,");
	query.SQL.add("  C_Hash_SHA512 = @hashSHA512,");
	query.SQL.add("  C_Digest_MD5 = @digestMD5,");
	query.SQL.add("  C_Digest_SHA256 = @digestSHA256,");
	query.SQL.add("  C_Digest_SHA512 = @digestSHA512,");
	query.SQL.add("  C_Level = @level");
	query.SQL.add("WHERE C_Username = @username");

	// Execute update statement
	try {
		dbguard.prepare();

		// Bind parameter to each field
		size_t paraCount = query.getParameterCount();
		if (paraCount >= 10) {
			query.param("@username")     = user.username;
			query.param("@lastname")     = user.lastname;
			query.param("@givenname")    = user.givenname;
			query.param("@password")     = user.password;
			query.param("@description")  = user.description;
			query.param("@gecos")        = user.gecos;
			query.param("@realm")        = user.realm;
			query.param("@hashMD5")      = user.hashMD5;
			query.param("@hashSHA256")   = user.hashSHA256;
			query.param("@hashSHA512")   = user.hashSHA512;
			query.param("@digestMD5")    = user.digestMD5;
			query.param("@digestSHA256") = user.digestSHA256;
			query.param("@digestSHA512") = user.digestSHA512;
			query.param("@level")        = user.level;

			// Insert row data
			retVal = query.execSQL();
		}

	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::updateUserRecord() \n" + sExcept + "\n";
		writeLog(sText);
	}

	return retVal;
}


bool TCredentials::deleteUserRecord(const std::string& username) {
	bool retVal = false;
	invalidate();

	if (!util::assigned(database))
		return retVal;

	sqlite::TQuery query(database);
	sql::TQueryGuard<sqlite::TQuery> dbguard(query);

	// Create SQL query to delete user entry
	query.SQL.clear();
	query.SQL.add("DELETE FROM " + CREDENTIAL_TABLE_NAME);
	query.SQL.add("WHERE C_Username = @username");

	// Execute delete statement
	try {
		dbguard.prepare();
		if (query.getParameterCount() > 0) {
			query.param("@username") = username;
			retVal = query.execSQL();
		} else {
			std::cout << "TCredentials::deleteUserRecord() Parameter count = " << std::to_string((size_u)query.getParameterCount()) << std::endl;
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TCredentials::deleteUserRecord() \n" + sExcept + "\n";
		writeLog(sText);
	}

	return retVal;
}


bool TCredentials::hash(TCredential& user, const EHttpAuthType algorithm, const std::string& realm) {
	// ATTENTION: password is lost after hashing the credentials!!!
	if (user.givenname.empty())
		user.givenname = "Default";
	if (user.lastname.empty())
		user.lastname = user.username;
	if (user.gecos.empty())
		user.gecos = user.username;
	if (user.realm.empty())
		user.realm = realm;
	if (user.valid()) {
		util::TDigest MD5(util::EDT_MD5);
		user.hashMD5 = MD5(user.password);
		user.digestMD5 = MD5(util::csnprintf("%:%:%", user.username, user.realm, user.password));

		util::TDigest SHA256(util::EDT_SHA256);
		user.hashSHA256 = SHA256(user.password);
		user.digestSHA256 = SHA256(util::csnprintf("%:%:%", user.username, user.realm, user.password));

		util::TDigest SHA512(util::EDT_SHA512);
		user.hashSHA512 = SHA512(user.password);
		user.digestSHA512 = SHA512(util::csnprintf("%:%:%", user.username, user.realm, user.password));

		user.password = "*";
		return true;
	}
	return false;
}

bool TCredentials::insert(TCredential& user) {
	app::TLockGuard<app::TMutex> lock(mtx);
	bool ok = false;
	if (hash(user, auth, realm)) {
		size_t r = hasUserName(user.username);
		if (r == std::string::npos)
			return false;
		if (r > 0) {
			ok = updateUserRecord(user);
		} else {
			ok = insertUserRecord(user);
		}
	}
	if (ok) {
		users[user.username] = user;
	}
	return ok;
}

bool TCredentials::modify(TCredential& user) {
	return insert(user);
}

bool TCredentials::remove(const std::string& username) {
	app::TLockGuard<app::TMutex> lock(mtx);
	bool ok = false;
	size_t r = hasUserName(username);
	if (r == std::string::npos)
		return false;
	if (r > 0)
		ok = deleteUserRecord(username);
	if (ok) {
		TCredentialMap::iterator it = users.find(username);
		if (it != users.end()) {
			users.erase(it);
		}
	}
	return ok;
}


void TCredentials::debugOutput(const std::string& preamble) {
	app::TLockGuard<app::TMutex> lock(mtx);
	TCredentialMap::const_iterator it = users.begin();
	while (it != users.end()) {
		std::cout << preamble << "Username      : " << it->second.username << std::endl;
		std::cout << preamble << "Lastname      : " << it->second.lastname << std::endl;
		std::cout << preamble << "Givenname     : " << it->second.givenname << std::endl;
		std::cout << preamble << "Password      : " << it->second.password << std::endl;
		std::cout << preamble << "Description   : " << it->second.description << std::endl;
		std::cout << preamble << "Gecos         : " << it->second.gecos << std::endl;
		std::cout << preamble << "Realm         : " << it->second.realm << std::endl;
		std::cout << preamble << "Hash MD5      : " << it->second.hashMD5 << std::endl;
		std::cout << preamble << "Hash SHA256   : " << it->second.hashSHA256 << std::endl;
		std::cout << preamble << "Hash SHA512   : " << it->second.hashSHA512 << std::endl;
		std::cout << preamble << "Digest MD5    : " << it->second.digestMD5 << std::endl;
		std::cout << preamble << "Digest SHA256 : " << it->second.digestSHA256 << std::endl;
		std::cout << preamble << "Digest SHA512 : " << it->second.digestSHA512 << std::endl;
		std::cout << preamble << "Level         : " << it->second.level << std::endl << std::endl;
		++it;
	}
}

} /* namespace app */
