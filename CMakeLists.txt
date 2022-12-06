cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(RP6502 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include_directories(tinyusb_kb)
add_subdirectory(tinyusb_kb)
add_subdirectory(src)
