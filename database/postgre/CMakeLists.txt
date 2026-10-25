cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(BuildEnginePostgreTests LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
foreach(_id IN ITEMS libpq libpqxx openssl zlib brotli zstd)
 string(REPLACE "-" "_" _key "${_id}")
 string(TOUPPER "${_key}" _key)
 adecc_package_current_root(${_key}_PACKAGE "${_id}")
endforeach()
set(PostgreSQL_INCLUDE_DIR "${LIBPQ_PACKAGE}/include")
set(PostgreSQL_LIBRARY "${LIBPQ_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libpq.lib")
find_package(PostgreSQL REQUIRED)
# Use the precise exported CMake config directory verified by the
# BuildEngine-Admin libpqxx consumer smoke. The package is installed under
# lib/win64/<configuration>/cmake/libpqxx, not at the package root.
set(_pqxx_config_dir
 "${LIBPQXX_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/libpqxx")
if(NOT EXISTS "${_pqxx_config_dir}/libpqxx-config.cmake" AND
   NOT EXISTS "${_pqxx_config_dir}/libpqxxConfig.cmake")
 message(FATAL_ERROR
  "BuildEngine libpqxx CMake export is missing: ${_pqxx_config_dir}. "
  "The package's install/publish phase must provide the official export.")
endif()
set(libpqxx_DIR "${_pqxx_config_dir}" CACHE PATH
 "BuildEngine-managed libpqxx package config" FORCE)
find_package(libpqxx CONFIG REQUIRED PATHS "${_pqxx_config_dir}" NO_DEFAULT_PATH)
if(TARGET libpqxx::pqxx)
 set(_pqxx_target libpqxx::pqxx)
elseif(TARGET pqxx)
 set(_pqxx_target pqxx)
else()
 message(FATAL_ERROR "libpqxx package lacks shared imported target")
endif()
add_executable(adecc_postgresql_pqxx_test test_postgresql_pqxx_database.cpp)
set_target_properties(adecc_postgresql_pqxx_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_postgresql_pqxx_test PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
adecc_test_use_core(adecc_postgresql_pqxx_test postgre)
target_include_directories(adecc_postgresql_pqxx_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")
target_link_libraries(adecc_postgresql_pqxx_test PRIVATE PostgreSQL::PostgreSQL ${_pqxx_target})
adecc_test_install(adecc_postgresql_pqxx_test PACKAGE_ROOTS
 "${LIBPQ_PACKAGE}" "${LIBPQXX_PACKAGE}" "${OPENSSL_PACKAGE}" "${ZLIB_PACKAGE}"
 "${BROTLI_PACKAGE}" "${ZSTD_PACKAGE}"
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
