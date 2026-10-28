cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(BuildEngineMariaDBTests LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
adecc_package_current_root(MARIADB_PACKAGE mariadb-connector-c)
adecc_package_current_root(ZLIB_PACKAGE zlib)
adecc_package_current_root(ZSTD_PACKAGE zstd)
adecc_package_current_root(PUGIXML_PACKAGE pugixml)
set(_import "${MARIADB_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libmariadb.lib")
if(NOT EXISTS "${_import}" OR NOT EXISTS "${MARIADB_PACKAGE}/include/mysql.h")
   message(FATAL_ERROR "Native MariaDB Connector/C package is not installed: ${MARIADB_PACKAGE}")
endif()
add_executable(adecc_mariadb_logical_test test_mariadb_database.cpp additional_dialect.h mariadb_arguments.h)
set_target_properties(adecc_mariadb_logical_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_mariadb_logical_test PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
adecc_test_use_core(adecc_mariadb_logical_test mariadb)
target_include_directories(adecc_mariadb_logical_test PRIVATE "${MARIADB_PACKAGE}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../common")
target_link_libraries(adecc_mariadb_logical_test PRIVATE "${_import}")
adecc_test_install(adecc_mariadb_logical_test RUNTIME_CLOSURE
   PACKAGE_ROOTS "${MARIADB_PACKAGE}" "${ZLIB_PACKAGE}" "${ZSTD_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
add_executable(adecc_mariadb_connection_test test_mariadb_connection.cpp mariadb_arguments.h)
set_target_properties(adecc_mariadb_connection_test PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_mariadb_connection_test PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
adecc_test_use_core(adecc_mariadb_connection_test mariadb)
target_include_directories(adecc_mariadb_connection_test PRIVATE "${MARIADB_PACKAGE}/include")
target_link_libraries(adecc_mariadb_connection_test PRIVATE "${_import}")
adecc_test_install(adecc_mariadb_connection_test RUNTIME_CLOSURE
   PACKAGE_ROOTS "${MARIADB_PACKAGE}" "${ZLIB_PACKAGE}" "${ZSTD_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
# Shared domain and CSV pipeline used unchanged by future PostgreSQL/MSSQL adapters.
add_executable(adecc_mariadb_member_pipeline member_pipeline_mariadb.cpp member_dialect.h
   ../../database/common/member_pipeline.h)
set_target_properties(adecc_mariadb_member_pipeline PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_mariadb_member_pipeline PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
target_compile_options(adecc_mariadb_member_pipeline PRIVATE "-fborland-exec-code-page=65001")
adecc_test_use_core(adecc_mariadb_member_pipeline mariadb)
target_include_directories(adecc_mariadb_member_pipeline PRIVATE
   "${MARIADB_PACKAGE}/include"
   "${PUGIXML_PACKAGE}/include"
   "${CMAKE_CURRENT_SOURCE_DIR}/../common"
   "${CMAKE_CURRENT_SOURCE_DIR}/../../adecc-core-lib/adecc/cpp_domain")
set(_pugi_import "${PUGIXML_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libpugixml.lib")
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_pugi_import "${PUGIXML_PACKAGE}/lib/win64/Debug/libpugixmld.lib")
endif()
if(NOT EXISTS "${_pugi_import}")
   message(FATAL_ERROR "Missing BuildEngine pugixml shared import: ${_pugi_import}")
endif()
target_link_libraries(adecc_mariadb_member_pipeline PRIVATE "${_import}" "${_pugi_import}")
adecc_test_install(adecc_mariadb_member_pipeline RUNTIME_CLOSURE
   PACKAGE_ROOTS "${MARIADB_PACKAGE}" "${PUGIXML_PACKAGE}" "${ZLIB_PACKAGE}" "${ZSTD_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/"
   DESTINATION "fixtures")
enable_testing()
add_test(NAME MariaDBMembersOffline COMMAND "$<TARGET_FILE:adecc_mariadb_member_pipeline>" --offline)
set_tests_properties(MariaDBMembersOffline PROPERTIES LABELS "database;mariadb;members;offline")
add_test(NAME MariaDBClientOffline COMMAND "$<TARGET_FILE:adecc_mariadb_connection_test>" --offline)
set_tests_properties(MariaDBClientOffline PROPERTIES LABELS "database;mariadb;offline")

add_test(NAME MariaDBLogicalOffline COMMAND "$<TARGET_FILE:adecc_mariadb_logical_test>" --offline)
set_tests_properties(MariaDBLogicalOffline PROPERTIES LABELS "database;mariadb;offline")
