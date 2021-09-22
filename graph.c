#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "assertions.h"
#include "graph.h"

/* Plotting into a small framebuffer.
 * Frame bits are top to bottom, left to right.
 * That might change when we have an actual display. */
#define WIDTH 80u
#define HEIGHT 20u
static uint8_t frame[WIDTH * HEIGHT / 8];

/* Print a full-width horizontal line. */
static void horizontal_line(void)
{
    for (unsigned int i = 0; i < WIDTH; i++) {
        putchar('-');
    }
    putchar('\n');
}

/* Print the frame on the serial console. */
static void print_frame()
{
    unsigned int bit, i;

    horizontal_line();
    for (i = 0; i < WIDTH * HEIGHT; i++) {
        bit = frame[i / 8] >> (i % 8) & 1u;
        putchar(bit ? '*' : ' ');
        if (i % WIDTH == WIDTH - 1) {
            putchar('\n');
        }
    }
    horizontal_line();
}

/* Graph 16-bit samples on a linear scale. */
void graph(const char *name, uint16_t *samples, unsigned int count)
{
    unsigned int i, x, y, bit;
    uint16_t max;

    /* Avoid some overflows below. */
    ASSERT(count < (1u << 16));

    /* Find our Y-axis scale. */
    max = 0;
    for (i = 0; i < count; i++) {
        if (samples[i] > max) {
            max = samples[i];
        }
    }

    /* Figure out the pixels. */
    memset(frame, 0, sizeof frame);
    for (i = 0; i < count; i++) {
        x = i * WIDTH / count;
        y = samples[i] * HEIGHT / (max + 1);
        ASSERT(x < WIDTH);
        ASSERT(y < HEIGHT);
        bit = (HEIGHT - 1 - y) * WIDTH + x;
        frame[bit / 8] |= 1u << (bit % 8);
    }

    print_frame();
    printf("[ %s | X: 0 - %d | Y: 0 - %d ]\n", name, count, max);
}
