#pragma once

/* Set up the AGC hardware. */
void agc_init(unsigned int dir_pin, unsigned int clock_pin);

/* Reset the AGC to a known, safe state. */
extern void agc_reset(void);

/* Set the potentiometer to a particular level.
 * Only useful for testing. */
extern void agc_set_level(unsigned int level);

/* The total resistance in the test circuit is some fraction of the
 * 10k potentiometer, plus its 'wiper' resistance of 40R (+/- 12)
 * plus a fixed 680R (+/- 2%) for safety. */
 #define FIXED_OHMS 720.0
 #define AGC_OHMS(level) (FIXED_OHMS + 1e4 * level / 127)
