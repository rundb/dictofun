# MIT License

# Copyright (c) 2020 Polidea

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Additional information used for selecting proper libs.
get_property(nrf5_mdk_compile_options TARGET nrf5_mdk PROPERTY "COMPILE_OPTIONS")

# Check CPU type.
set(cpu_type "")

list(FIND nrf5_mdk_compile_options "-mcpu=cortex-m0" cpu_available)
if (NOT cpu_available EQUAL -1)
  set(cpu_type "cortex-m0")
endif()

list(FIND nrf5_mdk_compile_options "-mcpu=cortex-m33" cpu_available)
if (NOT cpu_available EQUAL -1)
  set(cpu_type "cortex-m33")
endif()

list(FIND nrf5_mdk_compile_options "-mcpu=cortex-m4" cpu_available)
if (NOT cpu_available EQUAL -1)
  set(cpu_type "cortex-m4")
endif()

# Check FPU
set(float_abi "")

list(FIND nrf5_mdk_compile_options "-mfloat-abi=soft" float_abi_soft)
if (NOT float_abi_soft EQUAL -1)
  set(float_abi "soft-float")
endif()

list(FIND nrf5_mdk_compile_options "-mfloat-abi=hard" float_abi_hard)
if (NOT float_abi_hard EQUAL -1)
  set(float_abi "hard-float")
endif()

# Check wchar size
set(short_wchar "")

list(FIND nrf5_mdk_compile_options "-fshort-wchar" is_short_wchar)
if (NOT is_short_wchar EQUAL -1)
  set(short_wchar "short-wchar")
endif()

# Micro ECC
function(add_micro_ecc_target micro_ecc_source)
  add_library(nrf5_ext_micro_ecc_fwd INTERFACE)
  target_include_directories(nrf5_ext_micro_ecc_fwd INTERFACE
    "${micro_ecc_source}"
  )
  add_library(micro_ecc OBJECT EXCLUDE_FROM_ALL
    "${micro_ecc_source}/uECC.c"
  )
  set_source_files_properties(
    "${micro_ecc_source}/uECC.c" PROPERTIES GENERATED TRUE
  )
  target_link_libraries(micro_ecc PUBLIC
    nrf5_ext_micro_ecc_fwd
  )
endfunction()

set(micro_ecc_source_dir "${NRF5_SDK_PATH}/external/micro-ecc/micro-ecc")
if(EXISTS "${micro_ecc_source_dir}/uECC.c")
  # Use included uECC library.
  add_micro_ecc_target("${micro_ecc_source_dir}")
else()
  # Download uECC from github.
  include(ExternalProject)
  ExternalProject_Add(micro_ecc_src
    EXCLUDE_FROM_ALL TRUE
    PREFIX external/micro_ecc_src
    GIT_REPOSITORY "https://github.com/kmackay/micro-ecc.git"
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
  ExternalProject_Get_property(micro_ecc_src SOURCE_DIR)
  set(micro_ecc_source_dir "${SOURCE_DIR}")
  add_micro_ecc_target("${micro_ecc_source_dir}")
  add_dependencies(nrf5_ext_micro_ecc_fwd micro_ecc_src)
endif()
