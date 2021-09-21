cmake_minimum_required(VERSION 3.13)

# Invoke the Pico SDK's CMake runes.
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
project(flicker C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

# Our sources.
set(FLICKER_SOURCES
  main.c
  fft.c
  sample.c
)
add_executable(flicker ${FLICKER_SOURCES})

set(TEST_SOURCES
  tests/tests.c
  fft.c
  sample.c
)
add_executable(unit-tests ${TEST_SOURCES})

# Turn up warnings; turn off UB footguns.
set(OUR_SOURCES ${FLICKER_SOURCES} ${TEST_SOURCES})
set_source_files_properties(${OUR_SOURCES} PROPERTIES COMPILE_OPTIONS
  "-Wall;-Wextra;-Werror;-fanalyzer;-fno-strict-aliasing;-fwrapv"
)

# Add the SDK library.
set(SDK_LIBS
  pico_stdlib
  hardware_adc
  hardware_dma
)
target_link_libraries(flicker ${SDK_LIBS})
target_link_libraries(unit-tests ${SDK_LIBS})

# Use the USB console.
pico_enable_stdio_usb(flicker 1)
pico_enable_stdio_usb(unit-tests 1)
# ...and don't try to drive UART on GP0/1
pico_enable_stdio_uart(flicker 0)
pico_enable_stdio_uart(unit-tests 0)

# Build the full set of output files.
pico_add_extra_outputs(flicker)
pico_add_extra_outputs(unit-tests)
