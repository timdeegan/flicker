#pragma once

/* Set up the AGC hardware. */
void agc_init(unsigned int dir_pin, unsigned int clock_pin);

/* Reset the AGC to a known, safe state. */
extern void agc_reset(void);

/* Adjust the gain so that the waveform fits into the ADC's range.
 * @buffer must be at least AGC_SAMPLE uint16_ts long,
 * and will be overwritten. */
extern void agc_run(uint16_t *buffer);

/* Internal sampling for the AGC: 20ms,
 * long enough to catch a cycle of 50Hz */
#define AGC_SAMPLE_RATE 250000
#define AGC_SAMPLE_COUNT 5000

/* Set the potentiometer to a particular level.
 * Only useful for testing. */
extern void agc_set_level(unsigned int level);

/* The total resistance in the test circuit is some fraction of the
 * 10k potentiometer, plus its 'wiper' resistance of 40R (+/- 12)
 * plus a fixed 680R (+/- 2%) for safety. */
#define FIXED_OHMS 720.0
#define AGC_OHMS(level) (FIXED_OHMS + 1e4 * level / 127)
#define AGC_LEVEL(ohms) ((ohms - FIXED_OHMS) / (1e4 / 127))
