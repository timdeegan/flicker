#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/gpio.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "fft.h"
#include "graph.h"
#include "sample.h"

/* Phototransistor sampled here. */
#define PT_PIN 26

/* The phototransistor is (just) able to pick up 110kHz
 * flicker, so we need to sample at least twice as fast.
 * The pico can go to 500kHz but we don't have room to
 * process that much data. */
#define SAMPLE_RATE 262144.0

/* Sample count is limited by our FFT implementation.
 * It uses 8 bytes per sample and only works on powers of two,
 * so use 128kB just for that, and everything else fits in the
 * other half of memory.  That give us 1/16th of a second
 * at our chosen sample rate, i.e. 6.25 cycles of 100Hz. */
#define SAMPLE_COUNT (16u * 1024u)

/* FFT buckets: the FFT produces twice as many but
 * everything above this is just aliasing. */
#define FREQ_COUNT ((SAMPLE_COUNT / 2u) + 1u)
static unsigned int bucket(float freq)
{
    float hz_per_bucket = (SAMPLE_RATE / 2) / (FREQ_COUNT - 1);
    return roundf(freq / hz_per_bucket);
}

/* Raw 12-bit samples from the ADC. */
static uint16_t samples[SAMPLE_COUNT];

/* FFT calculation space.
 * We convert cartesian to polar coordinates in place to save space.
 * I can't think of a nice way of doing that without turning off
 * the aliasing rules, but we have turned them off, so that's OK. */
static struct {
    union {
        float real[SAMPLE_COUNT];
        float magnitude[FREQ_COUNT];
    };
    union {
        float imag[SAMPLE_COUNT];
        float phase[FREQ_COUNT];
    };
} f;

/* Assertion failures stop the world and keep logging so
 * we can connect the serial console to debug.  */
void assertion_failure(const char *pred, const char *file, int line)
{
    while (true) {
        printf("ASSERTION FAILED at %s line %d: %s\n", file, line, pred);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(10);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(990);
    }
}

/* Convert uint16_t samples to complex floats.
 * Returns false on error. */
static bool make_complex(uint16_t *samples,
                         float *real,
                         float *imag,
                         unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        uint16_t s = samples[i];
        if (s & SAMPLE_ERROR) {
            printf("Sampling error at %d/%d: 0x%4.4x\n",
                i, SAMPLE_COUNT, s);
            return false;
        }
        real[i] = (float)s;
        imag[i] = 0.0;
    }
    return true;
}

/* Convert FFT results from cartesian to polar coordinates. */
static void make_polar(float *real,
                       float *imag,
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

/* Measure a light source and report on it.
 * Returns false on error. */
static bool measure(void)
{

    /* Collect uint16_t samples in [0, 0xfff]. */
    sample(SAMPLE_COUNT, SAMPLE_RATE, samples);

    /* Convert those to floating point. */
    if (!make_complex(samples, f.real, f.imag, SAMPLE_COUNT)) {
        return false;
    }

    /* TODO here: Windowing. */
    fft(f.real, f.imag, SAMPLE_COUNT);
    make_polar(f.real, f.imag, f.magnitude, f.phase, FREQ_COUNT);

    /* Look at the spectrum, skipping everything < 10Hz */
    graph_logx("FFT", f.magnitude + bucket(10), FREQ_COUNT - bucket(10));

    /* Expect to see 210Hz PWM, 1248.3 samples/cycle. */
    graph("Raw samples (9.5ms)", samples + 8000, 2500);

    return true;
}

int main(void)
{
    /* Debugging metadata that gets baked into the binary. */
    bi_decl(bi_program_name("flicker"));
    bi_decl(bi_program_version_string("0.1"));
    bi_decl(bi_program_description("Lighting flicker meter"));
    bi_decl(bi_1pin_with_name(PT_PIN, "Phototransistor sampling pin"));

    /* Debugging output will go to the USB console. */
    stdio_init_all();

    /* Hardware setup. */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    sample_init(PT_PIN);

    while (1) {
        /* TODO: wait for a button press? */
        sleep_ms(1000);
        measure();
    }
}
