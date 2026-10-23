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
# Exact PDFium 2025.11.19 dependency closure from BuildEngine-Admin XML.
# The shared libraries are staged next to the EXE (Release/), not found via PATH.
# Header-only packages may be listed here without producing runtime DLLs.
adecc_package_root(ABSEIL_PACKAGE abseil 20260817.0)
adecc_package_root(FAST_FLOAT_PACKAGE fast_float 8.2.3)
adecc_package_root(SKIA_PACKAGE skia 144)
adecc_package_root(ZLIB_PACKAGE zlib 1.3.2)
adecc_package_root(JPEG_PACKAGE libjpeg-turbo 3.2.0)
adecc_package_root(PNG_PACKAGE libpng 1.6.59)
adecc_package_root(FREETYPE_PACKAGE freetype 2.14.3)
adecc_package_root(ICU_PACKAGE icu 78.3)
adecc_package_root(LCMS_PACKAGE lcms2 2.19.1)
adecc_package_root(OPENJPEG_PACKAGE openjpeg 2.5.4)
adecc_package_root(OPENGL_PACKAGE opengl 26.2.1)
adecc_package_root(BZIP2_PACKAGE bzip2 1.0.8)
adecc_package_root(BROTLI_PACKAGE brotli 1.2.0)
adecc_package_root(HARFBUZZ_PACKAGE harfbuzz 14.4.0)

# Recompute on every configure; an earlier CMakeCache must not hide new DLLs.
set(ADECC_PDFIUM_RUNTIME_ROOTS
   "${PDFIUM_PACKAGE};${SDL2_PACKAGE};${ABSEIL_PACKAGE};${FAST_FLOAT_PACKAGE};${SKIA_PACKAGE};${ZLIB_PACKAGE};${JPEG_PACKAGE};${PNG_PACKAGE};${FREETYPE_PACKAGE};${ICU_PACKAGE};${LCMS_PACKAGE};${OPENJPEG_PACKAGE};${OPENGL_PACKAGE};${BZIP2_PACKAGE};${BROTLI_PACKAGE};${HARFBUZZ_PACKAGE}")
adecc_test_install(pdfium-sdl2-viewer
   PACKAGE_ROOTS ${ADECC_PDFIUM_RUNTIME_ROOTS}
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll
   EXCLUDE_DLLS third_party_abseil-cpp_absl.dll)
