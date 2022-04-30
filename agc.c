#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/pio.h"

#include "agc.h"
#include "assertions.h"
#include "sample.h"

#include "ad5220.pio.h"

/* Which PIO hardware will we use? */
static PIO pio;
static unsigned int sm;
static unsigned int offset;

/* Where is the wiper set on the AD5220? (0 to 128) */
static unsigned int cursor;

/* We want the peak of the measured waveform to be at this level:
 * high enough to use the ADC range but not so high that we clip. */
#define AGC_TARGET 2800

/* Measurements above this level are not really linear - the current
 * is limited by the resistor more than by the phototransistor. */
#define AGC_CEILING 3600

/* Measurements below this level are not really linear either, as
 * the DAC's internal offsets become noticeable. */
 #define AGC_FLOOR 500

/* Set up the AGC hardware. */
void agc_init(unsigned int dir_pin, unsigned int clock_pin)
{
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    offset = pio_add_program(pio, &ad5220_program);
    ad5220_program_init(pio, sm, offset, dir_pin, clock_pin);
    agc_reset();
}

/* Reset the AGC to a known, safe state. */
void agc_reset(void)
{
    /* The potentiometer has 128 possible states; asking for 127 up-ticks
     * puts it in the highest resistance regardless of where we start. */
    ad5220_program_run(pio, sm, 127);
    cursor = 127;
}

/* Set the potentiometer to a particular level. */
void agc_set_level(unsigned int level)
{
    ASSERT(level < 128);
    ad5220_program_run(pio, sm, level - cursor);
    cursor = level;
}

/* Find the peak brightness. */
static uint16_t measure_peak(uint16_t *buffer)
{
    unsigned int i;
    uint16_t max = 0;
    sample(AGC_SAMPLE_COUNT, AGC_SAMPLE_RATE, buffer);
    for (i = 0; i < AGC_SAMPLE_COUNT; i++) {
        if (!(buffer[i] & SAMPLE_ERROR) && buffer[i] > max) {
            max = buffer[i];
        }
    }
    return max;
}

/* Adjust the gain so that the waveform fits into the ADC's range.
 * @buffer must be at least AGC_SAMPLE uint16_ts long,
 * and will be overwritten. */
void agc_run(uint16_t *buffer)
{
    uint16_t peak;

    /* This usually converges in two or three cycles.
     * Run four for safety and so the last peak measurement
     * is likely to be representative of the actual levels. */
    for (unsigned int i = 0; i < 4; i++) {
        /* Where are we now? */
        peak = measure_peak(buffer);

        /* In the mid-range, the phototransistor current is proportional
         * to the brightness, and the measured voltage is proportional to
         * that and to the resistance (V = IR).  Adjust the resistance
         * to bring the peak measurement to the target. */
        float ohms = AGC_OHMS(cursor);
        float new_ohms = ohms * AGC_TARGET / peak;
        int new_level = roundf(AGC_LEVEL(new_ohms));

        /* If we're above the linear range then the linear model
         * will adjust too slowly, so do something more dramatic. */
        if (peak > AGC_CEILING) {
            new_level = cursor / 10;
        }

        /* Don't go outside the range of the AD5220. */
        if (new_level < 0) {
            new_level = 0;
        }
        if (new_level > 127) {
            new_level = 127;
        }

        printf("AGC round %d: peak %d, %d -> %d\n",
            i, peak, cursor, new_level);

        agc_set_level(new_level);
    }

    printf("AGC: %d/127%s\n", cursor,
            (peak > AGC_CEILING) ? " (TOO BRIGHT)" :
            (peak < AGC_FLOOR) ? " (TOO DARK)" : "");
}
