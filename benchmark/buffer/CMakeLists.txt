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
# benchmark/ffs vs ringbuffer/CMakeLists.txt --
#   A benchmark agains the ffs based implementation that does the bookkeeping
#   of free slots using ffs on a set of bits, and the other is a ringbuffer
#   based model.
#   They are benchmarked for the following operations:
#     - insertion to empty store
#     - insertion to half-filled store
#     - insertion to full store

## mx_buffer implementation & test #############################################
add_library(mx_buffer OBJECT mx_buffer.hxx mx_buffer.cxx)
target_compile_features(mx_buffer PUBLIC cxx_std_20)

add_executable(mx-test mx_buffer_test.cxx)
target_link_libraries(mx-test PRIVATE mx_buffer Boost::ut)
target_compile_definitions(mx-test PRIVATE
                           BOOST_UT_DISABLE_MODULE)
add_test(NAME MutexBuffer
         COMMAND mx-test)

## rw_buffer implementation & test #############################################
add_library(rw_buffer OBJECT rw_buffer.hxx rw_buffer.cxx)
target_compile_features(rw_buffer PUBLIC cxx_std_20)

add_executable(rw-test rw_buffer_test.cxx)
target_link_libraries(rw-test PRIVATE rw_buffer Boost::ut)
target_compile_definitions(rw-test PRIVATE
                           BOOST_UT_DISABLE_MODULE)
add_test(NAME RWLockBuffer
         COMMAND rw-test)

## ffs_buffer implementation & test ############################################
add_library(ffs_buffer OBJECT ffs_buffer.hxx ffs_buffer.cxx)
target_compile_features(ffs_buffer PUBLIC cxx_std_20)

add_executable(ffs-test ffs_buffer_test.cxx)
target_link_libraries(ffs-test PRIVATE ffs_buffer Boost::ut)
target_compile_definitions(ffs-test PRIVATE
                           BOOST_UT_DISABLE_MODULE)
add_test(NAME FFSBuffer
         COMMAND ffs-test)

## ringbuffer implementation & test ############################################
# lmao

## benchmarks ##################################################################
add_executable(buffer_insert insert.cxx)
target_link_libraries(buffer_insert PRIVATE
                      ffs_buffer
                      mx_buffer
                      rw_buffer
                      Nonius::nonius)
target_compile_definitions(buffer_insert PRIVATE
                           $<$<CXX_COMPILER_ID:MSVC>:_HAS_DEPRECATED_RESULT_OF>)
add_executable(buffer_half_insert half_insert.cxx)
target_link_libraries(buffer_half_insert PRIVATE
                      ffs_buffer
                      mx_buffer
                      rw_buffer
                      Nonius::nonius)
target_compile_definitions(buffer_half_insert PRIVATE
                           $<$<CXX_COMPILER_ID:MSVC>:_HAS_DEPRECATED_RESULT_OF>)
add_executable(buffer_full_insert half_insert.cxx)
target_link_libraries(buffer_full_insert PRIVATE
                      ffs_buffer
                      mx_buffer
                      rw_buffer
                      Nonius::nonius)
target_compile_definitions(buffer_full_insert PRIVATE
                           $<$<CXX_COMPILER_ID:MSVC>:_HAS_DEPRECATED_RESULT_OF>)

add_custom_target(buffer-benchmarks
                  COMMAND buffer_insert -r html -o insert.html -t "Empty Insertion" -s 500
                  COMMAND buffer_half_insert -r html -o half_insert.html -t "Half-filled Insertion" -s 500
                  COMMAND buffer_full_insert -r html -o full_insert.html -t "Full-filled Insertion" -s 500
                  )
