#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Find the dominant frequency in the FFT. 
 * Returns a *normalized* frequency, in buckets. */
extern float peak(float *magnitudes, unsigned int count);

/* Convert uint16_t samples to complex floats, windowed for FFT'ing.
 * Returns false on error. */
extern bool window(const uint16_t *samples,
                   float *real,
                   float *imag,
                   unsigned int count);

/* Convert complex numbers from cartesian to polar coordinates. */
extern void make_polar(const float *real,
                       const float *imag,
                       float *abs,
                       float *angle,
                       unsigned int count);
