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
# src/haven/common/CMakeLists --
#   Common utilities used by libhaven that cannot be separated into meaningful
#   units on their own.

project(libhaven-common
        VERSION 1.0)

add_library(haven_common STATIC check_conditions.cxx)
add_library(haven::common ALIAS haven_common)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH haven_dir)
cmake_path(GET haven_dir PARENT_PATH src_dir)

target_include_directories(haven_common PUBLIC
                           $<BUILD_INTERFACE:${src_dir}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(haven_common PUBLIC haven-dbg)
target_compile_features(haven_common PUBLIC cxx_std_20)
set_target_properties(haven_common PROPERTIES
                      VERSION "${CMAKE_PROJECT_VERSION}"
                      SOVERSION "${CMAKE_PROJECT_VERSION}"
                      OUTPUT_NAME "common-${CMAKE_PROJECT_VERSION}"
                      PREFIX "libhvn-")
