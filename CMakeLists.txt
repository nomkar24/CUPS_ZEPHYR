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

zephyr_compile_options(-U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)

target_include_directories(app PRIVATE
    ../modules/lib/pappl_zephyr
    ../modules/lib/cups
    ../modules/lib/pdfio
)

zephyr_link_libraries(
    -Wl,--wrap=shared_multi_heap_alloc
    -Wl,--wrap=shared_multi_heap_aligned_alloc
    -Wl,--wrap=shared_multi_heap_realloc
    -Wl,--wrap=shared_multi_heap_free
    -Wl,--wrap=strcmp
    -Wl,--wrap=strlen
    -Wl,--wrap=strcpy
    -Wl,--wrap=strncmp
    -Wl,--wrap=strchr
)
