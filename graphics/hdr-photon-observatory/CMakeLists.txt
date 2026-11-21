cmake_minimum_required(VERSION 3.31)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(hdr_photon_observatory LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
include(CTest)

adecc_package_current_root(CERES_PACKAGE ceres-solver)
adecc_package_current_root(OPENEXR_PACKAGE openexr)
adecc_package_current_root(IMATH_PACKAGE imath)
adecc_package_current_root(ABSEIL_PACKAGE abseil)
adecc_package_current_root(EIGEN_PACKAGE eigen)
adecc_package_current_root(OPENJPH_PACKAGE openjph)
adecc_package_current_root(LIBDEFLATE_PACKAGE libdeflate)
adecc_package_current_root(ZSTD_PACKAGE zstd)
adecc_package_current_root(IMGUI_PACKAGE imgui)
adecc_package_current_root(SDL2_PACKAGE sdl2)

set(Eigen3_DIR "${EIGEN_PACKAGE}/lib/cmake/Eigen3")
set(absl_DIR "${ABSEIL_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/absl")
set(Ceres_DIR "${CERES_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/Ceres")
set(Imath_DIR "${IMATH_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/Imath")
set(openjph_DIR "${OPENJPH_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/openjph")
set(libdeflate_DIR "${LIBDEFLATE_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/libdeflate")
set(zstd_DIR "${ZSTD_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/zstd")
set(OpenEXR_DIR "${OPENEXR_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/OpenEXR")
set(GFLAGS_USE_TARGET_NAMESPACE TRUE)
find_package(Eigen3 CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(Imath CONFIG REQUIRED)
find_package(openjph CONFIG REQUIRED)
find_package(libdeflate CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(OpenEXR CONFIG REQUIRED)
find_package(Ceres CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED PATHS "${SDL2_PACKAGE}" NO_DEFAULT_PATH)
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_suffix d)
else()
   set(_suffix "")
endif()
set(_imgui_import "${IMGUI_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libimgui${_suffix}.lib")
if(NOT EXISTS "${_imgui_import}")
   message(FATAL_ERROR "Managed ImGui DLL import not found: ${_imgui_import}")
endif()
add_executable(hdr-photon-observatory WIN32 main.cpp)
set_target_properties(hdr-photon-observatory PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(hdr-photon-observatory PRIVATE
   SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(hdr-photon-observatory PRIVATE
   "${EIGEN_PACKAGE}/include/eigen3"
   "${IMGUI_PACKAGE}/include"
   "${SDL2_PACKAGE}/include/SDL2")
target_link_libraries(hdr-photon-observatory PRIVATE
   Ceres::ceres OpenEXR::OpenEXR "${_imgui_import}" SDL2::SDL2)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_link_options(hdr-photon-observatory PRIVATE "-Wl,--subsystem,windows")
endif()
adecc_test_install(hdr-photon-observatory RUNTIME_CLOSURE
   PACKAGE_ROOTS
      "${CERES_PACKAGE}" "${OPENEXR_PACKAGE}" "${IMATH_PACKAGE}"
      "${ABSEIL_PACKAGE}" "${EIGEN_PACKAGE}" "${OPENJPH_PACKAGE}"
      "${LIBDEFLATE_PACKAGE}" "${ZSTD_PACKAGE}"
      "${IMGUI_PACKAGE}" "${SDL2_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
if(BUILD_TESTING)
   add_test(NAME HDRPhotonObservatoryNative
      COMMAND "$<TARGET_FILE:hdr-photon-observatory>" --verify)
   set_tests_properties(HDRPhotonObservatoryNative PROPERTIES
      LABELS "native;ceres;openexr;imath;hdr;optimization;showcase"
      TIMEOUT 120)
endif()
