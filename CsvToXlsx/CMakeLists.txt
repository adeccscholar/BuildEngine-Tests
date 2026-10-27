cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineCsvToXlsx LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

# All declared transitive package dependencies of OpenXLSX from BuildEngine's
# admin/build-libraries.xml. Search roots are NOT bulk-copied by the PE stager.
foreach(_id IN ITEMS openxlsx pugixml libzip boost zlib xz zstd openssl brotli)
   string(TOUPPER "${_id}" _key)
   adecc_package_current_root(${_key}_PACKAGE "${_id}")
endforeach()

set(OpenXLSX_DIR "${OPENXLSX_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/OpenXLSX")
find_package(OpenXLSX CONFIG REQUIRED PATHS "${OpenXLSX_DIR}" NO_DEFAULT_PATH)

set(_csv_sources CsvReader.cpp Convert.cpp)
add_executable(adecc_csv_to_xlsx main.cpp ${_csv_sources})
set_target_properties(adecc_csv_to_xlsx PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
   CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_csv_to_xlsx PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
target_compile_options(adecc_csv_to_xlsx PRIVATE "-fborland-exec-code-page=65001")
target_link_libraries(adecc_csv_to_xlsx PRIVATE OpenXLSX::OpenXLSX shell32)
adecc_test_install(adecc_csv_to_xlsx RUNTIME_CLOSURE PACKAGE_ROOTS
   "${OPENXLSX_PACKAGE}" "${PUGIXML_PACKAGE}" "${LIBZIP_PACKAGE}" "${BOOST_PACKAGE}" "${ZLIB_PACKAGE}" "${XZ_PACKAGE}"
   "${ZSTD_PACKAGE}" "${OPENSSL_PACKAGE}" "${BROTLI_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)

add_executable(adecc_csv_to_xlsx_test CsvReaderTests.cpp ${_csv_sources})
set_target_properties(adecc_csv_to_xlsx_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
   CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(adecc_csv_to_xlsx_test PRIVATE CHRONO_WORK_AROUND CHRONO_MISSED_ZONEDB)
target_compile_options(adecc_csv_to_xlsx_test PRIVATE "-fborland-exec-code-page=65001")
target_link_libraries(adecc_csv_to_xlsx_test PRIVATE OpenXLSX::OpenXLSX)
adecc_test_install(adecc_csv_to_xlsx_test RUNTIME_CLOSURE PACKAGE_ROOTS
   "${OPENXLSX_PACKAGE}" "${PUGIXML_PACKAGE}" "${LIBZIP_PACKAGE}" "${BOOST_PACKAGE}" "${ZLIB_PACKAGE}" "${XZ_PACKAGE}"
   "${ZSTD_PACKAGE}" "${OPENSSL_PACKAGE}" "${BROTLI_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
enable_testing()
add_test(NAME CsvToXlsxParserAndWorkbook COMMAND "$<TARGET_FILE:adecc_csv_to_xlsx_test>")
