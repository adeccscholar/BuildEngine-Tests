# Copyright (c) 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: MIT
# Project: adecc Scholar

cmake_minimum_required(VERSION 3.30)
project(ThirdPartyShowcase VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED ENV{TP_CMAKE})
   message(FATAL_ERROR "TP_CMAKE is not set. Run scripts/00-env.cmd first.")
endif()

include("$ENV{TP_CMAKE}/embarcadero/Bcc64xVcl.cmake")

if(DEFINED ENV{ADECC_BOOST_PROJECT_INCLUDE} AND NOT "$ENV{ADECC_BOOST_PROJECT_INCLUDE}" STREQUAL "")
   if(NOT EXISTS "$ENV{ADECC_BOOST_PROJECT_INCLUDE}")
     message(FATAL_ERROR "Resolved Boost project include is missing: $ENV{ADECC_BOOST_PROJECT_INCLUDE}")
   endif()
   include("$ENV{ADECC_BOOST_PROJECT_INCLUDE}")
endif()

set(ADECC_BOOST_ROOT "" CACHE PATH "Boost root containing boost/version.hpp")
set(_boost_hints)
if(ADECC_BOOST_ROOT)
   list(APPEND _boost_hints "${ADECC_BOOST_ROOT}")
endif()
foreach(_env ADECC_BOOST_ROOT CG_64X_BOOST_ROOT CG_64_BOOST_ROOT BOOST_ROOT)
   if(DEFINED ENV{${_env}} AND NOT "$ENV{${_env}}" STREQUAL "")
      list(APPEND _boost_hints "$ENV{${_env}}")
   endif()
endforeach()
if(DEFINED ENV{BDS} AND NOT "$ENV{BDS}" STREQUAL "")
   list(APPEND _boost_hints
      "$ENV{BDS}/include/boost_1_89"
      "$ENV{BDS}/include")
endif()

find_path(ADECC_BOOST_INCLUDE_DIR
   NAMES boost/version.hpp
   HINTS ${_boost_hints})
if(NOT ADECC_BOOST_INCLUDE_DIR)
   message(FATAL_ERROR
      "Boost headers were not found. Set -DADECC_BOOST_ROOT=<path> to the C++Builder Boost root.")
endif()

find_package(OpenSSL CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(ZLIB CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

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

target_include_directories(ThirdPartyShowcase PRIVATE "${ADECC_BOOST_INCLUDE_DIR}")
target_compile_definitions(ThirdPartyShowcase PRIVATE
   BOOST_ALL_NO_LIB
   BOOST_SYSTEM_NO_LIB
   _WIN32_WINNT=0x0A00)

target_link_libraries(ThirdPartyShowcase PRIVATE
   OpenSSL::SSL
   OpenSSL::Crypto
   CURL::libcurl
   ZLIB::ZLIB
   libzip::zip
   nlohmann_json::nlohmann_json
   Crypt32)

foreach(theDllTarget IN ITEMS
   OpenSSL::SSL
   OpenSSL::Crypto
   CURL::libcurl
   ZLIB::ZLIB
   libzip::zip)
   add_custom_command(TARGET ThirdPartyShowcase POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
         "$<TARGET_FILE:${theDllTarget}>"
         "$<TARGET_FILE_DIR:ThirdPartyShowcase>"
      VERBATIM)
endforeach()
