#ifndef FIX_FFT_H
#define FIX_FFT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int fixed_fft(int16_t samples[], int count, int inverse);

#ifdef __cplusplus
}
#endif

#endif
