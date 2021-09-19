#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/float.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "../fft.h"

/* Like assert() but don't stop the world. */
static bool failed;
#define REQUIRE(pred) do {                 \
    if (!(pred)) {                         \
        printf("FAIL at %s line %d: %s\n", \
               __FILE__, __LINE__, #pred); \
        failed = true;                     \
    }                                      \
} while (0)

/* Buffers to run FFT tests in. */
#define MAX_TEST_LENGTH (1u << 12)
static float real[MAX_TEST_LENGTH];
static float imag[MAX_TEST_LENGTH];

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

    REQUIRE(length <= MAX_TEST_LENGTH);
    memcpy(real, real_input, length * sizeof *real);
    memset(imag, 0, length * sizeof *imag);

    fft(real, imag, length);

    REQUIRE(fft_match(real, real_reference, length));
    REQUIRE(fft_match(imag, imag_reference, length));

    printf("FFT %s: %s\n", name, failed ? "FAILED" : "OK");
}

int main(void)
{
    /* Debugging metadata that gets baked into the binary. */
    bi_decl(bi_program_name("fft-test"));
    bi_decl(bi_program_version_string("0.1"));
    bi_decl(bi_program_description("Unit tests for flicker"));

    /* Debugging output will go to the USB console. */
    stdio_init_all();

    /* Prepare to control the onboard LED. */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while(1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        printf("Starting tests.\n");
#include "fft-test-cosine.h"
#include "fft-test-noise.h"
#include "fft-test-square.h"
#include "fft-test-bigsquare.h"
#include "fft-test-sawtooth.h"
#include "fft-test-bigsawtooth.h"
        printf("Tests complete.\n\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(1000);
    }
}