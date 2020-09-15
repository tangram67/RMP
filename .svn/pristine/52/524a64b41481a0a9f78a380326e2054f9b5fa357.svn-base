/*
 * spectrum.h
 *
 *  Created on: 25.08.2019
 *      Author: dirk
 */

#ifndef INC_SPECTRUM_H_
#define INC_SPECTRUM_H_

#include <vector>

#include "gcc.h"
#include "fft.h"
#include "audiofile.h"
#include "audiobuffer.h"

namespace app {

typedef struct CSpectrumValue {
	numerical::TComplex complex;
	double frequency;
	double apmlitude;
} TSpectrumValue;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TSpectrumArray = std::vector<TSpectrumValue>;
using TSpectrumResult = std::vector<TSpectrumArray>;

#else

typedef std::vector<TSpectrumValue> TSpectrumArray;
typedef std::vector<TSpectrumArray> TSpectrumResult;

#endif


class TSpectrum {
private:
	double maxFrequency;
	double maxAmplitude;

	TSpectrumResult result;

public:
	void reset();
	void clear();

	double getMaxFrequency() const { return maxFrequency; };
	double getMaxAmplitude() const { return maxAmplitude; };

	TSpectrum();
	virtual ~TSpectrum();
};

} /* namespace app */

#endif /* INC_SPECTRUM_H_ */
