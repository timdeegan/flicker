#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assertions.h"
#include "graph.h"

/* We plot into a small framebuffer.
 * Frame bits are left to right, top to bottom.
 * That might change when we have an actual display. */
#define WIDTH 80u
#define HEIGHT 20u
static uint8_t frame[WIDTH * HEIGHT / 8];

/* Cursor */
static unsigned int cx, cy;

/* Set a pixel at these coordinates. */
static void set_pixel(unsigned int x, unsigned int y)
{
    unsigned int bit = (HEIGHT - 1 - y) * WIDTH + x;
    frame[bit / 8] |= 1u << (bit % 8);
}

/* Move the cursor to (x, y) and set the pixel there. */
static void skip_to(unsigned int x, unsigned int y)
{
    set_pixel(x, y);
    cx = x;
    cy = y;
}

/* Move the cursor to (x, y), filling in all pixels on the way. */
static void plot_to(unsigned int x, unsigned int y)
{
    int xrange = x - cx;
    int yrange = y - cy;
    int xsteps = abs(xrange);
    int ysteps = abs(yrange);
    int steps = (xsteps > ysteps) ? xsteps : ysteps;
    unsigned int px, py;

    for (int i = 1; i <= steps; i++) {
        px = cx + ((xrange * i + steps / 2) / steps);
        py = cy + ((yrange * i + steps / 2) / steps);
        set_pixel(px, py);
    }

    cx = x;
    cy = y;
}

/* Print a full-width horizontal line. */
static void print_line(void)
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

    print_line();
    for (i = 0; i < WIDTH * HEIGHT; i++) {
        bit = frame[i / 8] >> (i % 8) & 1u;
        putchar(bit ? '*' : ' ');
        if (i % WIDTH == WIDTH - 1) {
            putchar('\n');
        }
    }
    print_line();
}

/* Graph 16-bit samples on a linear scale. */
void graph(const char *name, uint16_t *samples, unsigned int count)
{
    unsigned int i, x, y;
    uint16_t max;

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
        x = ((uint64_t) i) * WIDTH / count;
        y = samples[i] * HEIGHT / (max + 1);
        ASSERT(x < WIDTH);
        ASSERT(y < HEIGHT);
        if (i == 0) {
            skip_to(x, y);
        } else {
            plot_to(x, y);
        }
    }

    print_frame();
    printf("[ %s ]\n", name);
}

/* Graph floating-point samples on a log-x/linear-y scale. */
void graph_logx(const char *name, float *samples, unsigned int count)
{
    unsigned int i, x, y;
    float max, log_count;

    /* Find our Y-axis scale. */
    max = 0;
    for (i = 0; i < count; i++) {
        if (samples[i] > max) {
            max = samples[i];
        }
    }
    log_count = log2f(count);

    /* Figure out the pixels. */
    memset(frame, 0, sizeof frame);
    for (i = 0; i < count; i++) {
        x = roundf(log2f(i) / log_count * (WIDTH - 1));
        y = roundf(samples[i] / max * (HEIGHT - 1));
        ASSERT(x < WIDTH);
        ASSERT(y < HEIGHT);
        if (i == 0) {
            skip_to(x, y);
        } else {
            plot_to(x, y);
        }
    }

    print_frame();
    printf("[ %s ]\n", name);
}
