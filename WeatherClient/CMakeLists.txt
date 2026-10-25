# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineWeatherClient LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

foreach(_id IN ITEMS skia sdl2 curl openssl boost nlohmann-json zlib
   brotli zstd xz bzip2 libjpeg-turbo libpng harfbuzz freetype)
   string(REPLACE "-" "_" _name "${_id}")
   string(TOUPPER "${_name}" _name)
   adecc_package_current_root("${_name}_PACKAGE" "${_id}")
   list(APPEND WEATHER_PACKAGE_ROOTS "${${_name}_PACKAGE}")
endforeach()

list(PREPEND CMAKE_PREFIX_PATH "${SKIA_PACKAGE}" "${SDL2_PACKAGE}")
find_package(Skia CONFIG REQUIRED PATHS "${SKIA_PACKAGE}" NO_DEFAULT_PATH)
find_package(SDL2 CONFIG REQUIRED PATHS "${SDL2_PACKAGE}" NO_DEFAULT_PATH)

set(_variant "${BUILDENGINE_CONFIGURATION}")
if(_variant STREQUAL "Debug")
   set(_curl_import libcurld_imp.lib)
else()
   set(_curl_import libcurl_imp.lib)
endif()
set(_curl_lib "${CURL_PACKAGE}/lib/win64/${_variant}/${_curl_import}")
set(_crypto_lib "${OPENSSL_PACKAGE}/lib/win64/${_variant}/libcrypto.lib")
set(_ssl_lib "${OPENSSL_PACKAGE}/lib/win64/${_variant}/libssl.lib")
foreach(_needed IN ITEMS "${_curl_lib}" "${_crypto_lib}" "${_ssl_lib}"
   "${NLOHMANN_JSON_PACKAGE}/include/nlohmann/json.hpp")
   if(NOT EXISTS "${_needed}")
      message(FATAL_ERROR "Missing installed active BuildEngine artifact: ${_needed}")
   endif()
endforeach()
add_library(WeatherCurl SHARED IMPORTED)
set_target_properties(WeatherCurl PROPERTIES
   IMPORTED_IMPLIB "${_curl_lib}"
   INTERFACE_INCLUDE_DIRECTORIES "${CURL_PACKAGE}/include")
add_library(WeatherCrypto SHARED IMPORTED)
set_target_properties(WeatherCrypto PROPERTIES
   IMPORTED_IMPLIB "${_crypto_lib}"
   INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_PACKAGE}/include")
add_library(WeatherSSL SHARED IMPORTED)
set_target_properties(WeatherSSL PROPERTIES
   IMPORTED_IMPLIB "${_ssl_lib}"
   INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_PACKAGE}/include"
   INTERFACE_LINK_LIBRARIES WeatherCrypto)

add_executable(WeatherClient WIN32 WeatherClient.cpp WeatherNetwork.h WeatherModel.h WeatherView.h WeatherArtwork.h WeatherNarrative.h WeatherText.h WeatherFont.h WeatherMetrics.h WeatherUnits.h)
set_target_properties(WeatherClient PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(WeatherClient PRIVATE
   SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX
   BOOST_ALL_NO_LIB BOOST_ERROR_CODE_HEADER_ONLY _WIN32_WINNT=0x0A00)
target_include_directories(WeatherClient PRIVATE
   "${NLOHMANN_JSON_PACKAGE}/include")
adecc_test_use_boost(WeatherClient "${BOOST_PACKAGE}")
target_link_libraries(WeatherClient PRIVATE
   Skia::skia SDL2::SDL2 WeatherCurl WeatherSSL WeatherCrypto ws2_32 crypt32)
target_link_options(WeatherClient PRIVATE "-Wl,--subsystem,windows")

# No entire SDK copy: only direct and transitive PE imports from installed
# BuildEngine package roots. This remains an ordinary consumer of DLLs.
adecc_test_install(WeatherClient RUNTIME_CLOSURE
   PACKAGE_ROOTS ${WEATHER_PACKAGE_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)

# User settings are created at runtime in the working directory; never
# overwrite a user's saved address during installation.

enable_testing()
add_test(NAME WeatherClientOfflineModel COMMAND "$<TARGET_FILE:WeatherClient>" --self-test)
set_tests_properties(WeatherClientOfflineModel PROPERTIES LABELS "weather;offline;data;graphics")
add_test(NAME WeatherClientStatechart COMMAND "$<TARGET_FILE:WeatherClient>" --fsm-test)
set_tests_properties(WeatherClientStatechart PROPERTIES LABELS "weather;fsm;offline")
