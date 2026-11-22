cmake_minimum_required(VERSION 3.31)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(geometry_cathedral LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
include(CTest)
adecc_package_current_root(CGAL_PACKAGE cgal)
adecc_package_current_root(BOOST_PACKAGE boost)
adecc_package_current_root(ASSIMP_PACKAGE assimp)
adecc_package_current_root(ZLIB_PACKAGE zlib)
adecc_package_current_root(ZSTD_PACKAGE zstd)
adecc_package_current_root(DOCTEST_PACKAGE doctest)
adecc_package_current_root(IMGUI_PACKAGE imgui)
adecc_package_current_root(SDL2_PACKAGE sdl2)
set(CGAL_DIR
   "${CGAL_PACKAGE}/lib/win64/${CMAKE_BUILD_TYPE}/cmake/CGAL"
   CACHE PATH "Managed configuration-specific CGAL CMake package" FORCE)
foreach(theFile IN ITEMS
   "${CGAL_DIR}/CGALConfig.cmake"
   "${CGAL_DIR}/CGALConfig-installation-dirs.cmake"
   "${CGAL_PACKAGE}/include/CGAL/config.h"
   "${CGAL_PACKAGE}/include/CGAL/Simple_cartesian.h")
   if(NOT EXISTS "${theFile}")
      message(FATAL_ERROR "CGAL installed package artifact missing: ${theFile}")
   endif()
endforeach()
message(STATUS "[CGAL] Checked installed package root: ${CGAL_PACKAGE}")
message(STATUS "[CGAL] Checked CMake package: ${CGAL_DIR}")
# Pre-resolve the exact install root even when the package config does not
# include its optional companion during find_package(). This keeps the
# standard CGAL package and target checks intact.
include("${CGAL_DIR}/CGALConfig-installation-dirs.cmake")

set(CGAL_DISABLE_GMP ON CACHE BOOL "CGAL uses the managed Boost number types" FORCE)
set(CGAL_DO_NOT_USE_BOOST_MP OFF CACHE BOOL "Use managed Boost.Multiprecision" FORCE)

# CGAL_SetupBoost.cmake calls find_package(Boost 1.74 REQUIRED). CMake 4.1
# must resolve the exact managed BoostConfig, not the RAD Studio provider.
# Boost installs its CMake package below its selected variant directory.
get_filename_component(strBoostVersion "${BOOST_PACKAGE}" NAME)
if(NOT strBoostVersion MATCHES "^[0-9]+[.][0-9]+[.][0-9]+$")
   message(FATAL_ERROR "Managed Boost package root has no semantic version: ${BOOST_PACKAGE}")
endif()
set(theManagedBoostDir
   "${BOOST_PACKAGE}/lib/win64/${CMAKE_BUILD_TYPE}/cmake/Boost-${strBoostVersion}")
if(NOT EXISTS "${theManagedBoostDir}/BoostConfig.cmake")
   message(FATAL_ERROR "Managed Boost ${CMAKE_BUILD_TYPE} package config is missing: ${theManagedBoostDir}/BoostConfig.cmake")
endif()
set(Boost_DIR "${theManagedBoostDir}" CACHE PATH "Exact managed Boost CMake package" FORCE)
set(Boost_ROOT "${BOOST_PACKAGE}")
set(BOOST_ROOT "${BOOST_PACKAGE}")
list(PREPEND CMAKE_PREFIX_PATH "${BOOST_PACKAGE}")

# Keep the proven BCC64X Clang-specific Boost.Config override first in
# every native compiler invocation, ahead of the installed Boost headers.
set(theBoostClangConfigDir
   "${BUILDENGINE_ROOT}/admin/cmake/consumer/boost/bcc64x-native-clang")
if(NOT EXISTS "${theBoostClangConfigDir}/boost/config/user.hpp")
   message(FATAL_ERROR "Managed BCC64X Clang Boost.Config user.hpp missing: ${theBoostClangConfigDir}")
endif()
set(ADECC_BOOST_MANAGED_INCLUDE_DIR "${BOOST_PACKAGE}/include")
include("${theBoostClangConfigDir}/project-include.cmake")

find_package(Boost "${strBoostVersion}" EXACT CONFIG REQUIRED
   PATHS "${theManagedBoostDir}" NO_DEFAULT_PATH)
message(STATUS "[CGAL] Managed Boost ${Boost_VERSION}: ${Boost_DIR}")

find_package(CGAL 6.2.1 EXACT CONFIG REQUIRED
   PATHS "${CGAL_DIR}" NO_DEFAULT_PATH)
if(NOT TARGET CGAL::CGAL)
   message(FATAL_ERROR "Installed CGAL config did not define CGAL::CGAL")
endif()

set(assimp_DIR "${ASSIMP_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/assimp-6.0")
list(PREPEND CMAKE_PREFIX_PATH "${ZLIB_PACKAGE}" "${ZSTD_PACKAGE}")
find_package(assimp 6.0.6 EXACT CONFIG REQUIRED PATHS "${assimp_DIR}" NO_DEFAULT_PATH)
find_package(SDL2 CONFIG REQUIRED PATHS "${SDL2_PACKAGE}" NO_DEFAULT_PATH)
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_suffix d)
else()
   set(_suffix "")
endif()
set(_imgui "${IMGUI_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libimgui${_suffix}.lib")
if(NOT EXISTS "${_imgui}")
   message(FATAL_ERROR "Missing managed ImGui import: ${_imgui}")
endif()
add_executable(geometry-cathedral WIN32 main.cpp)
set_target_properties(geometry-cathedral PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(geometry-cathedral PRIVATE SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(geometry-cathedral BEFORE PRIVATE "${theBoostClangConfigDir}" "${BOOST_PACKAGE}/include" "${DOCTEST_PACKAGE}/include" "${IMGUI_PACKAGE}/include" "${SDL2_PACKAGE}/include/SDL2")
target_link_libraries(geometry-cathedral PRIVATE CGAL::CGAL assimp::assimp "${_imgui}" SDL2::SDL2)
target_link_options(geometry-cathedral PRIVATE "-Wl,--subsystem,windows")
adecc_test_install(geometry-cathedral RUNTIME_CLOSURE PACKAGE_ROOTS "${CGAL_PACKAGE}" "${BOOST_PACKAGE}" "${ASSIMP_PACKAGE}" "${ZLIB_PACKAGE}" "${ZSTD_PACKAGE}" "${DOCTEST_PACKAGE}" "${IMGUI_PACKAGE}" "${SDL2_PACKAGE}" TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
if(BUILD_TESTING)
   add_test(NAME GeometryCathedralVerify COMMAND "$<TARGET_FILE:geometry-cathedral>" --verify)
   set_tests_properties(GeometryCathedralVerify PROPERTIES LABELS "native;cgal;assimp;doctest;showcase" TIMEOUT 120)
endif()
