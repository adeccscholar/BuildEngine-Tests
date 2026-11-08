cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(BuildEnginePDFiumSDL2 LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
adecc_package_root(PDFIUM_PACKAGE pdfium 2025.11.19)
adecc_package_root(SDL2_PACKAGE sdl2 2.32.10)
list(PREPEND CMAKE_PREFIX_PATH "${PDFIUM_PACKAGE}" "${SDL2_PACKAGE}")
find_package(PDFium CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
add_executable(pdfium-sdl2-viewer WIN32 main.cpp)
set_property(TARGET pdfium-sdl2-viewer PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdfium-sdl2-viewer PRIVATE cxx_std_23)
target_compile_definitions(pdfium-sdl2-viewer PRIVATE SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pdfium-sdl2-viewer PRIVATE PDFium::pdfium SDL2::SDL2 comdlg32)
# Exact PDFium 2025.11.19 and Skia 153 closure from BuildEngine-Admin XML.
# Each test executable has its own runtime directory. Only its recursive
# PE import closure is staged from the pinned packages, never from PATH.
# Header-only packages may be declared without contributing runtime DLLs.
adecc_package_root(ABSEIL_PACKAGE abseil 20260817.0)
adecc_package_root(FAST_FLOAT_PACKAGE fast_float 8.3.3)
adecc_package_root(SKIA_PACKAGE skia 153)
adecc_package_root(ZLIB_PACKAGE zlib 1.4)
adecc_package_root(JPEG_PACKAGE libjpeg-turbo 3.2.0)
adecc_package_root(PNG_PACKAGE libpng 1.6.59)
adecc_package_root(FREETYPE_PACKAGE freetype 2.15.0)
adecc_package_root(ICU_PACKAGE icu 78.3)
adecc_package_root(LCMS_PACKAGE lcms2 2.20)
adecc_package_root(OPENJPEG_PACKAGE openjpeg 2.5.4)
adecc_package_root(OPENGL_PACKAGE opengl 26.2.1)
adecc_package_root(BZIP2_PACKAGE bzip2 1.0.8)
adecc_package_root(BROTLI_PACKAGE brotli 1.2.1)
adecc_package_root(HARFBUZZ_PACKAGE harfbuzz 15.1.0)

# Recompute on every configure; an earlier CMakeCache must not hide new DLLs.
set(ADECC_PDFIUM_RUNTIME_ROOTS
   "${PDFIUM_PACKAGE};${SDL2_PACKAGE};${ABSEIL_PACKAGE};${FAST_FLOAT_PACKAGE};${SKIA_PACKAGE};${ZLIB_PACKAGE};${JPEG_PACKAGE};${PNG_PACKAGE};${FREETYPE_PACKAGE};${ICU_PACKAGE};${LCMS_PACKAGE};${OPENJPEG_PACKAGE};${OPENGL_PACKAGE};${BZIP2_PACKAGE};${BROTLI_PACKAGE};${HARFBUZZ_PACKAGE}")
adecc_test_install(pdfium-sdl2-viewer RUNTIME_CLOSURE
   PACKAGE_ROOTS ${ADECC_PDFIUM_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)

# Non-interactive PDFium C-API/render consumer, independent of SDL2 and VCL.
add_executable(pdfium-c-api-test PdfiumApiTest.cpp)
set_property(TARGET pdfium-c-api-test PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdfium-c-api-test PRIVATE cxx_std_23)
target_link_libraries(pdfium-c-api-test PRIVATE PDFium::pdfium)
set(ADECC_PDFIUM_API_RUNTIME_ROOTS ${ADECC_PDFIUM_RUNTIME_ROOTS})
list(REMOVE_ITEM ADECC_PDFIUM_API_RUNTIME_ROOTS "${SDL2_PACKAGE}")
adecc_test_install(pdfium-c-api-test RUNTIME_CLOSURE
   PACKAGE_ROOTS ${ADECC_PDFIUM_API_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)

# Regression suite for viewer navigation without a GUI or PDFium input file.
add_executable(pdfium-page-scroll-test ../Common/PageScrollTest.cpp)
set_property(TARGET pdfium-page-scroll-test PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdfium-page-scroll-test PRIVATE cxx_std_23)
# This pure navigation test has no PDFium, Skia or SDL2 imports.
# Stage its actual BCC64X PE import closure without any package roots.
adecc_test_install(pdfium-page-scroll-test RUNTIME_CLOSURE
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)

# Skia 153 publishes its runtime ICU data beside the configuration DLLs.
# The generic DLL stager does not stage .dat files; refresh it on every build.
set(ADECC_PDFIUM_ICU_DATA "${SKIA_PACKAGE}/bin/win64/${BUILDENGINE_CONFIGURATION}/icudtl.dat")
if(NOT EXISTS "${ADECC_PDFIUM_ICU_DATA}")
   message(FATAL_ERROR "Required Skia 153 runtime data missing: ${ADECC_PDFIUM_ICU_DATA}")
endif()
foreach(_pdfium_target IN ITEMS pdfium-sdl2-viewer pdfium-c-api-test)
   add_custom_target("buildengine-stage-data-${_pdfium_target}" ALL
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
         "${ADECC_PDFIUM_ICU_DATA}" "$<TARGET_FILE_DIR:${_pdfium_target}>/icudtl.dat"
      DEPENDS "buildengine-stage-${_pdfium_target}"
      COMMENT "Stage exact Skia ICU data for ${_pdfium_target}" VERBATIM)
endforeach()
install(FILES "${ADECC_PDFIUM_ICU_DATA}" DESTINATION ".")

enable_testing()
add_test(NAME pdfium-c-api-render COMMAND "$<TARGET_FILE:pdfium-c-api-test>")
add_test(NAME pdfium-page-scroll COMMAND "$<TARGET_FILE:pdfium-page-scroll-test>")
