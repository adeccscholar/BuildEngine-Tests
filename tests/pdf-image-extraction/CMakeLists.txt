cmake_minimum_required(VERSION 3.25)
project(BuildEnginePdfImageExtraction LANGUAGES CXX)

# Use the installed BuildEngine package instead of an arbitrary qpdf from PATH.
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
adecc_package_root(QPDF_PACKAGE qpdf 12.4.1)
adecc_package_root(ZLIB_PACKAGE zlib 1.3.2)
adecc_package_root(JPEG_PACKAGE libjpeg-turbo 3.2.0)
list(PREPEND CMAKE_PREFIX_PATH "${QPDF_PACKAGE}" "${ZLIB_PACKAGE}" "${JPEG_PACKAGE}")
find_package(qpdf CONFIG REQUIRED)

add_executable(pdf-image-extraction main.cpp)
set_property(TARGET pdf-image-extraction PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdf-image-extraction PRIVATE cxx_std_23)
target_link_libraries(pdf-image-extraction PRIVATE qpdf::libqpdf)

# Full transitive DLL closure may be supplied explicitly by the calling build.
# QPDF 12.4.1 BuildEngine contract: shared QPDF, zlib 1.3.2 and
# libjpeg-turbo 3.2.0. Do not resolve transitive DLLs through PATH.
# This is a fixed dependency contract, not a cached user preference.
# An old CMakeCache.txt may still contain a QPDF-only runtime root.
set(ADECC_QPDF_RUNTIME_ROOTS
   "${QPDF_PACKAGE};${ZLIB_PACKAGE};${JPEG_PACKAGE}")
# BCC64X C++ executables depend on the selected Embarcadero runtime.
set(ADECC_QPDF_TOOLCHAIN_DLLS "libc++-370.dll;libunwind-370.dll"
   CACHE STRING "Exact BCC64X runtime DLLs required by the QPDF consumer")
adecc_test_install(pdf-image-extraction
   PACKAGE_ROOTS ${ADECC_QPDF_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS ${ADECC_QPDF_TOOLCHAIN_DLLS})

# Standalone integration regression: export a known one-pixel PDF image.
enable_testing()
add_test(NAME qpdf-image-extraction
  COMMAND "${CMAKE_COMMAND}"
    "-DTEST_EXE=$<TARGET_FILE:pdf-image-extraction>"
    "-DTEST_PDF=${CMAKE_CURRENT_SOURCE_DIR}/../../pdf-image-extraction/fixtures/single-image.pdf"
    "-DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/qpdf-image-extraction-output"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../../pdf-image-extraction/CheckExtraction.cmake")

# Ship a ready-to-run example PDF with the installed demo application.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../pdf-image-extraction/fixtures/single-image.pdf"
   DESTINATION "pdf-image-extraction/examples")
