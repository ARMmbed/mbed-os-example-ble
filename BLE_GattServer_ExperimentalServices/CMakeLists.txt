# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19.0 FATAL_ERROR)

set(MBED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mbed-os CACHE INTERNAL "")
set(MBED_CONFIG_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")
set(APP_TARGET BLE_GattServer_ExperimentalServices)

include(${MBED_PATH}/tools/cmake/app.cmake)

project(${APP_TARGET})

add_subdirectory(${MBED_PATH})

add_subdirectory(mbed-os-ble-utils)
add_subdirectory(mbed-os-experimental-ble-services/services/CurrentTime)
add_subdirectory(mbed-os-experimental-ble-services/services/LinkLoss)

add_executable(${APP_TARGET})

target_sources(${APP_TARGET}
    PRIVATE
        source/main.cpp
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        mbed-os
        mbed-events
        mbed-ble
        mbed-ble-utils
        ble-service-link-loss
        ble-service-current-time
)

mbed_set_post_build(${APP_TARGET})

option(VERBOSE_BUILD "Have a verbose build process")
if(VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
