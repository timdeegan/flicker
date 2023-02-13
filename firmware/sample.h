#pragma once

#include <stdint.h>

/* Set up the ADC hardware once at boot time. */
extern void sample_init(unsigned int pin);

/* Take @count ADC samples at @hz Hz.
 * Blocks until sampling is complete. */
extern void sample(unsigned int count, float hz, uint16_t *dest);

/* Samples with this bit set were ADC errors. */
#define SAMPLE_ERROR ((uint16_t) 0x8000)
