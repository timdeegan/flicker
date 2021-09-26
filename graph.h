#pragma once

#include <stdint.h>

/* Graph 16-bit samples on a linear scale. */
void graph(const char *name, uint16_t *samples, unsigned int count);

/* Graph floating-point samples on a log-x/linear-y scale. */
void graph_logx(const char *name, float *samples, unsigned int count);
