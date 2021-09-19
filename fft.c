#include <assert.h>
#include <string.h>

#include "pico/float.h"

#include "fft.h"

/* Compute the bit-reverse of an N-bit input.
 * TODO: ARM RBIT instruction? */
static unsigned int bit_reverse(unsigned int input, unsigned int N)
{
    unsigned int result = input & 1u;
    while (N-- > 1) {
        result <<= 1;
        input >>= 1;
        result |= (input & 1u);
    }
    return result;
}

/* Bit-reverse shuffle an array of 2^N entries in place. */
static void bit_reverse_shuffle(float *samples, unsigned int N)
{
    unsigned int count = 1u << N;
    for (unsigned int i = 0; i < count; i++) {
        unsigned int j = bit_reverse(i, N);
        if (i < j) {
            float t = samples[i];
            samples[i] = samples[j];
            samples[j] = t;
        }
    }
}

/* In-place radix-2 time-decimation FFT.
 * Input/output array length must must be a power of 2. */
void fft(float *real, float *imag, unsigned int length)
{
    /* Length must be 2^N. */
    assert(length != 0);
    assert((length & (length - 1)) == 0);
    /* Find N, which is the bit-width of our array offsets. */
    unsigned int N = __builtin_ctz(length);

    /* The radix-2 FFT is a recursive algorithm that breaks a
     * DFT of 2^N entries into two DFTs each of 2^(N-1) entries.
     * It combines the results of the sub-DFTs using a set of
     * 'butterfly' multiply-and-add operations.
     *
     * In order to operate in place, the recursive traversal
     * is actually implemented breadth-first.  Conceptually
     * we do 2^N 1-entry DFTs (which are noops), then combine
     * them into 2^(N-1) 2-entry DFTs, and so on until we have
     * one 2^N-entry DFT.
     * 
     * Each recursive step would have split the inputs into even
     * and odd entries.  We can avoid shuffling between stages by
     * shuffling once first. */
    bit_reverse_shuffle(real, N);
    bit_reverse_shuffle(imag, N);

    for (unsigned int sub_length = 2; sub_length <= length; sub_length *= 2) {
        /* In this pass we are merging smaller DFTs into DFTs
         * of length sub_length.  This should look like:
         * for (base = 0; base < length; base += sub_length):
         *     for (step = 0; step < sub_length / 2; step++):
         *         calculate 'twiddle factor' for this step
         *         merge [base+step] with [base+(sub_length/2)+step]
         * but calculating twiddle factors is expensive so 
         * we invert the inner two loops so we can reuse them. */
        for (unsigned int step = 0; step < sub_length / 2; step++) {
            /* Calculate "twiddle factor" e^(-2*pi*i*step/sub_length). */
            float twiddle_angle = -(float)M_TWOPI * step / sub_length;
            float twiddle_sin, twiddle_cos;
            sincosf(twiddle_angle, &twiddle_sin, &twiddle_cos);
            for (unsigned int base = 0; base < length; base += sub_length) {
                /* Load the two entries that we're going to merge. */
                unsigned int A_index = base + step;
                float A_real = real[A_index];
                float A_imag = imag[A_index];
                unsigned int B_index = A_index + (sub_length / 2);
                float B_real = real[B_index];
                float B_imag = imag[B_index];
                /* Butterfly.  This is equivalent to taking
                 * complex A and B and twiddle T and calculating
                 * A' = A + TB
                 * B' = A - TB */
                float TB_real = B_real * twiddle_cos - B_imag * twiddle_sin;
                float TB_imag = B_imag * twiddle_cos + B_real * twiddle_sin;
                real[A_index] = (A_real + TB_real);
                imag[A_index] = (A_imag + TB_imag);
                real[B_index] = (A_real - TB_real);
                imag[B_index] = (A_imag - TB_imag);
            }
        }
    }
}
