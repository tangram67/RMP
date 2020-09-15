/*
 * fft.h
 *
 *  Created on: 22.07.2019
 *      Author: dirk
 */

#ifndef INC_FFT_H_
#define INC_FFT_H_

#include "gcc.h"
#include <cmath>
#include <complex>
#include <iostream>
#include <valarray>

namespace numerical {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TComplex = std::complex<double>;
using TComplexArray = std::valarray<TComplex>;

#else

typedef std::complex<double> TComplex;
typedef std::valarray<TComplex> TComplexArray;

#endif

class TFFT {
private:
	void fft1(TComplexArray& values);
	void ifft1(TComplexArray& values);

public:
	TComplex value(const double re, const double im) const;
	TComplex value(const double val) const;

	void fft(TComplexArray& values);
	void ifft(TComplexArray& values);

	void print(TComplexArray& values);
	void test();
	void test1();

	TFFT();
	virtual ~TFFT();
};

} /* namespace numerical */

#endif /* INC_FFT_H_ */
