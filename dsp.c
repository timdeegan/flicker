#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "dsp.h"
#include "sample.h"

/* Find the dominant frequency in the FFT. 
 * Returns a *normalized* frequency, in buckets. */
float peak(float *magnitudes, unsigned int count)
{
    unsigned int i, max_index = 0;
    float max_val = -1;

    /* Skip bucket 0 (DC), though it should be 0 anyway. */
    for (i = 1; i < count; i++) {
        if (magnitudes[i] > max_val) {
            max_val = magnitudes[i];
            max_index = i;
        }
    }

    return max_index;
}

/* Convert uint16_t samples to complex floats, windowed for FFT'ing.
 * Returns false on error. */
bool window(const uint16_t *samples,
            float *real,
            float *imag,
            unsigned int count)
{
    unsigned int i;
    float t, window, sum = 0.0, mean, middle;

    /* Find the mean so we can remove DC. */
    for (i = 0; i < count; i++) {
        sum += samples[i];
    }
    mean = sum / count;

    /* We'll apply a windowing function to the samples before
     * the FFT.  This reduces edge effects that crop up because
     * the sample doesn't wrap around at the edges, and the FFT
     * assumes that it does.
     *
     * Our window function is "Gaussian, r = 8" from Gasior and Gonzales.
     * it's relatively expensive, but we're not optimizing yet, and
     * it lets us use Gaussian interpolation on the results.
     *
     * It's e^(-r^2*t^2/(2L^2)) where
     *  L = window length,
     *  t = time (in samples, from the middle of the window),
     *  r = ratio of L to sigma, in our case set to 8.
     * Pull out the constant parts, K=-r^2/2L*2. */
    float K = -32.0 / (count * count);
    middle = (float)(count - 1) / 2;

    for (i = 0; i < count; i++) {
        uint16_t s = samples[i];
        if (s & SAMPLE_ERROR) {
            printf("Sampling error at %d/%d: 0x%4.4x\n",
                i, count, s);
            return false;
        }

        /* Calculate the window function. */
        t = (float)i - middle;
        window = expf(K*t*t);

        /* Remove DC and apply the window. */
        real[i] = window * ((float)s - mean);
        imag[i] = 0.0;
    }
    return true;
}

/* Convert complex numbers from cartesian to polar coordinates. */
void make_polar(const float *real,
                const float *imag,
                float *abs,
                float *angle,
                unsigned int count)
{
    unsigned int n;
    for (n = 0; n < count; n++) {
        double r = real[n], i = imag[n];
        abs[n] = sqrt(r * r + i * i);
        angle[n] = atan2(i, r);
    }
}
