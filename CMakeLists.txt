cmake_minimum_required(VERSION 3.13)

# Invoke the Pico SDK's CMake runes.
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
project(flicker C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

# Our sources.
add_executable(flicker
  main.c
  fft.c
)

# Some UB-based optimizations are more trouble than they're worth.
target_compile_options(flicker PRIVATE -fno-strict-aliasing -fwrapv)

# Add the SDK library.
target_link_libraries(flicker pico_stdlib)

# Use the USB console.
pico_enable_stdio_usb(flicker 1)
# ...and don't try to drive UART on GP0/1
pico_enable_stdio_uart(flicker 0)

# Build the full set of output files.
pico_add_extra_outputs(flicker)
