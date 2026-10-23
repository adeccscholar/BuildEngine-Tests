# Copyright (c) 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: MIT
# Project: adecc Scholar

cmake_minimum_required(VERSION 3.30)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(ThirdPartyShowcase VERSION 1.0.0 LANGUAGES CXX RC)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Bcc64xVcl.cmake")

# Resolve unique active BuildEngine packages instead of host/Catalog variables.
foreach(_id IN ITEMS boost openssl curl zlib libzip nlohmann-json brotli zstd xz)
   string(REPLACE "-" "_" _key "${_id}")
   string(TOUPPER "${_key}" _key)
   adecc_package_current_root(${_key}_PACKAGE "${_id}")
endforeach()

# The installed OpenSSL CMake export has an invalid include path;
# use the actual native shared-library import artifacts, as in Financial.
set(_variant "${BUILDENGINE_CONFIGURATION}")
set(_openssl_include "${OPENSSL_PACKAGE}/include")
set(_openssl_lib "${OPENSSL_PACKAGE}/lib/win64/${_variant}")
if(_variant STREQUAL "Debug")
   set(_curl_import "libcurld_imp.lib")
   set(_zlib_import "libzd.lib")
else()
   set(_curl_import "libcurl_imp.lib")
   set(_zlib_import "libz.lib")
endif()
foreach(_file IN ITEMS
   "${_openssl_include}/openssl/ssl.h"
   "${_openssl_lib}/libssl.lib"
   "${_openssl_lib}/libcrypto.lib"
   "${CURL_PACKAGE}/lib/win64/${_variant}/${_curl_import}"
   "${ZLIB_PACKAGE}/lib/win64/${_variant}/${_zlib_import}"
   "${LIBZIP_PACKAGE}/lib/win64/${_variant}/libzip.lib"
   "${NLOHMANN_JSON_PACKAGE}/include/nlohmann/json.hpp")
   if(NOT EXISTS "${_file}")
      message(FATAL_ERROR "Required BuildEngine DLL import/header missing: ${_file}")
   endif()
endforeach()
add_library(OpenSSL::Crypto SHARED IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
   IMPORTED_IMPLIB "${_openssl_lib}/libcrypto.lib"
   INTERFACE_INCLUDE_DIRECTORIES "${_openssl_include}")
add_library(OpenSSL::SSL SHARED IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
   IMPORTED_IMPLIB "${_openssl_lib}/libssl.lib"
   INTERFACE_INCLUDE_DIRECTORIES "${_openssl_include}"
   INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto")
add_library(AdeccCurl SHARED IMPORTED)
set_target_properties(AdeccCurl PROPERTIES
   IMPORTED_IMPLIB "${CURL_PACKAGE}/lib/win64/${_variant}/${_curl_import}"
   INTERFACE_INCLUDE_DIRECTORIES "${CURL_PACKAGE}/include")
add_library(AdeccZlib SHARED IMPORTED)
set_target_properties(AdeccZlib PROPERTIES
   IMPORTED_IMPLIB "${ZLIB_PACKAGE}/lib/win64/${_variant}/${_zlib_import}"
   INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_PACKAGE}/include")
add_library(AdeccLibzip SHARED IMPORTED)
set_target_properties(AdeccLibzip PROPERTIES
   IMPORTED_IMPLIB "${LIBZIP_PACKAGE}/lib/win64/${_variant}/libzip.lib"
   INTERFACE_INCLUDE_DIRECTORIES "${LIBZIP_PACKAGE}/include")

# The managed native BCC64X Boost configuration overlay is mandatory.
set(_boost_overlay "${BUILDENGINE_ROOT}/admin/cmake/boost/bcc64x-native-clang")
if(NOT EXISTS "${_boost_overlay}/boost/config/user.hpp")
   message(FATAL_ERROR "Missing managed BCC64X Boost overlay: ${_boost_overlay}")
endif()

add_executable(ThirdPartyShowcase WIN32
   ThirdPartyShowcase.cpp
   MainForm.cpp
   MainForm.h
   ShowcaseEngine.cpp
   ShowcaseEngine.h
   HttpsClient.cpp
   HttpsClient.h)

# CMake 3.28+ scans C++20-and-newer sources for module dependencies by
# default. With BCC64X 7.80 the clang-scan-deps frontend does not inherit the
# compiler driver's implicit RAD Studio / standard-library include search
# configuration, so the scan phase cannot even resolve <string> or vcl.h.
# This target does not use C++ modules; disable only the unnecessary scan.
set_property(TARGET ThirdPartyShowcase PROPERTY CXX_SCAN_FOR_MODULES OFF)

cb_add_vcl_form(ThirdPartyShowcase MainForm.cpp MainForm.dfm)
cb_set_embt_vcl_target(ThirdPartyShowcase)
cb_add_windows_application_resources(ThirdPartyShowcase
   ICON "${CMAKE_CURRENT_SOURCE_DIR}/app.ico"
   FILE_VERSION 1 0 0 0
   PRODUCT_VERSION 1 0 0 0
   COMPANY_NAME "adecc Systemhaus GmbH"
   FILE_DESCRIPTION "BCC64X Third-Party Integration Showcase"
   INTERNAL_NAME "ThirdPartyShowcase"
   ORIGINAL_FILENAME "ThirdPartyShowcase.exe"
   PRODUCT_NAME "adecc Scholar BCC64X Third-Party Showcase"
   COPYRIGHT "Copyright (C) 2026 adecc Systemhaus GmbH")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Embarcadero")
   target_compile_options(ThirdPartyShowcase PRIVATE -std=c++23)
else()
   target_compile_features(ThirdPartyShowcase PRIVATE cxx_std_23)
endif()

target_include_directories(ThirdPartyShowcase BEFORE PRIVATE
   "${_boost_overlay}" "${BOOST_PACKAGE}/include")
target_include_directories(ThirdPartyShowcase PRIVATE
   "${NLOHMANN_JSON_PACKAGE}/include")
target_compile_definitions(ThirdPartyShowcase PRIVATE
   BOOST_ALL_NO_LIB
   BOOST_SYSTEM_NO_LIB
   _WIN32_WINNT=0x0A00)

target_link_libraries(ThirdPartyShowcase PRIVATE
   OpenSSL::SSL
   OpenSSL::Crypto
   AdeccCurl
   AdeccZlib
   AdeccLibzip
   Crypt32)

# Complete runtime closure from the BuildEngine XML dependency declarations.
# Runtime staging retains PE/COFF, SHA256 and collision checks.
adecc_test_install(ThirdPartyShowcase
   PACKAGE_ROOTS
      "${BOOST_PACKAGE}" "${OPENSSL_PACKAGE}" "${CURL_PACKAGE}"
      "${ZLIB_PACKAGE}" "${LIBZIP_PACKAGE}" "${BROTLI_PACKAGE}"
      "${ZSTD_PACKAGE}" "${XZ_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
