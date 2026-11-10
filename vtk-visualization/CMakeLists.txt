# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineVTKVisualization VERSION 1.0 LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
include(CTest)

foreach(_id IN ITEMS vtk onetbb llvm-openmp opengl)
   string(REPLACE "-" "_" _key "${_id}")
   string(TOUPPER "${_key}" _key)
   adecc_package_current_root("${_key}_PACKAGE" "${_id}")
endforeach()
set(VTK_DIR "${VTK_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/vtk-9.7")
set(TBB_DIR "${ONETBB_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/TBB")
set(OPENGL_gl_LIBRARY "${OPENGL_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/opengl32.lib")
set(OPENGL_INCLUDE_DIR "${OPENGL_PACKAGE}/include")
set(OpenGL_GL_PREFERENCE LEGACY)
set(OpenMP_CXX_FLAGS "-fopenmp")
set(OpenMP_CXX_LIB_NAMES omp)
set(OpenMP_omp_LIBRARY "${LLVM_OPENMP_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libomp.lib")
foreach(_required IN ITEMS
   "${OPENGL_gl_LIBRARY}" "${OpenMP_omp_LIBRARY}"
   "${TBB_DIR}/TBBConfig.cmake" "${VTK_DIR}/vtk-config.cmake")
   if(NOT EXISTS "${_required}")
      message(FATAL_ERROR "Missing managed BuildEngine dependency: ${_required}")
   endif()
endforeach()
list(PREPEND CMAKE_PREFIX_PATH "${VTK_PACKAGE}" "${ONETBB_PACKAGE}" "${LLVM_OPENMP_PACKAGE}" "${OPENGL_PACKAGE}")
find_package(TBB CONFIG REQUIRED COMPONENTS tbb PATHS "${TBB_DIR}" NO_DEFAULT_PATH)
find_package(VTK 9.7.2 EXACT CONFIG REQUIRED COMPONENTS
   CommonCore CommonDataModel FiltersCore FiltersSources
   IOImage ImagingCore RenderingCore RenderingOpenGL2 RenderingFreeType
   RenderingVolume RenderingVolumeOpenGL2 RenderingAnnotation InteractionStyle)
add_executable(vtk-visualization main.cpp)
set_target_properties(vtk-visualization PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(vtk-visualization PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(vtk-visualization PRIVATE
   VTK::CommonCore VTK::CommonDataModel VTK::FiltersCore VTK::FiltersSources
   VTK::IOImage VTK::ImagingCore VTK::RenderingCore VTK::RenderingOpenGL2
   VTK::RenderingFreeType VTK::RenderingVolume
   VTK::RenderingVolumeOpenGL2 VTK::RenderingAnnotation VTK::InteractionStyle)
vtk_module_autoinit(TARGETS vtk-visualization MODULES
   VTK::RenderingCore VTK::RenderingOpenGL2 VTK::RenderingFreeType
   VTK::RenderingVolume VTK::RenderingVolumeOpenGL2 VTK::InteractionStyle)
# Use Windows' hardware ICD at runtime, not the managed Mesa softpipe DLL.
# Mesa is used only as a pinned build/link package.
adecc_test_install(vtk-visualization
   PACKAGE_ROOTS "${VTK_PACKAGE}" "${ONETBB_PACKAGE}" "${LLVM_OPENMP_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll
   EXCLUDE_DLLS opengl32.dll)
install(CODE "if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/opengl32.dll\")
   file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/opengl32.dll\")
endif()")

if(BUILD_TESTING)
   add_test(NAME VTK972Geometry COMMAND "$<TARGET_FILE:vtk-visualization>" --verify --resolution 129 --backend Sequential)
   add_test(NAME VTK972TBB COMMAND "$<TARGET_FILE:vtk-visualization>" --verify --resolution 129 --backend TBB)
   add_test(NAME VTK972OpenMP COMMAND "$<TARGET_FILE:vtk-visualization>" --verify --resolution 129 --backend OpenMP)
   add_test(NAME VTK972Load COMMAND "$<TARGET_FILE:vtk-visualization>" --stress --resolution 193 --iterations 4 --backend TBB)
   set_tests_properties(VTK972Load PROPERTIES LABELS "load;vtk" TIMEOUT 240)
   option(VTK_ENABLE_GRAPHICS_CTEST "Run graphics CTest requiring an interactive Windows desktop" OFF)
   if(VTK_ENABLE_GRAPHICS_CTEST)
      add_test(NAME VTK972Screenshot COMMAND "$<TARGET_FILE:vtk-visualization>"
         --screenshot "${CMAKE_CURRENT_BINARY_DIR}/VTK972.png" --resolution 129 --backend TBB)
      set_tests_properties(VTK972Screenshot PROPERTIES LABELS "graphics;vtk" TIMEOUT 120)
   endif()
endif()
