/*
 * credentialtypes.h
 *
 *  Created on: 03.12.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef CREDENTIALTYPES_H_
#define CREDENTIALTYPES_H_

#include <map>
#include <string>
#include "gcc.h"

namespace app {

typedef struct CCredential {
	std::string username;
	std::string lastname;
	std::string givenname;
	std::string password;
	std::string description;
	std::string gecos;
	std::string realm;
	std::string hashMD5;
	std::string hashSHA256;
	std::string hashSHA512;
	std::string digestMD5;
	std::string digestSHA256;
	std::string digestSHA512;
	int level;

	void prime() {
		level = 0;
	}

	void clear() {
		username.clear();
		lastname.clear();
		givenname.clear();
		password.clear();
		description.clear();
		gecos.clear();
		realm.clear();
		hashMD5.clear();
		hashSHA256.clear();
		hashSHA512.clear();
		digestMD5.clear();
		digestSHA256.clear();
		digestSHA512.clear();
		level = 0;
	}

	bool valid() const {
		if (!username.empty() &&
			!lastname.empty() &&
			!givenname.empty() &&
			!password.empty() &&
			!realm.empty()) {
			return true;
		}
		return false;
	}

	bool check() const {
		if (!username.empty() &&
			!lastname.empty() &&
			!givenname.empty() &&
			!password.empty()) {
			return true;
		}
		return false;
	}

	CCredential& operator = (const CCredential& value) {
		username = value.username;
		lastname = value.lastname;
		givenname = value.givenname;
		password = value.password;
		description = value.description;
		gecos = value.gecos;
		realm = value.realm;
		hashMD5 = value.hashMD5;
		hashSHA256 = value.hashSHA256;
		hashSHA512 = value.hashSHA512;
		digestMD5 = value.digestMD5;
		digestSHA256 = value.digestSHA256;
		digestSHA512 = value.digestSHA512;
		level = value.level;
		return *this;
	}

	CCredential() {
		prime();
	}

} TCredential;

class TCredentials;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PCredential = CCredential*;
using TCredentialMap = std::map<std::string, CCredential>;

#else

typedef CCredential* PCredential;
typedef std::map<std::string, TCredential> TCredentialMap;

#endif

} /* namespace app */

#endif /* CREDENTIALTYPES_H_ */
