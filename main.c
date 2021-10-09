#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/gpio.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "assertions.h"
#include "dsp.h"
#include "fft.h"
#include "graph.h"
#include "pins.h"
#include "sample.h"

/* The phototransistor is (just) able to pick up 110kHz
 * flicker, so we need to sample at least twice as fast.
 * The pico can go to 500kHz but we don't have room to
 * process that much data. */
#define SAMPLE_RATE 250000.0

/* Sample count is limited by our FFT implementation.
 * It uses 8 bytes per sample and only works on powers of two,
 * so use 128kB just for that, and everything else fits in the
 * other half of memory.  That give us 1/16th of a second
 * at our chosen sample rate, i.e. 6.25 cycles of 100Hz. */
#define SAMPLE_COUNT (16u * 1024u)

/* FFT buckets: the FFT produces twice as many but
 * everything above this is just aliasing. */
#define FREQ_COUNT ((SAMPLE_COUNT / 2u) + 1u)
#define HZ_PER_BUCKET ((SAMPLE_RATE / 2) / (FREQ_COUNT - 1))
static inline unsigned int to_bucket(float hz)
{
    return roundf(hz / HZ_PER_BUCKET);
}
static inline float to_frequency(float bucket)
{
    return HZ_PER_BUCKET * bucket;
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

/* Calculate the modulation percentage of these samples. */
static int mod_percent(uint16_t *samples, unsigned int count)
{
    unsigned int max = 0, min = -1;

    for (unsigned int i = 0; i < count; i++)
    {
        float s = samples[i];
        max = (s > max) ? s : max;
        min = (s < min) ? s : min;
    }

    /* N.B. *not* (100 * (max - min) / max), as you might expect. */
    return (int) roundf(100.0 * (max - min) / (max + min));
}

/* Measure a light source and report on it.
 * Returns false on error. */
static bool measure(void)
{
    float frequency;
    unsigned int cycle, mod;

    /* Collect uint16_t samples in [0, 0xfff]. */
    sample(SAMPLE_COUNT, SAMPLE_RATE, samples);

    /* Find the spectrum and the peak frequency. */
    if (!window(samples, f.real, f.imag, SAMPLE_COUNT)) {
        return false;
    }
    fft(f.real, f.imag, SAMPLE_COUNT);
    make_polar(f.real, f.imag, f.magnitude, f.phase, FREQ_COUNT);
    frequency = to_frequency(peak(f.magnitude, FREQ_COUNT));

    /* Look at the spectrum. */
    graph_logx(f.magnitude, FREQ_COUNT);
    printf("FFT: peak at %fHz\n", frequency);

    /* Look at a couple of cycles of the raw samples. */
    cycle = SAMPLE_RATE / frequency;
    if (cycle > SAMPLE_COUNT / 2) {
        cycle = SAMPLE_COUNT / 2;
    }
    graph(samples + SAMPLE_COUNT / 2 - cycle, cycle * 2);
    mod = mod_percent(samples + SAMPLE_COUNT / 2 - cycle, cycle * 2);
    printf("Raw samples: %dms, %d%% flicker.\n",
           (unsigned int)(2 * cycle / SAMPLE_RATE * 1000),
           mod);

    return true;
}

int main(void)
{
    /* Debugging metadata that gets baked into the binary. */
    bi_decl(bi_program_name("flicker"));
    bi_decl(bi_program_version_string("0.1"));
    bi_decl(bi_program_description("Lighting flicker meter"));

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
