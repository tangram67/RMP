/*
 * utilities.cpp
 *
 *  Created on: 16.08.2014
 *      Author: Dirk Brinkmeier
 */
#include "random.h"
#include "sysutils.h"
#include "templates.h"
#include "stringutils.h"

static bool localMacInitialized = false;
static bool randomSeedInitialized = false;
static const long int randomSeed = util::setRandomSeed();

#ifdef STL_HAS_LAMBDAS

static const std::string localMacAddress = [] {
	std::string eth = sysutil::getDefaultAdapter();
	std::string mac = sysutil::getMacAddress(eth);
	util::replace(mac, ":", "");
	localMacInitialized = mac.size() == 12;
	return mac;
} (); // () invokes lambda

#else

std::string initLocalMac() {
	std::string eth = util::getDefaultAdapter();
	std::string mac = util::getMacAddress(eth);
	util::replace(mac, ":", "");
	localMacInitialized = mac.size() == 12;
	return mac;
}
static const std::string localMacAddress = initLocalMac();

#endif

static const std::string localMacAddressUpper = localMacAddress;
static const std::string localMacAddressLower = util::tolower(localMacAddress);;

#ifdef STL_HAS_RANDOM_ENGINE
static std::random_device rdDevice;
static std::ranlux24 rdRanlux24(randomSeed);
static std::ranlux48 rdRanlux48(randomSeed);
static std::knuth_b rdKnuth(randomSeed);
static std::default_random_engine rdDefault(randomSeed);
#endif


long int util::setRandomSeed() {
	timespec t;
	long int retVal = randomSeed;
	if (!randomSeedInitialized) {
#ifdef CLOCK_REALTIME
		int r = clock_gettime(CLOCK_REALTIME, &t);
		if (checkSucceeded(r))
			retVal = t.tv_nsec + t.tv_sec;
		else
#endif
		retVal = ::time(nil);
		srand(retVal);
		randomSeedInitialized = true;
	}
	return retVal;
}


inline int sysrand(int from, int to) {
	return ((rand() % (++to - from)) + from);
}	

int util::randomize(int from, int to, ERandomEngine engine) {
	int r = 0;
#ifdef STL_HAS_RANDOM_ENGINE
	switch (engine) {
		case util::ERandomEngine::RE_DEVICE:
			r = util::randomizer(rdDevice, from, to);
			break;
		case util::ERandomEngine::RE_RAN24:
			r = util::randomizer(rdRanlux24, from, to);
			break;
		case util::ERandomEngine::RE_RAN48:
			r = util::randomizer(rdRanlux48, from, to);
			break;
		case util::ERandomEngine::RE_KNUTH:
			r = util::randomizer(rdKnuth, from, to);
			break;
		case util::ERandomEngine::RE_SYSTEM:
			r = sysrand(from, to);
			break;
		case util::ERandomEngine::RE_DEFAULT:
		default:
			r = util::randomizer(rdDefault, from, to);
			break;
	}
#else
	r = sysrand(from, to);
#endif
	return r;
}


#ifndef STL_HAS_LAMBDAS

// Workaround for compiler that have no lambda support
struct CRandom {
	ERandomEngine _engine;
	int _from, _to;
	int fn() {
		if (_engine == RE_SYSTEM)
			return sysrand(_from, _to);
		else	
			return random(_from, _to, _engine);
	}
	CRandom(ERandomEngine engine, int from, int to) : _engine(engine), _from(from), _to(to) {};
};

#endif


std::string util::createUUID(bool forceRandom, ERandomEngine engine) {
	/*
	 * This function generate a DCE 1.1, ISO/IEC 11578:1996 and IETF RFC-4122
	 * Version 4 conform local unique UUID based upon random number generation.
	 */
	char retVal[40]; // 32 Byte + 4 times '-' + reserve
	char *p = retVal;
	int i;
	int from = 0;
	int to = 15;

#ifdef STL_HAS_LAMBDAS

		auto fn = [=]() -> int { // Capture "from", "to" and "engine" by assigning the values via "[=]"
			return (engine == RE_SYSTEM) ? sysrand(from, to) : randomize(from, to, engine);
		};

#else

	// Workaround for compiler that have no lambda support
	CRandom r(engine, from, to);
	std::function<int()> fn;
	fn = std::bind(&CRandom::fn, &r);

#endif

	/* Data 1 - 8 characters.*/
	for(i = 0; i < 8; i++, p++)
		((*p = (fn())) < 10) ? *p += 48 : *p += 55;

	/* Data 2 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (fn())) < 10) ? *p += 48 : *p += 55;

	/* Data 3 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (fn())) < 10) ? *p += 48 : *p += 55;

	/* Data 4 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (fn())) < 10) ? *p += 48 : *p += 55;

	/* Data 5 - 12 characters.*/
	*p++ = '-';

	// Use MAC address instead of random numbers
	if (!localMacInitialized || forceRandom) {
		for(i = 0; i < 12; i++, p++)
			((*p = (fn())) < 10) ? *p += 48 : *p += 55;
	} else {
		strncpy(p, localMacAddress.c_str(), 12);
		p += localMacAddress.size();
	}

	*p = '\0';

	return std::string(retVal, 36);
}


bool util::isValidUUID(const std::string& uuid) {
	// 0123456789-123456789-123456789-12345 = 36
	// 0         10        20        30
	// 1A88C099-8060-7D21-AEB7-16DF4E0E1D22
	//         8    13   18   23
	if (uuid.size() == 36) {
		if ((uuid[8] == '-') &&
			(uuid[13] == '-') &&
			(uuid[18] == '-') &&
			(uuid[23] == '-')) {
			return true;
		}
	}
	return false;
}


std::string util::fastCreateUUID(bool forceRandom, bool upper) {
	/*
	 * This function generate a DCE 1.1, ISO/IEC 11578:1996 and IETF RFC-4122
	 * Version 4 conform local unique UUID based upon random number generation.
	 */
	char retVal[40]; // 32 Byte + 4 times '-' + reserve
	char *p = retVal;
	int off = upper ? 55 : 87;
	int i;

	if (!randomSeedInitialized) {
		setRandomSeed();
	}

	/* Data 1 - 8 characters.*/
	for(i = 0; i < 8; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 2 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 3 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 4 - 4 characters.*/
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 5 - 12 characters.*/
	*p++ = '-';

	// Use MAC address instead of random numbers
	if (!localMacInitialized || forceRandom) {
		for(i = 0; i < 12; i++, p++)
			((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;
	} else {
		std::string mac = upper ? localMacAddressUpper.c_str() : localMacAddressLower.c_str();
		if (mac.size() == 12) {
			strncpy(p, mac.c_str() , mac.size());
			p += mac.size();
		}
	}
	*p = '\0';

	return std::string(retVal, 36);
}


std::string util::fastCreateHexStr(size_t size, bool upper) {
	/*
	 * Example output: 81b680e...0002199
	 */
	char retVal[size+1];
	char *p = retVal;
	int off = upper ? 55 : 87;
	size_t i;

	if (!randomSeedInitialized) {
		setRandomSeed();
	}

	/* Data characters.*/
	for(i = 0; i < size; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;
	*p = '\0';

	return std::string(retVal, size);
}
