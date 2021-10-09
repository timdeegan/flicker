#include <stdbool.h>

#include "hardware/pio.h"

#include "assertions.h"

#include "ad5220.pio.h"

/* Which PIO hardware will we use? */
static PIO pio;
static unsigned int sm;
static unsigned int offset;

/* Where is the wiper set on the AD5220? (0 to 128) */
static unsigned int cursor;

/* Set up the AGC hardware. */
void agc_init(unsigned int dir_pin, unsigned int clock_pin)
{
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    offset = pio_add_program(pio, &ad5220_program);
    ad5220_program_init(pio, sm, offset, dir_pin, clock_pin);
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