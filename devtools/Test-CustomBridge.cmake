# SPDX-License-Identifier: GPL-2.0-only
# Added 2026-11-13. Run without Qt or a C++ compiler: cmake -P devtools/Test-CustomBridge.cmake
cmake_minimum_required(VERSION 3.24)
get_filename_component(_repo "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run)
set(_fixtures "${_repo}/.local/cmake-tests/${_run}")
file(MAKE_DIRECTORY "${_fixtures}")

# Structurally valid key, intentionally not an actual device identity.
string(ASCII 4 _uncompressed)
string(ASCII 3 _compressed)
string(REPEAT "a" 64 _coordinates)
file(WRITE "${_fixtures}/valid.bin" "${_uncompressed}${_coordinates}")
file(WRITE "${_fixtures}/bad-prefix.bin" "${_compressed}${_coordinates}")
file(WRITE "${_fixtures}/short.bin" "${_uncompressed}")
file(MAKE_DIRECTORY "${_fixtures}/cmake" "${_fixtures}/submodules/esp32-hid-tools")
file(COPY "${_repo}/cmake/bin2array.cmake" DESTINATION "${_fixtures}/cmake")
file(WRITE "${_fixtures}/submodules/esp32-hid-tools/CMakeLists.txt"
  "message(FATAL_ERROR \"Vendor tooling must not be loaded by this test\")\n")

file(WRITE "${_fixtures}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.24)
project(CustomBridgeConfiguration NONE)
include("${REPO}/cmake/CustomBridge.cmake")
if(DSH_CUSTOM_BRIDGE AND DSH_VENDOR_FIRMWARE_TOOLS)
  message(FATAL_ERROR "Vendor firmware tools were enabled in custom mode")
endif()
include("${REPO}/src/CMakeLists.user.cmake")
if(HAS_ESP32_HID_TOOLS)
  message(FATAL_ERROR "The installed vendor submodule was loaded despite the build mode")
endif()
if(DSH_CUSTOM_BRIDGE AND TARGET generate_public_key)
  message(FATAL_ERROR "Custom bridge must load keys at runtime")
endif()
]=])

function(check_case name expect_success expected_text)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${_fixtures}" -B "${_fixtures}/${name}"
      "-DREPO=${_repo}" ${ARGN}
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err
  )
  if(expect_success AND NOT _result EQUAL 0)
    message(FATAL_ERROR "${name} unexpectedly failed: ${_out}\n${_err}")
  elseif(NOT expect_success AND _result EQUAL 0)
    message(FATAL_ERROR "${name} unexpectedly succeeded")
  endif()
  if(NOT "${expected_text}" STREQUAL "" AND NOT "${_out}${_err}" MATCHES "${expected_text}")
    message(FATAL_ERROR "${name} failed for the wrong reason: ${_out}\n${_err}")
  endif()
  message(STATUS "PASS: ${name}")
endfunction()

check_case(runtime_keys TRUE "" )
check_case(vendor_forced_off TRUE "" -DDSH_VENDOR_FIRMWARE_TOOLS=ON)
check_case(upstream_opt_out TRUE "" -DDSH_CUSTOM_BRIDGE=OFF -DDSH_VENDOR_FIRMWARE_TOOLS=OFF)
