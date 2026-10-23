cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineFinancial LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

adecc_package_root(BOOST_PACKAGE boost 1.92.0)
adecc_package_root(PUGIXML_PACKAGE pugixml 1.16)
adecc_package_root(OPENSSL_PACKAGE openssl 3.5.8)
list(PREPEND CMAKE_PREFIX_PATH "${PUGIXML_PACKAGE}" "${OPENSSL_PACKAGE}" "${BOOST_PACKAGE}")
find_package(pugixml CONFIG REQUIRED)
# OpenSSL's exported CONFIG has an invalid include path, and FindOpenSSL
# can discard preselected import libraries on a reused CMake cache.
# BuildEngine's installed shared import libraries are the source of truth.
set(_adecc_openssl_include "${OPENSSL_PACKAGE}/include")
set(_adecc_openssl_lib_dir "${OPENSSL_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}")
foreach(_adecc_openssl_file IN ITEMS
   "${_adecc_openssl_include}/openssl/ssl.h"
   "${_adecc_openssl_lib_dir}/libssl.lib"
   "${_adecc_openssl_lib_dir}/libcrypto.lib")
   if(NOT EXISTS "${_adecc_openssl_file}")
      message(FATAL_ERROR "Required BuildEngine OpenSSL artifact missing: ${_adecc_openssl_file}")
   endif()
endforeach()
if(NOT TARGET OpenSSL::Crypto)
   add_library(OpenSSL::Crypto SHARED IMPORTED)
   set_target_properties(OpenSSL::Crypto PROPERTIES
      IMPORTED_IMPLIB "${_adecc_openssl_lib_dir}/libcrypto.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${_adecc_openssl_include}")
endif()
if(NOT TARGET OpenSSL::SSL)
   add_library(OpenSSL::SSL SHARED IMPORTED)
   set_target_properties(OpenSSL::SSL PROPERTIES
      IMPORTED_IMPLIB "${_adecc_openssl_lib_dir}/libssl.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${_adecc_openssl_include}"
      INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto")
endif()

add_executable(financial_demo
 main.cpp FinancialJson.cpp HttpsClient.cpp EcbHttpsDownload.cpp
 OpenHolidaysQuery.cpp EcbPublicationCheck.cpp EcbHistoryCache.cpp)
set_property(TARGET financial_demo PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(financial_demo PRIVATE cxx_std_23)
# Boost's managed BCC64X/Clang compatibility overlay is mandatory and must
# precede the unmodified Boost package headers in the compiler search path.
set(_adecc_boost_overlay "${BUILDENGINE_ROOT}/admin/cmake/boost/bcc64x-native-clang")
if(NOT EXISTS "${_adecc_boost_overlay}/boost/config/user.hpp")
   message(FATAL_ERROR "Managed Boost BCC64X overlay missing: ${_adecc_boost_overlay}/boost/config/user.hpp")
endif()
target_include_directories(financial_demo BEFORE PRIVATE
   "${_adecc_boost_overlay}"
   "${BOOST_PACKAGE}/include")
target_compile_definitions(financial_demo PRIVATE
 BOOST_ALL_NO_LIB BOOST_SYSTEM_NO_LIB BOOST_JSON_DYN_LINK _WIN32_WINNT=0x0A00
)
# BCC64X defines __CODEGEARC__ and a GCC compatibility version, which can
# otherwise select GCC 4.x compatibility in Boost.Config instead of Clang.
set(_adecc_boost_lib "${BOOST_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libboost_json.lib")
if(NOT EXISTS "${_adecc_boost_lib}")
   message(FATAL_ERROR "Required BuildEngine Boost.JSON DLL import library missing: ${_adecc_boost_lib}")
endif()
add_library(AdeccBoostJson SHARED IMPORTED)
set_target_properties(AdeccBoostJson PROPERTIES
   IMPORTED_IMPLIB "${_adecc_boost_lib}"
   INTERFACE_INCLUDE_DIRECTORIES "${BOOST_PACKAGE}/include")
target_link_libraries(financial_demo PRIVATE
 pugixml::pugixml OpenSSL::SSL OpenSSL::Crypto AdeccBoostJson crypt32)

set(FINANCIAL_RUNTIME_ROOTS
 "${PUGIXML_PACKAGE};${OPENSSL_PACKAGE};${BOOST_PACKAGE}")
adecc_test_install(financial_demo PACKAGE_ROOTS ${FINANCIAL_RUNTIME_ROOTS}
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Tests/" DESTINATION "Tests"
 FILES_MATCHING PATTERN "*.json")
