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
# benchmark/pool/common --
#   Implementation of a page-granular allocator using the raw system allocator
#   interfaces.

set(allocator_generic_platform "page-allocator.${HAVEN_GENERIC_PLATFORM}.cxx")
set(allocator_specific_platform "page-allocator.${HAVEN_SPECIFIC_PLATFORM}.cxx")
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${allocator_generic_platform}")
    unset(allocator_generic_platform)
endif ()
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${allocator_specific_platform}")
    unset(allocator_specific_platform)
endif ()

add_library(page-allocator STATIC
            page-allocator.hxx
            ${allocator_generic_platform}
            ${allocator_specific_platform}
            check_conditions.hxx check_conditions.cxx)
target_link_libraries(page-allocator PUBLIC
                      haven-dbg)
target_include_directories(page-allocator PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_features(page-allocator PUBLIC cxx_std_20)

