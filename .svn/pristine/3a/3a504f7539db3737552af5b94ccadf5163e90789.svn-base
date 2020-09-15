/*
 * fft.cpp
 *
 * The folowing code was adapted from:
 * https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B
 *
 * Calculate the FFT (Fast Fourier Transform) of an input sequence.
 * The most general case allows for complex numbers at the input and results in a sequence of equal length, again of complex numbers.
 * If you need to restrict yourself to real numbers, the output should be the magnitude (i.e. sqrt(re²+im²)) of the complex result.
 * The classic version is the recursive Cooley–Tukey FFT. Wikipedia has pseudo-code for that.
 * Further optimizations are possible but not required.
 *
 * See also:
 * https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
 *
 * This simplified form assumes that N is a power of two; since the number of sample points N can usually be chosen freely 
 * by the application (e.g. by changing the sample rate or window, zero-padding, etcetera), 
 * this is often not an important restriction. 
 * 
 * Performance analysis of the code below for 8 values an 100 cycles, see test() and test1():
 *   fft   Duration = 14 microseconds
 *   ifft  Duration = 25 microseconds
 *   fft1  Duration = 136 microseconds
 *   ifft1 Duration = 134 microseconds
 *
 *  Created on: 22.07.2019
 *      Author: dirk
 */

#include "fft.h"
#include "datetime.h"
#include "mathconsts.h"

namespace numerical {

#ifdef GCC_IS_C14
STATIC_CONST auto pi2m = -2.0 * value_pi<double>;
#else
STATIC_CONST auto pi2m = -2.0 * pi;
#endif

TFFT::TFFT() {
}

TFFT::~TFFT() {
}

TComplex TFFT::value(const double re, const double im) const {
	return TComplex(re, im);
}

TComplex TFFT::value(const double val) const {
	return TComplex(val, 0);
}

// Cooley–Tukey FFT (in-place, divide-and-conquer)
// Higher memory requirements and redundancy although more intuitive
void TFFT::fft1(TComplexArray& values) {
	const size_t N = values.size();
	const size_t N2 = N / 2;
	if (N <= 1) return;

	// Divide
	TComplexArray even = values[std::slice(0, N2, 2)];
	TComplexArray odd  = values[std::slice(1, N2, 2)];

	// Conquer
	fft1(even);
	fft1(odd);

	// Combine
	for (size_t k = 0; k < N2; ++k) {
		TComplex t = std::polar(1.0, pi2m * k / N) * odd[k];
		values[k] = even[k] + t;
		values[k + N2] = even[k] - t;
	}
}

// Inverse FFT (in-place)
void TFFT::ifft1(TComplexArray& values) {
	// Conjugate the complex numbers
	values = values.apply(std::conj);

	// Forward FFT
	fft1(values);

	// Conjugate the complex numbers again
	values = values.apply(std::conj);

	// Scale the numbers
	values /= values.size();
}


// Cooley-Tukey FFT (in-place, breadth-first, decimation-in-frequency)
// Better optimized but less intuitive
void TFFT::fft(TComplexArray &values) {
	// DFT
	uint32_t N = values.size(), k = N, n;
	double thetaT = pi / N;
	TComplex T, phiT = TComplex(cos(thetaT), -sin(thetaT));
	while (k > 1) {
		n = k;
		k >>= 1;
		phiT = phiT * phiT;
		T = 1.0L;
		for (uint32_t l = 0; l < k; l++) {
			for (uint32_t a = l; a < N; a += n) {
				uint32_t b = a + k;
				TComplex t = values[a] - values[b];
				values[a] += values[b];
				values[b] = t * T;
			}
			T *= phiT;
		}
	}

	// Decimate
	uint32_t m = (uint32_t)log2(N);
	for (uint32_t a = 0; a < N; a++) {
		uint32_t b = a;

		// Reverse bits
		b = (((b & 0xAAAAAAAA) >> 1) | ((b & 0x55555555) << 1));
		b = (((b & 0xCCCCCCC) >> 2) | ((b & 0x33333333) << 2));
		b = (((b & 0xF0F0F0F0) >> 4) | ((b & 0x0F0F0F0F) << 4));
		b = (((b & 0xFF00FF00) >> 8) | ((b & 0x00FF00FF) << 8));
		b = ((b >> 16) | (b << 16)) >> (32 - m);
		if (b > a) {
			TComplex t = values[a];
			values[a] = values[b];
			values[b] = t;
		}
	}
}

// Inverse FFT (in-place)
void TFFT::ifft(TComplexArray& values) {
	// Conjugate the complex numbers
	values = values.apply(std::conj);

	// Forward FFT
	fft(values);

	// Conjugate the complex numbers again
	values = values.apply(std::conj);

	// Scale the numbers
	values /= values.size();
}


void TFFT::print(TComplexArray& values) {
	size_t size = values.size();
	std::streamsize ss = std::cout.precision();
	std::cout.precision(std::numeric_limits<double>::digits10 + 1);
	try {
		for (size_t i=0; i<size; ++i) {
			TComplex& value = values[i];
			std::cout << "Value[" << i << "] = (" << value.real() << ", " << value.imag() << ")" << std::endl;
		}
	} catch (...) {};
	std::cout.precision(ss);
}


void TFFT::test() {
	const TComplex test[] = { 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0 };
	TComplexArray work, data(test, 8);
	util::TDateTime time;
	util::TTimePart duration;

	// Input data
	std::cout << "DATA" << std::endl;
	print(data);
	std::cout << std::endl;

	// Forward FFT
	time.start();
	for (int k=0; k<128; ++k) {
		work = data;
		fft(work);
	}
	duration = time.stop(util::ETP_MICRON);

	// Result data after FFT
	data = work;
	std::cout << "FFT" << std::endl;
	print(data);
	std::cout << "Duration = " << std::to_string((size_u)duration) << " microseconds per 1024 samples" << std::endl;

	// Inverse FFT
	time.start();
	for (int k=0; k<128; ++k) {
		work = data;
		ifft(work);
	}
	duration = time.stop(util::ETP_MICRON);

	// Result data after IFFT
	data = work;
	std::cout << std::endl << "IFFT" << std::endl;
	print(data);
	std::cout << "Duration = " << std::to_string((size_u)duration) << " microseconds per 1024 samples" << std::endl << std::endl;
}

void TFFT::test1() {
	const TComplex test[] = { 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0 };
	TComplexArray work, data(test, 8);
	util::TDateTime time;
	util::TTimePart duration;

	// Input data
	std::cout << "DATA" << std::endl;
	print(data);
	std::cout << std::endl;

	// Forward FFT
	time.start();
	for (int k=0; k<128; ++k) {
		work = data;
		fft1(work);
	}
	duration = time.stop(util::ETP_MICRON);

	// Result data after FFT
	data = work;
	std::cout << "FFT" << std::endl;
	print(data);
	std::cout << "Duration = " << std::to_string((size_u)duration) << " microseconds per 1024 samples" << std::endl;

	// Inverse FFT
	time.start();
	for (int k=0; k<128; ++k) {
		work = data;
		ifft1(work);
	}
	duration = time.stop(util::ETP_MICRON);

	// Result data after IFFT
	data = work;
	std::cout << std::endl << "IFFT" << std::endl;
	print(data);
	std::cout << "Duration = " << std::to_string((size_u)duration) << " microseconds per 1024 samples" << std::endl << std::endl;
}


} /* namespace numerical */
