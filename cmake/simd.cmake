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
# cmake/simd.cmake --
#   A CMake script for figuring out which vector instruction sets are available
#   on the current computer.
#   Adds all required flags into the haven::vec INTERFACE library.

include(CheckCXXCompilerFlag)

add_library(haven_vec INTERFACE)
add_library(haven::vec ALIAS haven_vec)

check_cxx_compiler_flag("/arch:IA32" HAVE_ARCH_IA32)
check_cxx_compiler_flag("/arch:SSE" HAVE_ARCH_SSE)
check_cxx_compiler_flag("/arch:SSE2" HAVE_ARCH_SSE2)
check_cxx_compiler_flag("/arch:AVX" HAVE_ARCH_AVX)
check_cxx_compiler_flag("/arch:AVX2" HAVE_ARCH_AVX2)
check_cxx_compiler_flag("/arch:AVX512" HAVE_ARCH_AVX512)
check_cxx_compiler_flag("-msse" HAVE_MSSE)
check_cxx_compiler_flag("-msse2" HAVE_MSSE2)
check_cxx_compiler_flag("-msse3" HAVE_MSSE3)
check_cxx_compiler_flag("-mssse3" HAVE_MSSSE3)
check_cxx_compiler_flag("-msse4.1" HAVE_MSSE4_1)
check_cxx_compiler_flag("-msse4.2" HAVE_MSSE4_2)
check_cxx_compiler_flag("-msse4" HAVE_MSSE4)
check_cxx_compiler_flag("-mavx" HAVE_MAVX)
check_cxx_compiler_flag("-mavx2" HAVE_MAVX2)
check_cxx_compiler_flag("-m64" HAVE_M64)

target_compile_options(haven_vec INTERFACE
                       $<$<BOOL:${HAVE_ARCH_IA32}>:/arch:IA32>
                       $<$<BOOL:${HAVE_ARCH_SSE}>:/arch:SSE>
                       $<$<BOOL:${HAVE_ARCH_SSE2}>:/arch:SSE2>
                       $<$<BOOL:${HAVE_ARCH_AVX}>:/arch:AVX>
                       $<$<BOOL:${HAVE_ARCH_AVX2}>:/arch:AVX2>
                       $<$<BOOL:${HAVE_MSSE}>:-msse>
                       $<$<BOOL:${HAVE_MSSE2}>:-msse2>
                       $<$<BOOL:${HAVE_MSSE2}>:-msse2>
                       $<$<BOOL:${HAVE_MSSE3}>:-msse3>
                       $<$<BOOL:${HAVE_MSSSE3}>:-mssse3>
                       $<$<BOOL:${HAVE_MSSE4_1}>:-msse4.1>
                       $<$<BOOL:${HAVE_MSSE4_2}>:-msse4.2>
                       $<$<BOOL:${HAVE_MSSE4}>:-msse4>
                       $<$<BOOL:${HAVE_MAVX}>:-mavx>
                       $<$<BOOL:${HAVE_MAVX2}>:-mavx2>
                       $<$<BOOL:${HAVE_M64}>:-m64>)
