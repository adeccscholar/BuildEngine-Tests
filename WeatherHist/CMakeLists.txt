# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineWeatherHist LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Bcc64xVcl.cmake")

foreach(_id IN ITEMS curl openssl zlib brotli zstd nlohmann-json)
   string(REPLACE "-" "_" _key "${_id}")
   string(TOUPPER "${_key}" _key)
   adecc_package_current_root("${_key}_PACKAGE" "${_id}")
   list(APPEND WEATHERHIST_RUNTIME_ROOTS "${${_key}_PACKAGE}")
endforeach()

set(_variant "${BUILDENGINE_CONFIGURATION}")
if(_variant STREQUAL "Debug")
   set(_curl_imp libcurld_imp.lib)
else()
   set(_curl_imp libcurl_imp.lib)
endif()
foreach(_file IN ITEMS
   "${CURL_PACKAGE}/lib/win64/${_variant}/${_curl_imp}"
   "${OPENSSL_PACKAGE}/lib/win64/${_variant}/libcrypto.lib"
   "${OPENSSL_PACKAGE}/lib/win64/${_variant}/libssl.lib"
   "${NLOHMANN_JSON_PACKAGE}/include/nlohmann/json.hpp")
   if(NOT EXISTS "${_file}")
      message(FATAL_ERROR "Missing BuildEngine dependency: ${_file}")
   endif()
endforeach()

add_library(WeatherHistCurl SHARED IMPORTED)
set_target_properties(WeatherHistCurl PROPERTIES
   IMPORTED_IMPLIB "${CURL_PACKAGE}/lib/win64/${_variant}/${_curl_imp}"
   INTERFACE_INCLUDE_DIRECTORIES "${CURL_PACKAGE}/include")

add_executable(WeatherHist WIN32 WeatherHist.cpp MainForm.cpp MainForm.h
   HistoryNetwork.h HistoryModel.h HistoryTimeline.h)
set_target_properties(WeatherHist PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
cb_add_vcl_form(WeatherHist MainForm.cpp MainForm.dfm)
cb_set_embt_vcl_target(WeatherHist)
target_include_directories(WeatherHist PRIVATE "${NLOHMANN_JSON_PACKAGE}/include")
target_compile_definitions(WeatherHist PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(WeatherHist PRIVATE "-fborland-exec-code-page=65001")
target_link_libraries(WeatherHist PRIVATE WeatherHistCurl)
# VCL target uses only the managed BCC64X toolchain; no MSVC/MinGW or static OSS.
adecc_test_install(WeatherHist RUNTIME_CLOSURE
   PACKAGE_ROOTS ${WEATHERHIST_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)

add_executable(WeatherHistModelTest HistoryModelTests.cpp HistoryModel.h HistoryTimeline.h)
set_target_properties(WeatherHistModelTest PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_include_directories(WeatherHistModelTest PRIVATE
   "${NLOHMANN_JSON_PACKAGE}/include")
target_compile_options(WeatherHistModelTest PRIVATE
   "-fborland-exec-code-page=65001")
adecc_test_install(WeatherHistModelTest RUNTIME_CLOSURE
   PACKAGE_ROOTS ${WEATHERHIST_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
enable_testing()
add_test(NAME WeatherHistOfflineModel
   COMMAND "$<TARGET_FILE:WeatherHistModelTest>")
set_tests_properties(WeatherHistOfflineModel PROPERTIES LABELS "weather;history;offline")
