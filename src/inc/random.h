/*
 * utilities.h
 *
 *  Created on: 16.08.2014
 *      Author: Dirk Brinkmeier
 */
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <random>
#include <string>
#include <iostream>
#include "gcc.h"

namespace util {

enum ERandomEngine { RE_DEFAULT, RE_RAN24, RE_RAN48, RE_KNUTH, RE_DEVICE, RE_SYSTEM };

long int setRandomSeed();

int randomize(int from, int to, ERandomEngine engine = ERandomEngine::RE_SYSTEM);
bool isValidUUID(const std::string& uuid);
std::string createUUID(bool forceRandom = true, ERandomEngine engine = ERandomEngine::RE_SYSTEM);
std::string fastCreateUUID(bool forceRandom = true, bool upper = true);
std::string fastCreateHexStr(size_t size, bool upper = false);

/*
 * Valid values for engine_t:
 * 	minstd_rand 	--> fast but poor random distribution
 * 	ranlux48 		--> good random distribution
 * 	random_device 	--> slow but real random
 */
template<typename engine_t>
	inline int randomizer(engine_t&& engine, int from, int to) {
#ifdef STL_HAS_RANDOM_ENGINE
	std::uniform_int_distribution<int> d(from, to);
	return d(engine);
#else
	return (rand() % (++to - from)) + from;
#endif
	}

} // namespace util

#endif /* UTILITIES_H_ */
