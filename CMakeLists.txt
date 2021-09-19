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
)
add_executable(flicker ${FLICKER_SOURCES})

# Turn up warnings; turn off UB footguns.
set_source_files_properties(${FLICKER_SOURCES} PROPERTIES COMPILE_OPTIONS
  "-Wall;-Wextra;-Werror;-fanalyzer;-fno-strict-aliasing;-fwrapv"
)

# Turn warnings up as far as the SDK will stand.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare")
# Turn off sharp-edged UB-based optimizations.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fwrapv")

# Add the SDK library.
target_link_libraries(flicker pico_stdlib)

# Use the USB console.
pico_enable_stdio_usb(flicker 1)
# ...and don't try to drive UART on GP0/1
pico_enable_stdio_uart(flicker 0)

# Build the full set of output files.
pico_add_extra_outputs(flicker)
