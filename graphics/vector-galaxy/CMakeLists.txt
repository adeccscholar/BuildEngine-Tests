cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(vector_galaxy LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
include(CTest)

foreach(_id IN ITEMS hnswlib libigl eigen imgui sdl2)
   string(TOUPPER "${_id}" _name)
   adecc_package_current_root(${_name}_PACKAGE ${_id})
endforeach()
list(PREPEND CMAKE_PREFIX_PATH "${SDL2_PACKAGE}")
find_package(SDL2 CONFIG REQUIRED PATHS "${SDL2_PACKAGE}" NO_DEFAULT_PATH)
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_imgui_suffix d)
else()
   set(_imgui_suffix "")
endif()
set(_imgui_import "${IMGUI_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libimgui${_imgui_suffix}.lib")
if(NOT EXISTS "${_imgui_import}")
   message(FATAL_ERROR "Managed ImGui DLL import missing: ${_imgui_import}")
endif()

add_executable(vector-galaxy WIN32 main.cpp)
set_target_properties(vector-galaxy PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(vector-galaxy PRIVATE SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(vector-galaxy PRIVATE
   "${HNSWLIB_PACKAGE}/include"
   "${LIBIGL_PACKAGE}/include"
   "${EIGEN_PACKAGE}/include/eigen3"
   "${IMGUI_PACKAGE}/include"
   "${SDL2_PACKAGE}/include/SDL2")
target_link_libraries(vector-galaxy PRIVATE "${_imgui_import}" SDL2::SDL2)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_link_options(vector-galaxy PRIVATE "-Wl,--subsystem,windows")
endif()
adecc_test_install(vector-galaxy RUNTIME_CLOSURE
   PACKAGE_ROOTS "${HNSWLIB_PACKAGE}" "${LIBIGL_PACKAGE}" "${EIGEN_PACKAGE}" "${IMGUI_PACKAGE}" "${SDL2_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
if(BUILD_TESTING)
   add_test(NAME VectorGalaxyAlgorithm COMMAND "$<TARGET_FILE:vector-galaxy>" --verify)
   set_tests_properties(VectorGalaxyAlgorithm PROPERTIES LABELS "native;hnswlib;libigl;geometry;showcase" TIMEOUT 60)
endif()
