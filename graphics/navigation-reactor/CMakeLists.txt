cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../bootstrap/PrepareToolchain.cmake")
project(navigation_reactor LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BuildEngineTests.cmake")
include(CTest)

adecc_package_current_root(RECAST_PACKAGE recastnavigation)
adecc_package_current_root(OPENGL_PACKAGE opengl)
adecc_package_current_root(GLEW_PACKAGE glew)
adecc_package_current_root(IMGUI_PACKAGE imgui)
adecc_package_current_root(SDL2_PACKAGE sdl2)
list(PREPEND CMAKE_PREFIX_PATH "${SDL2_PACKAGE}" "${OPENGL_PACKAGE}" "${GLEW_PACKAGE}")
set(OpenGL_DIR "${OPENGL_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/OpenGL")
set(GLEW_DIR "${GLEW_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/GLEW")
find_package(OpenGL CONFIG REQUIRED PATHS "${OpenGL_DIR}" NO_DEFAULT_PATH)
find_package(GLEW CONFIG REQUIRED PATHS "${GLEW_DIR}" NO_DEFAULT_PATH)
if(NOT EXISTS "${OpenGL_RUNTIME_DLL}" OR NOT EXISTS "${OpenGL_GALLIUM_DLL}")
   message(FATAL_ERROR "Managed Mesa runtime pair is unavailable")
endif()
find_package(SDL2 CONFIG REQUIRED PATHS "${SDL2_PACKAGE}" NO_DEFAULT_PATH)
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_recast_suffix "-d")
   set(_imgui_suffix d)
else()
   set(_recast_suffix "")
   set(_imgui_suffix "")
endif()
set(_recast_import "${RECAST_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libRecast${_recast_suffix}.lib")
set(_detour_import "${RECAST_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libDetour${_recast_suffix}.lib")
set(_imgui_import "${IMGUI_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libimgui${_imgui_suffix}.lib")
set(_imgui_backend_import "${IMGUI_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libimgui_sdl2_opengl3${_imgui_suffix}.lib")
foreach(_lib IN ITEMS "${_recast_import}" "${_detour_import}" "${_imgui_import}" "${_imgui_backend_import}")
   if(NOT EXISTS "${_lib}")
      message(FATAL_ERROR "Managed DLL import missing: ${_lib}")
   endif()
endforeach()

add_executable(navigation-reactor WIN32 main.cpp)
set_target_properties(navigation-reactor PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(navigation-reactor PRIVATE
   SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX
   ADECC_MESA_OPENGL_DLL="${OpenGL_RUNTIME_DLL}")
target_include_directories(navigation-reactor PRIVATE
   "${RECAST_PACKAGE}/include/recastnavigation"
   "${GLEW_PACKAGE}/include"
   "${OPENGL_PACKAGE}/include"
   "${IMGUI_PACKAGE}/include"
   "${SDL2_PACKAGE}/include/SDL2")
target_link_libraries(navigation-reactor PRIVATE
   "${_recast_import}" "${_detour_import}" "${_imgui_import}" "${_imgui_backend_import}"
   SDL2::SDL2 OpenGL::OpenGL GLEW::GLEW)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_link_options(navigation-reactor PRIVATE "-Wl,--subsystem,windows")
endif()
adecc_test_install(navigation-reactor RUNTIME_CLOSURE
   PACKAGE_ROOTS "${RECAST_PACKAGE}" "${OPENGL_PACKAGE}" "${GLEW_PACKAGE}" "${IMGUI_PACKAGE}" "${SDL2_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
# Mesa loads the Gallium WGL implementation dynamically: a PE import scan alone
# cannot discover it. Stage only the two exact, versioned managed Mesa DLLs.
add_custom_command(TARGET buildengine-stage-navigation-reactor POST_BUILD
   COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${OpenGL_RUNTIME_DLL}" "$<TARGET_FILE_DIR:navigation-reactor>/opengl32.dll"
   COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${OpenGL_GALLIUM_DLL}" "$<TARGET_FILE_DIR:navigation-reactor>/libgallium_wgl.dll"
   COMMENT "Stage managed Mesa WGL pair without SDK-wide DLL copying"
   VERBATIM)
if(BUILD_TESTING)
   add_test(NAME NavigationReactorRecastRasterization COMMAND "$<TARGET_FILE:navigation-reactor>" --verify)
   set_tests_properties(NavigationReactorRecastRasterization PROPERTIES LABELS "native;recast;terrain;showcase" TIMEOUT 60)
endif()
