#pragma once

/* Set up the AGC hardware. */
void agc_init(unsigned int dir_pin, unsigned int clock_pin);

/* Reset the AGC to a known, safe state. */
extern void agc_reset(void);

/* Set the potentiometer to a particular level.
 * Only useful for testing. */
extern void agc_set_level(unsigned int level);
