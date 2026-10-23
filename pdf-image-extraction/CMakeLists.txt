cmake_minimum_required(VERSION 3.25)
project(BuildEnginePdfImageExtraction LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
adecc_package_root(QPDF_PACKAGE qpdf 12.4.1)
list(PREPEND CMAKE_PREFIX_PATH "${QPDF_PACKAGE}")
find_package(qpdf CONFIG REQUIRED)
add_executable(pdf-image-extraction main.cpp)
set_property(TARGET pdf-image-extraction PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdf-image-extraction PRIVATE cxx_std_23)
target_link_libraries(pdf-image-extraction PRIVATE qpdf::libqpdf)
set(ADECC_QPDF_RUNTIME_ROOTS "${QPDF_PACKAGE}" CACHE STRING "Exact QPDF transitive runtime package roots")
adecc_test_install(pdf-image-extraction PACKAGE_ROOTS ${ADECC_QPDF_RUNTIME_ROOTS})

# Standalone integration regression: export a known one-pixel PDF image.
enable_testing()
add_test(NAME qpdf-image-extraction
  COMMAND "${CMAKE_COMMAND}"
    "-DTEST_EXE=$<TARGET_FILE:pdf-image-extraction>"
    "-DTEST_PDF=${CMAKE_CURRENT_SOURCE_DIR}/./fixtures/single-image.pdf"
    "-DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/qpdf-image-extraction-output"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/./CheckExtraction.cmake")
