from matplotlib import pyplot as plt
import numpy as np

# Compute the bit-reverse of an N-bit input
# (ARM RBIT instruction may be useful here?)
def bit_reverse(input, N):
    result = input & 1
    for _ in range(N - 1):
        result <<= 1
        input >>= 1
        result |= (input & 1)
    return result

# Bit-reverse shuffle an array of 2^N entries in place.
def bit_reverse_shuffle(samples, N):
    assert len(samples) == 2**N, f'bad array length, {len(samples)} != {2**N} (2^{N})'
    for i in range(2**N):
        j = bit_reverse(i, N)
        if i < j:
            t = samples[i]
            samples[i] = samples[j]
            samples[j] = t

# Radix-2, time decimation FFT.
# Inputs are real-valued; input length must be 2^N.
#
# very good match to numpy for float64, float32
# float16 works for small but overflows for big.
# int64, int32 work for big but not for small (underflow)
# int16 is bad for big as well (overflow)
#
# TODO: can we do a fixed-point FFT with appropriate scaling?
# TODO: optimize for real-valued FFT being symmetrical?
# TODO: replace bit-reverse shuffle with per-pass strides?
# TODO: small optimizations like calculating angles by addition.
#
def fft(input, N):
    length = len(input)
    assert length == 2**N, f'bad input length, {length} != {2**N} (2^{N})'

    # The radix-2 FFT is a recursive algorithm that breaks a
    # DFT of 2^N entries into two DFTs each of 2^(N-1) entries.
    # It combines the results of the sub-DFTs using a set of
    # 'butterfly' multiply-and-add operations.
    #
    # In order to operate in place, the recursive traversal
    # is actually implemented breadth-first.  Conceptually
    # we do 2^N 1-entry DFTs (which are noops), then combine
    # them into 2^(N-1) 2-entry DFTs, and so on until we have
    # one 2^N-entry DFT.
    # 
    # Each recursive step would have split the inputs into even
    # and odd entries.  We can avoid shuffling between stages by
    # shuffling once first.
    real = input.copy()
    bit_reverse_shuffle(real, N)

    # No need to shuffle the imaginary parts, which are all zeros.
    imag = np.zeros(len(real), real.dtype)

    #for (sub_length = 2; sub_length <= 2**N; sub_length *= 2):
    for n in range(1, N + 1):
        sub_length = 2**n
        # In this pass we are merging smaller DFTs into DFTs
        # of length sub_length.  This should look like:
        # for (base = 0; base < length; base += sub_length):
        #     for (step = 0; step < sub_length // 2; step++):
        #         calculate 'twiddle factor' for this step
        #         merge [base+step] with [base+(sub_length/2)+step]
        # but calculating twiddle factors is expensive so 
        # we invert the inner two loops so we can reuse them.
        for step in range(sub_length // 2):
            # "twiddle factor" of e^(-2*pi*i*step/sub_length)
            twiddle_angle = -2 * np.pi * step / sub_length
            twiddle_sin = np.sin(twiddle_angle)
            twiddle_cos = np.cos(twiddle_angle)
            for base in range(0, length, sub_length):
                # Load the two entries that we're going to merge.
                A_index = base + step
                A_real = real[A_index]
                A_imag = imag[A_index]
                B_index = A_index + (sub_length // 2)
                B_real = real[B_index]
                B_imag = imag[B_index]
                # Butterfly.  This is equivalent to taking
                # complex A and B and twiddle T and calculating
                # A' = A + TB
                # B' = A - TB
                TB_real = B_real * twiddle_cos - B_imag * twiddle_sin
                TB_imag = B_imag * twiddle_cos + B_real * twiddle_sin
                real[A_index] = (A_real + TB_real)
                imag[A_index] = (A_imag + TB_imag)
                real[B_index] = (A_real - TB_real)
                imag[B_index] = (A_imag - TB_imag)

    # Return a complex-valued numpy array to match the usual API
    return real + 1j * imag

# Size of inputs.
power_of_two = 12
number_of_samples = 2**power_of_two

# Format a 1D array for C.
def c_decl(name, data):
    decl = f'static const float {name}[{len(data)}]'
    init = '{' + ', '.join(data.astype(str)) + '}'
    return f'{decl} = {init};\n'

# Test our code on one set of input data,
# plot the answers and save the test patterns.
def compare(name, data):
    reference = np.fft.rfft(data.astype(np.float64))
    ours64 = fft(data.astype(np.float64), power_of_two)
    ours32 = fft(data.astype(np.float32), power_of_two)

    status = 'OK'
    # FFT of real-valued inputs should be symmetrical apart from DC.
    if not np.allclose(np.conj(ours64[-1:0:-1]), ours64[1:]):
        status = 'NOT SYMMETRICAL'
    # 64bit should match numpy closely.
    if not np.allclose(ours64[:len(ours64) // 2 + 1], reference, 1e-5):
        status = 'DOES NOT MATCH NUMPY'
    # 32bit should match 64bit within reason.
    if not np.allclose(ours64, ours32, 1e-2, 1e-3):
        status = '32 DOES NOT MATCH 64'

    plt.clf()
    plt.plot(np.abs(reference), 'x')
    plt.plot(np.abs(ours64), '+')
    plt.title(f'{name} - {status}')
    plt.show()

    with open(f'fft-test-{name}.h', 'w') as fh:
        fh.write('AUTOGENERATED FILE -- DO NOT EDIT\n')
        fh.write(c_decl(f'{name}_input', data))
        fh.write(c_decl(f'{name}_real', np.real(ours32)))
        fh.write(c_decl(f'{name}_imag', np.imag(ours32)))
        fh.write(f'fft_test("{name}", {len(data)}, '
                 f'{name}_input, {name}_real, {name}_imag);\n')

# Single frequency
cosine = np.cos(np.linspace(0, np.pi * 8, number_of_samples))
compare('cosine', cosine)

# Pseudorandom noise
noise = np.random.RandomState(700).random(number_of_samples) - 0.5
compare('noise', noise)

# Square wave, +/- 1
square_wave = np.array([((x >> 7) & 2) - 1 for x in range(number_of_samples)])
compare('square', square_wave)

# Square wave, +/- 2^12
big_square_wave = (square_wave * (2**12)).astype(np.int16)
compare('bigsquare', big_square_wave)

# Sawtoooth, +/- 1
sawtooth = ((np.linspace(0, 256, number_of_samples) % 1) * 2) -1
compare('sawtooth', sawtooth)

# Sawtoooth, +/- 2^12
big_sawtooth = (sawtooth * (2**12)).astype(np.int16)
compare('bigsawtooth', big_sawtooth)
