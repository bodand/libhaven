# libhaven project
#
# Copyright (c) 2022, András Bodor <bodand@proton.me>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# - Neither the name of the copyright holder nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# CMakeLists.txt --
#   The main CMake script for building and managing the libhaven library.

cmake_minimum_required(VERSION 3.22)
enable_language(C)
enable_language(CXX)

# vcpkg ########################################################################
# Get vcpkg, install required dependencies, and load the toolchain file.

if (WIN32)
    if (MSVC)
        set(VCPKG_TRIPLET x64-windows)
    else ()
        set(VCPKG_TRIPLET x64-mingw-dynamic)
        set(ENV{VCPKG_DEFAULT_HOST_TRIPLET} x64-mingw-dynamic)
    endif ()
    set(_VCPKG_BOOT_SUFFIX .bat)
    set(_VCPKG_SUFFIX .exe)
else ()
    set(_VCPKG_BOOT_SUFFIX .sh)
    set(_VCPKG_SUFFIX)
endif ()

if (VCPKG_TRIPLET)
    set(VCPKG_TRIPLET_CMD "--triplet=${VCPKG_TRIPLET}")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/${VCPKG_TRIPLET}")
else ()
    # ¯\_(ツ)_/¯
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/x86-windows") # maybe?
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/x64-windows")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/arm64-windows")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/x64-linux")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/installed/x64-osx")
    # unlikely any of the uwp targets to be used
endif ()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/vcpkg${_VCPKG_SUFFIX}")
    # Setup vcpkg
    file(DOWNLOAD
         https://github.com/microsoft/vcpkg/archive/refs/heads/master.tar.gz
         ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.tar.gz)
    file(ARCHIVE_EXTRACT INPUT
         ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.tar.gz)
    execute_process(COMMAND
                    ${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/bootstrap-vcpkg${_VCPKG_BOOT_SUFFIX}
                    COMMAND_ECHO STDOUT
                    OUTPUT_VARIABLE _ignore
                    ECHO_OUTPUT_VARIABLE)
endif ()

set(DEPS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.txt")
if (EXISTS "${DEPS_FILE}")
    file(SHA3_256 "${DEPS_FILE}" CURRENT_HASH)
    set(DEPS_HASH_FILE "${CMAKE_CURRENT_BINARY_DIR}/dependencies.txt.sha3")
    set(need_deps TRUE)
    if (EXISTS "${DEPS_HASH_FILE}")
        file(READ "${DEPS_HASH_FILE}" LAST_HASH)

        if (CURRENT_HASH STREQUAL LAST_HASH)
            set(need_deps FALSE)
        endif ()
    endif ()

    if (need_deps)
        # Get depdencies
        file(READ "${DEPS_FILE}" PKG_PROJECT_DEPENDENCIES)
        list(TRANSFORM PKG_PROJECT_DEPENDENCIES STRIP)
        execute_process(COMMAND
                        "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/vcpkg${_VCPKG_SUFFIX}"
                        install
                        ${VCPKG_TRIPLET_CMD}
                        ${PKG_PROJECT_DEPENDENCIES}
                        COMMAND_ECHO STDOUT
                        OUTPUT_VARIABLE _ignore
                        ECHO_OUTPUT_VARIABLE)
        file(WRITE "${DEPS_HASH_FILE}" "${CURRENT_HASH}")
    endif ()
endif ()

set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
message(STATUS "Using triplet: ${VCPKG_TRIPLET}")
message(STATUS "Using Toolchain: ${CMAKE_CURRENT_BINARY_DIR}/vcpkg-master/scripts/buildsystems/vcpkg.cmake")

# CMake policies ###############################################################
# Set CMake policies to the expected behavior.

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
endif ()
cmake_policy(SET CMP0127 NEW)
cmake_policy(SET CMP0125 NEW)
cmake_policy(SET CMP0118 NEW)
cmake_policy(SET CMP0115 NEW)
cmake_policy(SET CMP0111 NEW)
cmake_policy(SET CMP0110 NEW)
cmake_policy(SET CMP0096 NEW)
cmake_policy(SET CMP0092 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0048 NEW)

# project #########################################################################
project(libhaven
        VERSION 0.1.0
        DESCRIPTION "libhaven is an async I/O library based on completion ports"
        LANGUAGES CXX)

# Include modules
include(GNUInstallDirs)
include(CheckCXXSymbolExists)
include(CMakeDependentOption)

#include(cmake/simd.cmake)

# libraries
find_package(xsimd CONFIG REQUIRED)

# options
option(HAVEN_BUILD_TESTS "Build the have test suite" YES)
option(HAVEN_BUILD_BENCHMARKS "Build the have benchmark suite" YES)
cmake_dependent_option(HAVEN_MEMORY_PAGE_OFFER
                       "Allow offering unused pages back to the OS on compatible systems (Windows+MSVC)" YES
                       "WIN32 AND MSVC" NO)

## special debug options
option(HAVEN_DBG_PAGE_TRACE "Trace page allocations" YES)
option(HAVEN_DBG_PUDDLE_TRACE "Trace allocations in puddles" YES)
option(HAVEN_DBG_CHECK_CALLS "Validate pre/post-conditions for function calls inside libhaven." YES)
cmake_dependent_option(HAVEN_DBG_CHECK_PRE
                       "Check preconditions for libhaven function calls" YES
                       "HAVEN_DBG_CHECK_CALLS" NO)
cmake_dependent_option(HAVEN_DBG_CHECK_PRE_PARAMS
                       "If possible, after a failed precondition check output the list of parameters" YES
                       "HAVEN_DBG_CHECK_CALLS" NO)
cmake_dependent_option(HAVEN_DBG_CHECK_POST
                       "Check postconditions for libhaven function calls" YES
                       "HAVEN_DBG_CHECK_PRE" NO)
cmake_dependent_option(HAVEN_DBG_CHECK_POST_PARAMS
                       "If possible, after a failed postcondition check output the list of parameters" YES
                       "HAVEN_DBG_CHECK_POST" NO)

## debug flags target
add_library(haven-dbg INTERFACE)
target_compile_definitions(haven-dbg INTERFACE
                           $<$<BOOL:${HAVEN_DBG_PAGE_TRACE}>:HAVEN_DBG_PAGE_TRACE>
                           $<$<BOOL:${HAVEN_DBG_PUDDLE_TRACE}>:HAVEN_DBG_PUDDLE_TRACE>
                           $<$<BOOL:${HAVEN_DBG_CHECK_PRE}>:HAVEN_DBG_CHECK_PRE>
                           HAVEN_DBG_CHECK_PRE_V=0$<BOOL:${HAVEN_DBG_CHECK_PRE}>
                           HAVEN_DBG_CHECK_PRE_PARAMS_V=0$<BOOL:${HAVEN_DBG_CHECK_PRE_PARAMS}>
                           $<$<BOOL:${HAVEN_DBG_CHECK_PRE_PARAMS}>:HAVEN_DBG_CHECK_PRE_PARAMS>
                           $<$<BOOL:${HAVEN_DBG_CHECK_POST}>:HAVEN_DBG_CHECK_POST>
                           $<$<BOOL:${HAVEN_DBG_CHECK_POST_PARAMS}>:HAVEN_DBG_CHECK_POST_PARAMS>
                           HAVEN_DBG_CHECK_POST_V=0$<BOOL:${HAVEN_DBG_CHECK_POST}>
                           HAVEN_DBG_CHECK_POST_PARAMS_V=0$<BOOL:${HAVEN_DBG_CHECK_POST_PARAMS}>
                           )

# configs
if (WIN32)
    set(HAVEN_GENERIC_PLATFORM "win")

    check_cxx_symbol_exists("OfferVirtualMemory" "windows.h;memoryapi.h" HAVE_OFFER_VIRTUAL_MEMORY)
    check_cxx_symbol_exists("ReclaimVirtualMemory" "windows.h;memoryapi.h" HAVE_RECLAIM_VIRTUAL_MEMORY)

    if (HAVE_OFFER_VIRTUAL_MEMORY AND HAVE_RECLAIM_VIRTUAL_MEMORY)
        set(HAVEN_SPECIFIC_PLATFORM "win32")
    else ()
        set(HAVEN_SPECIFIC_PLATFORM "mingw")
    endif ()
else ()
    set(HAVEN_GENERIC_PLATFORM "unix")

    check_cxx_symbol_exists("sysctl" "sys/types.h;sys/sysctl.h" HAVE_SYSCTL)
    check_cxx_symbol_exists("sysconf" "unistd.h" HAVE_SYSCONF)
    check_cxx_symbol_exists("_SC_PAGESIZE" "unistd.h" HAVE_SC_PAGESIZE)

    if (HAVE_SYSCTL)
        set(HAVEN_SPECIFIC_PLATFORM "bsd")
    elseif (HAVE_SYSCONF AND HAVE_SC_PAGESIZE)
        set(HAVEN_SPECIFIC_PLATFORM "linux")
    endif ()
endif ()

add_subdirectory(src/haven/common)
add_subdirectory(src/haven/mem)

#add_library(haven_st STATIC
#            )
#add_library(haven_dyn SHARED
#            )
#add_library(haven::static ALIAS haven_st)
#add_library(haven::dynamic ALIAS haven_dyn)
#if (CMAKE_BUILD_DYNAMIC)
#    add_library(have::haven ALIAS haven_dyn)
#else ()
#    add_library(have::haven ALIAS haven_st)
#endif ()
#
#foreach (tgt haven_st haven_dyn)
#    target_include_directories(${tgt} PUBLIC
#                               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#                               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
#    target_compile_definitions(${tgt} PRIVATE
#                               $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
#                               $<$<PLATFORM_ID:Windows>:NOMINMAX>
#                               $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>)
#    target_compile_features(${tgt} PUBLIC cxx_std_20)
#endforeach ()
#
#target_link_libraries(haven_dyn PRIVATE
#                      $<$<PLATFORM_ID:Windows>:ws2_32>
#                      $<$<PLATFORM_ID:Windows>:mswsock>)
#target_link_libraries(haven_st PUBLIC
#                      $<$<PLATFORM_ID:Windows>:ws2_32>
#                      $<$<PLATFORM_ID:Windows>:mswsock>)

include(CTest)
if (HAVEN_BUILD_TESTS)
    find_package(ut CONFIG REQUIRED)
    add_subdirectory(test)
endif ()

if (HAVEN_BUILD_BENCHMARKS)
    find_package(nonius CONFIG REQUIRED)
    find_package(ut CONFIG REQUIRED)
    add_subdirectory(benchmark)
endif ()
