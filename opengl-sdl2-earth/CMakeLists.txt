cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(OpenGLSDL2EarthDemo LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

adecc_package_root(SDL2_PACKAGE sdl2 2.32.10)
adecc_package_root(GLEW_PACKAGE glew 2.3.1)
adecc_package_root(SOIL2_PACKAGE soil2 1.3.0)
adecc_package_root(OPENGL_PACKAGE opengl 26.2.1)
list(PREPEND CMAKE_PREFIX_PATH
 "${SDL2_PACKAGE}" "${GLEW_PACKAGE}" "${SOIL2_PACKAGE}" "${OPENGL_PACKAGE}")
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(SOIL2 CONFIG REQUIRED)

add_executable(OpenGLSDL2EarthDemo WIN32
 src/main.cpp src/EarthScene.cpp src/LogoScene.cpp)
set_property(TARGET OpenGLSDL2EarthDemo PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(OpenGLSDL2EarthDemo PRIVATE cxx_std_23)
target_compile_definitions(OpenGLSDL2EarthDemo PRIVATE
 SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(OpenGLSDL2EarthDemo PRIVATE
 SDL2::SDL2 GLEW::GLEW SOIL2::SOIL2 opengl32)
adecc_test_install(OpenGLSDL2EarthDemo
 PACKAGE_ROOTS "${SDL2_PACKAGE}" "${GLEW_PACKAGE}" "${SOIL2_PACKAGE}" "${OPENGL_PACKAGE}"
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/graphics/" DESTINATION "graphics")
