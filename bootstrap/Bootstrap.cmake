# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

cmake_minimum_required(VERSION 3.25)
get_filename_component(TEST_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(BUILDENGINE_ROOT "${TEST_ROOT}/.." ABSOLUTE)
if("$ENV{BDS}" STREQUAL "")
 message(FATAL_ERROR "BDS missing; use RAD Studio 13 developer environment")
endif()
foreach(path IN ITEMS
 "$ENV{BDS}/bin64/bcc64x.exe"
 "${BUILDENGINE_ROOT}/admin/cmake/toolchains/bcc64x-buildengine-cxx.cmake"
 "${BUILDENGINE_ROOT}/tools/ninja/1.13.2/ninja.exe"
 "${BUILDENGINE_ROOT}/admin/cmake/consumer/BuildEngineRuntime.cmake"
 "${TEST_ROOT}/bootstrap/BuildEngineInit.cmake")
 if(NOT EXISTS "${path}")
   message(FATAL_ERROR "BuildEngine input missing: ${path}")
 endif()
endforeach()
message(STATUS "Bootstrap verification passed; shared initial cache is tracked in Git")
message(STATUS "Configure: cmake -S CompareTest -B ../build/Test/CompareTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CompareTest/Release")
message(STATUS "Build: cmake --build ../build/Test/CompareTest/Release/cmake")
message(STATUS "Install: cmake --install ../build/Test/CompareTest/Release/cmake")
