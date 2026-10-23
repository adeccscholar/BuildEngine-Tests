cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineOpenMpMandelbrot LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
include(CTest)
adecc_package_current_root(OPENMP_PACKAGE llvm-openmp)
adecc_package_current_root(SDL2_PACKAGE sdl2)
set(_variant "${BUILDENGINE_CONFIGURATION}")
set(_omp_lib "${OPENMP_PACKAGE}/lib/win64/${_variant}/libomp.lib")
foreach(_file IN ITEMS "${OPENMP_PACKAGE}/include/omp.h" "${_omp_lib}")
   if(NOT EXISTS "${_file}")
      message(FATAL_ERROR "Managed OpenMP artifact missing: ${_file}")
   endif()
endforeach()
add_library(BuildEngineOpenMP SHARED IMPORTED)
set_target_properties(BuildEngineOpenMP PROPERTIES
   IMPORTED_IMPLIB "${_omp_lib}"
   INTERFACE_INCLUDE_DIRECTORIES "${OPENMP_PACKAGE}/include")
# Shared compute implementation as object files; no static archives.
add_library(mandelbrot_core OBJECT Mandelbrot.cpp)
# Object library compiles once and links into both demo executables.
set_property(TARGET mandelbrot_core PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(mandelbrot_core PUBLIC cxx_std_23)
target_compile_options(mandelbrot_core PRIVATE -fopenmp)
target_link_libraries(mandelbrot_core PUBLIC BuildEngineOpenMP)
add_executable(openmp_mandelbrot_benchmark benchmark.cpp)
set_property(TARGET openmp_mandelbrot_benchmark PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(openmp_mandelbrot_benchmark PRIVATE mandelbrot_core)
list(PREPEND CMAKE_PREFIX_PATH "${SDL2_PACKAGE}")
find_package(SDL2 CONFIG REQUIRED)
add_executable(openmp_mandelbrot_viewer WIN32 viewer.cpp)
set_property(TARGET openmp_mandelbrot_viewer PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(openmp_mandelbrot_viewer PRIVATE SDL_MAIN_HANDLED)
target_link_options(openmp_mandelbrot_viewer PRIVATE "-Wl,--subsystem,windows")
target_link_libraries(openmp_mandelbrot_viewer PRIVATE mandelbrot_core SDL2::SDL2)
adecc_test_install(openmp_mandelbrot_benchmark
   PACKAGE_ROOTS "${OPENMP_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
adecc_test_install(openmp_mandelbrot_viewer
   PACKAGE_ROOTS "${OPENMP_PACKAGE}" "${SDL2_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
if(BUILD_TESTING)
   add_test(NAME MandelbrotDeterminism COMMAND openmp_mandelbrot_benchmark 320 240 128 4)
endif()
