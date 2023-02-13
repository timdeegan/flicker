#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/float.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "../assertions.h"
#include "../agc.h"
#include "../dsp.h"
#include "../fft.h"
#include "../graph.h"
#include "../pins.h"
#include "../sample.h"

/* Assertion failures in testing don't stop the world. */
static bool failed;
void assertion_failure(const char *pred, const char *file, int line)
{
    printf(" ** ASSERTION FAILED ** at %s line %d: %s\n", file, line, pred);
    failed = true;
}

/* Buffers to run FFT tests in. */
#define MAX_FFT_LENGTH (1u << 12)
static float real[MAX_FFT_LENGTH];
static float imag[MAX_FFT_LENGTH];

/* Check the results of an FFT are close enough. */
static bool fft_match(const float *ours,
                      const float *theirs,
                      unsigned int length)
{
    unsigned int errors = 0;

    /* Allow any entry to be off by 0.01% of the largest entry.
     * (It's OK if we get 0.3 instead of 0 if the max is 3,000). */
    float max = 0.0;
    for (unsigned int i = 0; i < length; i++)
        max = fmaxf(max, theirs[i]);
    float abs_margin = max / 100000;

    /* Allow any entry to be off by 0.1% for rounding errors. */
    for (unsigned int i = 0; i < length && errors < 10; i++)
    {
        float a = ours[i], b = theirs[i];
        float rel_margin = fabsf(a) + fabsf(b) / 1000;
        if (fabsf(a - b) > abs_margin + rel_margin) {
            printf("%f != %f\n", a, b);
            errors++;
        }
    }

    return (errors == 0);
}

/* Run an FFT and check that we got the same answer
 * that the python generator got. */
static void fft_test(const char *name,
                     unsigned int length,
                     const float *real_input,
                     const float *real_reference,
                     const float *imag_reference)
{
    printf("FFT %s\n", name);
    failed = false;

    ASSERT(length <= MAX_FFT_LENGTH);
    memcpy(real, real_input, length * sizeof *real);
    memset(imag, 0, length * sizeof *imag);

    fft(real, imag, length);

    ASSERT(fft_match(real, real_reference, length));
    ASSERT(fft_match(imag, imag_reference, length));

    printf("FFT %s: %s\n", name, failed ? "FAILED" : "OK");
}

/* Buffer for sampling */
#define SAMPLE_COUNT 10000u
static uint16_t samples[SAMPLE_COUNT];

/* Test ADC sampling. */
static void sample_test(unsigned int count, float hz, bool print)
{
    printf("SAMPLE %d @%fHz\n", count, hz);
    failed = false;

    memset(samples, 0, count * sizeof *samples);
    sample(count, hz, samples);
    for (unsigned int i = 0; i < count; i++) {
        if (print) {
            printf("  %3d: 0x%04x\n", i, samples[i]);
        }
        /* No errors. */
        ASSERT((samples[i] & SAMPLE_ERROR) == 0);
        /* No blanks. */
        ASSERT(samples[i] != 0);
    }

    printf("SAMPLE %d @%fHz: %s\n", count, hz, failed ? "FAILED" : "OK");
}

/* Test plotting. */
static void graph_test(void)
{
    printf("GRAPH\n");
    failed = false;

    for (unsigned int i = 0; i < 1000; i++) {
        samples[i] = roundf((sinf(M_TWOPI * i / 500) + 1.0) * 32767);
    }
    graph(samples, 1000);

    printf("GRAPH: %s\n", failed ? "FAILED" : "OK");
}

/* Check that the window function looks sensible */
static void window_test(void)
{
    printf("WINDOW\n");
    failed = false;

    /* Square wave: half high, half low. */
    for (unsigned int i = 0; i < MAX_FFT_LENGTH; i++) {
        samples[i] = (i < MAX_FFT_LENGTH / 2) ? 0x100 : 0;
    }
    /* Window.  Should end up with a Gaussian curve,
     * with the second half inverted. */
    bool ok = window(samples, real, imag, MAX_FFT_LENGTH);
    ASSERT(ok);
    /* Back into uint16s for plotting. */
    for (unsigned int i = 0; i < MAX_FFT_LENGTH; i++) {
        samples[i] = real[i] + 0x80;
    }
    graph(samples, MAX_FFT_LENGTH);

    printf("WINDOW: %s\n", failed ? "FAILED" : "OK");
}

/* Measure the average level over 20ms to smooth out the
 * most common 100Hz ripple. */
static float average_sample(void)
{
    unsigned int i, total = 0, count = 5000;
    ASSERT(count < SAMPLE_COUNT);
    sample(count, 250e3, samples);
    for (i = 0; i < count; i++) {
        total += samples[i];
    }
    return (float) total / count;
}

/* Check that the AGC logic can set the resistance properly. */
static void agc_test(void)
{
    float low, mid, high, low_r, mid_r, high_r;
    printf("AGC\n");
    failed = false;

    agc_reset();

    /* Set to a low resistance and measure the voltage across it.  Pick a
     * number near the bottom but not *at* the bottom so the error in
     * FIXED_OHMS doesn't ruin our sums. */
    agc_set_level(16);
    low_r = AGC_OHMS(16);
    low = average_sample();

    /* Measure again in mid-range. */
    agc_set_level(63);
    mid_r = AGC_OHMS(63);
    mid = average_sample();

    /* Measure again at the highest resistance. */
    agc_set_level(127);
    high_r = AGC_OHMS(127);
    high = average_sample();

    printf("Low %f, mid %f, high %f\n", low, mid, high);

    /* Increasing resistance with constant current should increase voltage. */
    ASSERT(low < mid);
    ASSERT(mid < high);

    /* In the middle of the range, the increase should be linear-ish. */
    if (low > 100 && mid < 3500) {
        float prediction = mid * low_r / mid_r;
        printf("Predicted mid->low %f (actual %f)\n", prediction, low);
        ASSERT(fabsf((prediction - low) / prediction) < 0.1);
    } else {
        printf("No mid->low prediction: out of range.\n");
    }
    if (mid > 100 && high < 3500) {
        float prediction = high * mid_r / high_r;
        printf("Predicted high->mid %f (actual %f)\n", prediction, mid);
        ASSERT(fabsf((prediction - mid) / prediction) < 0.1);
    } else {
        printf("No high->mid prediction: out of range.\n");
    }

    printf("AGC: %s\n", failed ? "FAILED" : "OK");
}

int main(void)
{
    /* Debugging metadata that gets baked into the binary. */
    bi_decl(bi_program_name("unit-tests"));
    bi_decl(bi_program_version_string("0.1"));
    bi_decl(bi_program_description("Unit tests for flicker"));

    /* Debugging output will go to the USB console. */
    stdio_init_all();

    /* Hardware setup. */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    sample_init(PT_PIN);
    agc_init(AD5220_DIR_PIN, AD5220_CLOCK_PIN);

    while(1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        printf("Starting tests.\n");

        graph_test();

#include "fft-test-cosine.h"
#include "fft-test-noise.h"
#include "fft-test-square.h"
#include "fft-test-bigsquare.h"
#include "fft-test-sawtooth.h"
#include "fft-test-bigsawtooth.h"

        sample_test(10, 1e3, true);
        sample_test(SAMPLE_COUNT, 500e3, false);

        window_test();

        agc_test();

        printf("Tests complete.\n\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(1000);
    }
}