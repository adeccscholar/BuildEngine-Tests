cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(OpenGLSDL2EarthDemo LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

adecc_package_current_root(SDL2_PACKAGE sdl2)
adecc_package_current_root(GLEW_PACKAGE glew)
adecc_package_current_root(SOIL2_PACKAGE soil2)
adecc_package_current_root(OPENGL_PACKAGE opengl)
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
# BCC64X needs an explicit PE GUI subsystem; WIN32 alone is not sufficient
# with the BuildEngine native linker rule override. Do not open a console.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_link_options(OpenGLSDL2EarthDemo PRIVATE "-Wl,--subsystem,windows")
endif()
# The native Windows GPU driver is exposed through the system opengl32.dll.
# Never bundle Mesa's opengl32.dll with this GPU demo: it selects softpipe
# and can render entirely on the CPU. EXCLUDE_DLLS also purges an older
# staged copy from an incremental build before installation.
adecc_test_install(OpenGLSDL2EarthDemo
 PACKAGE_ROOTS "${SDL2_PACKAGE}" "${GLEW_PACKAGE}" "${SOIL2_PACKAGE}"
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll
 EXCLUDE_DLLS opengl32.dll)
# Remove a Mesa DLL left in the INSTALL prefix by older demo installs.
# The stage exclusion above affects the build output, not pre-existing Release/.
install(CODE "if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/opengl32.dll\")
   file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/opengl32.dll\")
   message(STATUS \"[OpenGL] removed legacy local Mesa opengl32.dll\")
endif()")
# These five assets are required by the real demo. A missing texture or OBJ
# is a configuration error, not an empty graphics directory at runtime.
set(_adecc_earth_assets
   2k_earth_daymap.jpg
   2k_earth_nightmap.jpg
   2k_stars_milky_way.jpg
   adecc_official_logo_planar_cutout_3d_opengl.obj
   adecc_official_logo_planar_cutout_3d_opengl.mtl)
foreach(_asset IN LISTS _adecc_earth_assets)
   if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graphics/${_asset}")
      message(FATAL_ERROR "Missing OpenGL Earth demo asset: graphics/${_asset}")
   endif()
endforeach()
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/graphics/" DESTINATION "graphics"
   FILES_MATCHING PATTERN "*.jpg" PATTERN "*.obj" PATTERN "*.mtl")
