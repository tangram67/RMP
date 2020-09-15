/*
 * mathconsts.h
 *
 *  Created on: 05.09.2019
 *      Author: dirk
 */

#ifndef INC_MATHCONSTS_H_
#define INC_MATHCONSTS_H_

#include "gcc.h"
#include <cmath>

namespace numerical {

// Use Variable templates (C++14)
#ifdef GCC_IS_C14

template <typename T>
#ifdef M_PIl
constexpr T value_pi = T(M_PIl);
#else
constexpr T value_pi = T(3.141592653589793238462643383279502884L);
#endif

template <typename T>
#ifdef M_El
constexpr T value_e = T(M_El);
#else
constexpr T value_e = T(2.718281828459045235360287471352662498L);
#endif

template <typename T>
constexpr T value_el = T(1.602176634E-19L);

template <typename T>
constexpr T value_e0 = T(1.2566370621219E-6L);

template <typename T>
constexpr T value_m0 = T(8.854187812813E-12L);

template <typename T>
constexpr T value_c0 = T(299792458L);

template <typename T>
constexpr T value_h = T(6.62607015E-34L);

template <typename T>
constexpr T value_a = T(1.0L / 137.03599908421L);

// Pi
STATIC_CONST auto pi = value_pi<double>;
STATIC_CONST auto PI = value_pi<long double>;

// Euler's number
STATIC_CONST auto e = value_e<double>;
STATIC_CONST auto E = value_e<long double>;

// Elementary charge (C)
STATIC_CONST auto el = value_el<double>;
STATIC_CONST auto EL = value_el<long double>;

// Vacuum permeability (H/m, N/A²)
STATIC_CONST auto e0 = value_e0<double>;
STATIC_CONST auto E0 = value_e0<long double>;

// Vacuum permittivity (F/m, As/Vm)
STATIC_CONST auto m0 = value_m0<double>;
STATIC_CONST auto M0 = value_m0<long double>;

// Vacuum lightspeed (m/s)
STATIC_CONST auto c0 = value_c0<double>;
STATIC_CONST auto C0 = value_c0<long double>;

// Planck constant (Js)
STATIC_CONST auto h = value_h<double>;
STATIC_CONST auto H = value_h<long double>;

// Fine-structure constant = (1.0 / (2.0 * c0 * e0)) * (el * el / h)
STATIC_CONST auto a = value_a<double>;
STATIC_CONST auto A = value_a<long double>;

#else

// Pi
#ifdef M_PI
STATIC_CONST auto pi = M_PI;
#else
STATIC_CONST auto pi = std::atan2(0.0, -1.0); // acos(-1.0) = 3.14159265358979323846
#endif
#ifdef M_PIl
STATIC_CONST auto PI = M_PIl;
#else
STATIC_CONST auto PI = 3.141592653589793238462643383279502884L;
#endif

// Euler's number
#ifdef M_E
STATIC_CONST auto e = M_E;
#else
STATIC_CONST auto e = 2.7182818284590452354;
#endif
#ifdef M_El
STATIC_CONST auto E = M_El;
#else
STATIC_CONST auto E = 2.718281828459045235360287471352662498L;
#endif

// Elementary charge (C)
STATIC_CONST auto el = 1.602176634E-19;
STATIC_CONST auto EL = 1.602176634E-19L;

// Vacuum permeability (H/m, N/A²)
STATIC_CONST auto e0 = 1.2566370621219E-6;
STATIC_CONST auto E0 = 1.2566370621219E-6L;

// Vacuum permittivity (F/m, As/Vm)
STATIC_CONST auto m0 = 8.854187812813E-12;
STATIC_CONST auto M0 = 8.854187812813E-12L;

// Vacuum lightspeed (m/s)
STATIC_CONST auto c0 = 299792458;
STATIC_CONST auto C0 = 299792458L;

// Planck constant (Js)
STATIC_CONST auto h = 6.62607015E-34;
STATIC_CONST auto H = 6.62607015E-34L;

// Fine-structure constant = (1.0 / (2.0 * c0 * e0)) * (el * el / h)
STATIC_CONST auto a = 1.0 / 137.03599908421;
STATIC_CONST auto A = 1.0L / 137.03599908421L;

#endif

} /* namespace numerical */

#endif /* INC_MATHCONSTS_H_ */
