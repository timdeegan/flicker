#pragma once

/* In-place radix-2 time-decimation FFT.
 * Input/output array length must must be a power of 2. */
extern void fft(float *real, float *imag, unsigned int length);
