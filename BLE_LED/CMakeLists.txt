# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18.2 FATAL_ERROR)

set(APP_TARGET BLE_LED)

include(${MBED_ROOT}/tools/cmake/app.cmake)

add_subdirectory(${MBED_ROOT})

add_executable(${APP_TARGET})

mbed_configure_app_target(${APP_TARGET})

mbed_set_mbed_target_linker_script(${APP_TARGET})

project(${APP_TARGET})

target_include_directories(${APP_TARGET}
    PRIVATE
        ./source
)

target_sources(${APP_TARGET}
    PRIVATE
        source/main.cpp
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        mbed-os
        mbed-events
        mbed-ble
        mbed-ble-cordio
        mbed-ble-blue_nrg
)

mbed_generate_bin_hex(${APP_TARGET})

option(VERBOSE_BUILD "Have a verbose build process")
if(VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
