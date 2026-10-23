# Copyright (c) 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(SkiaDirectShowcase LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

# Active native DLL package, never an old flat lib/skia.lib.
adecc_package_current_root(SKIA_PACKAGE skia)
list(PREPEND CMAKE_PREFIX_PATH "${SKIA_PACKAGE}")
find_package(Skia CONFIG REQUIRED)

add_executable(SkiaDirectShowcase WIN32 main.cpp)
set_property(TARGET SkiaDirectShowcase PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(SkiaDirectShowcase PRIVATE cxx_std_23)
target_compile_definitions(SkiaDirectShowcase PRIVATE
   UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(SkiaDirectShowcase PRIVATE
   Skia::skia user32 gdi32 ole32 windowscodecs)
# Native BCC64X Windows GUI entry point, no console subsystem.
target_link_options(SkiaDirectShowcase PRIVATE "-Wl,--subsystem,windows")

# Skia native dynamic dependencies from the BuildEngine XML contract.
foreach(_id IN ITEMS zlib brotli libjpeg-turbo libpng harfbuzz freetype)
   string(REPLACE "-" "_" _variable "${_id}")
   adecc_package_current_root(_package "${_id}")
   list(APPEND SKIA_RUNTIME_ROOTS "${_package}")
endforeach()
list(PREPEND SKIA_RUNTIME_ROOTS "${SKIA_PACKAGE}")
adecc_test_install(SkiaDirectShowcase
   PACKAGE_ROOTS ${SKIA_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)

set(_assets "Hintergrund.png" "C++Builder13.png"
   "CB-Logo.png" "Community Edition.png")
foreach(_asset IN LISTS _assets)
   if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/${_asset}")
      message(FATAL_ERROR "Missing Skia showcase asset: ${_asset}")
   endif()
endforeach()
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
   DESTINATION assets FILES_MATCHING PATTERN "*.png")
