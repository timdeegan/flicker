#pragma once

/* Handle assertion failure: different in unit test from the main build. */
extern void assertion_failure(const char *pred, const char *file, int line);

#define ASSERT(pred) do {                              \
    if (!(pred)) {                                     \
        assertion_failure(#pred, __FILE__, __LINE__);  \
    }                                                  \
} while(0)
