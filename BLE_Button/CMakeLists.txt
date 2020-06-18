# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13)

# Mbed OS system root path - that probably needs to come from tools
# TODO: @mbed-os-tools
set(MBED_OS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/mbed-os CACHE INTERNAL "")

# Include standard Mbed definitions
include(${MBED_OS_ROOT}/cmake/app.cmake)

project(BLE_Button)

# uncomment below to have a verbose build process
#set(CMAKE_VERBOSE_MAKEFILE ON)

# Sources for main application
target_sources(app
    PRIVATE
        source/main.cpp
)

target_include_directories(mbed-os
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

add_subdirectory(shields)
