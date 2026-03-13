# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE tests/libcups/main.c tests/libcups/net_sample_common.c)
target_compile_options(app PRIVATE -U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)
target_include_directories(app PRIVATE ../modules/lib/pappl ../modules/lib/cups)
