cmake_minimum_required(VERSION 3.12)

# IIDX Pico builds on PICO_W, but supports PICO as well
set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(iidx_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_subdirectory(src)
