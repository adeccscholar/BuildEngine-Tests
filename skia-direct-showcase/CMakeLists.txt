# Copyright (c) 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: MIT
# Project: adecc Scholar

cmake_minimum_required(VERSION 3.25)
project(SkiaDirectShowcase LANGUAGES CXX)

if(NOT DEFINED SKIA_PACKAGE_DIR OR SKIA_PACKAGE_DIR STREQUAL "")
   message(FATAL_ERROR "SKIA_PACKAGE_DIR must point to the installed direct BCC64X Skia package")
endif()

set(SKIA_INCLUDE_ROOT "${SKIA_PACKAGE_DIR}/include")
set(SKIA_LIBRARY "${SKIA_PACKAGE_DIR}/lib/skia.lib")

if(NOT EXISTS "${SKIA_INCLUDE_ROOT}/include/core/SkSurface.h")
   message(FATAL_ERROR "Skia public headers not found below ${SKIA_INCLUDE_ROOT}")
endif()
if(NOT EXISTS "${SKIA_INCLUDE_ROOT}/modules/skcms/skcms.h")
   message(FATAL_ERROR "Skia package is incomplete: modules/skcms/skcms.h is missing below ${SKIA_INCLUDE_ROOT}")
endif()
if(NOT EXISTS "${SKIA_INCLUDE_ROOT}/modules/skcms/src/skcms_public.h")
   message(FATAL_ERROR "Skia package is incomplete: modules/skcms/src/skcms_public.h is missing below ${SKIA_INCLUDE_ROOT}")
endif()
if(NOT EXISTS "${SKIA_LIBRARY}")
   message(FATAL_ERROR "Direct BCC64X Skia library not found: ${SKIA_LIBRARY}")
endif()

add_executable(SkiaDirectShowcase WIN32
   main.cpp)

# BCC64X 7.80/CMake 4.x: clang-scan-deps does not inherit the RAD Studio
# standard-library include search. These demos do not use C++ modules.
set_property(TARGET SkiaDirectShowcase PROPERTY CXX_SCAN_FOR_MODULES OFF)

set_target_properties(SkiaDirectShowcase PROPERTIES
   CXX_STANDARD 20
   CXX_STANDARD_REQUIRED YES
   CXX_EXTENSIONS NO)

target_compile_definitions(SkiaDirectShowcase PRIVATE
   UNICODE
   _UNICODE
   WIN32_LEAN_AND_MEAN
   NOMINMAX)

target_compile_options(SkiaDirectShowcase PRIVATE
   -tM
   -tR)

target_link_options(SkiaDirectShowcase PRIVATE
   -tM
   -tR)

target_include_directories(SkiaDirectShowcase PRIVATE
   "${SKIA_INCLUDE_ROOT}")

target_link_libraries(SkiaDirectShowcase PRIVATE
   "${SKIA_LIBRARY}"
   user32
   gdi32
   ole32
   windowscodecs)

set(DEMO_ASSETS
   "Hintergrund.png"
   "C++Builder13.png"
   "CB-Logo.png"
   "Community Edition.png")

foreach(theAsset IN LISTS DEMO_ASSETS)
   add_custom_command(TARGET SkiaDirectShowcase POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:SkiaDirectShowcase>/assets"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
         "${CMAKE_CURRENT_SOURCE_DIR}/assets/${theAsset}"
         "$<TARGET_FILE_DIR:SkiaDirectShowcase>/assets/${theAsset}"
      VERBATIM)
endforeach()
