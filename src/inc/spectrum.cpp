/*
 * spectrum.cpp
 *
 *  Created on: 25.08.2019
 *      Author: dirk
 */

#include "spectrum.h"

namespace app {

TSpectrum::TSpectrum() {
	reset();
}

TSpectrum::~TSpectrum() {

}

void TSpectrum::reset() {
	maxFrequency = 0.0;
	maxAmplitude = 0.0;
}

void TSpectrum::clear() {
	for (size_t i=0; i<result.size(); ++i) {
		result[i].clear();
	}
	result.clear();
	reset();
}

} /* namespace app */
