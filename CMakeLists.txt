# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE
    tests/pappl/main.c
    tests/pappl/testpappl.c
    tests/pappl/pwg-driver.c
    tests/pappl/heap_wrap.c
)

target_compile_options(app PRIVATE -U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)

target_include_directories(app PRIVATE
    ../modules/lib/pappl_zephyr
    ../modules/lib/cups
    ../modules/lib/pdfio
)

target_link_options(app PRIVATE
    -Wl,--wrap=shared_multi_heap_alloc
    -Wl,--wrap=shared_multi_heap_aligned_alloc
    -Wl,--wrap=shared_multi_heap_realloc
    -Wl,--wrap=shared_multi_heap_free
)
