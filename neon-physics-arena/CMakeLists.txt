# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineNeonPhysicsArena VERSION 1.0 LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
include(CTest)

# Exact active packages from the BuildEngine catalog; no PATH probing.
adecc_package_current_root(SDL3_PACKAGE sdl3)
adecc_package_current_root(BOX2D_PACKAGE box2d)
adecc_package_current_root(JOLT_PACKAGE jolt-physics)

set(_sdl3_config "${SDL3_PACKAGE}/lib/cmake/SDL3/SDL3Config.cmake")
set(_box2d_config "${BOX2D_PACKAGE}/lib/cmake/box2d/box2dConfig.cmake")
set(_jolt_config "${JOLT_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/cmake/Jolt/JoltConfig.cmake")
foreach(_config IN ITEMS "${_sdl3_config}" "${_box2d_config}")
   if(NOT EXISTS "${_config}")
      message(FATAL_ERROR "Missing native BuildEngine CMake package: ${_config}")
   endif()
endforeach()

find_package(SDL3 CONFIG REQUIRED
   PATHS "${SDL3_PACKAGE}/lib/cmake/SDL3" NO_DEFAULT_PATH)
find_package(box2d CONFIG REQUIRED
   PATHS "${BOX2D_PACKAGE}/lib/cmake/box2d" NO_DEFAULT_PATH)

if(NOT TARGET SDL3::SDL3 OR NOT TARGET box2d::box2d)
   message(FATAL_ERROR "Native SDL3 and Box2D DLL targets are both required")
endif()
if(NOT BUILDENGINE_CONFIGURATION MATCHES "^(Debug|Release)$")
   message(FATAL_ERROR "BUILDENGINE_CONFIGURATION must be Release or Debug")
endif()

string(TOUPPER "${BUILDENGINE_CONFIGURATION}" _variant)
foreach(_target IN ITEMS SDL3::SDL3 box2d::box2d)
   get_target_property(_type "${_target}" TYPE)
   if(NOT _type STREQUAL "SHARED_LIBRARY")
      message(FATAL_ERROR "Expected native shared DLL package: ${_target}")
   endif()
   get_target_property(_dll "${_target}" "IMPORTED_LOCATION_${_variant}")
   get_target_property(_lib "${_target}" "IMPORTED_IMPLIB_${_variant}")
   foreach(_artifact IN ITEMS "${_dll}" "${_lib}")
      if(NOT EXISTS "${_artifact}")
         message(FATAL_ERROR "Missing ${BUILDENGINE_CONFIGURATION} managed artifact: ${_artifact}")
      endif()
   endforeach()
endforeach()

# A local object library is not an additional third-party package.
add_library(neon_physics_core OBJECT PhysicsArena.cpp PhysicsArena.h)
set_target_properties(neon_physics_core PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(neon_physics_core PUBLIC box2d::box2d)

add_executable(neon-physics-arena WIN32 main.cpp)
set_target_properties(neon-physics-arena PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_compile_definitions(neon-physics-arena PRIVATE
   SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(neon-physics-arena PRIVATE
   neon_physics_core SDL3::SDL3 user32)
target_link_options(neon-physics-arena PRIVATE "-Wl,--subsystem,windows")

add_executable(neon-physics-verify verify.cpp)
set_target_properties(neon-physics-verify PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(neon-physics-verify PRIVATE neon_physics_core SDL3::SDL3)

# Independently installed Jolt DLL: differential physics is an additional
# executable, not a replacement for the existing SDL3/Box2D arena.
if(BUILDENGINE_CONFIGURATION STREQUAL "Debug")
   set(_jolt_suffix "d")
else()
   set(_jolt_suffix "")
endif()
set(_jolt_lib "${JOLT_PACKAGE}/lib/win64/${BUILDENGINE_CONFIGURATION}/libJolt${_jolt_suffix}.lib")
set(_jolt_dll "${JOLT_PACKAGE}/bin/win64/${BUILDENGINE_CONFIGURATION}/libJolt${_jolt_suffix}.dll")
foreach(_artifact IN ITEMS "${_jolt_lib}" "${_jolt_dll}")
   if(NOT EXISTS "${_artifact}")
      message(FATAL_ERROR "Native Jolt shared DLL artifact missing: ${_artifact}")
   endif()
endforeach()

add_executable(neon-physics-duel JoltDifferential.cpp)
set_target_properties(neon-physics-duel PROPERTIES
   CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
   CXX_SCAN_FOR_MODULES OFF)
target_include_directories(neon-physics-duel PRIVATE "${JOLT_PACKAGE}/include")
# Match the accepted Jolt 5.7.0 Release consumer ABI definition set.
target_compile_definitions(neon-physics-duel PRIVATE
   JPH_SHARED_LIBRARY JPH_OBJECT_LAYER_BITS=16 JPH_OBJECT_STREAM
   JPH_DEBUG_RENDERER JPH_PROFILE_ENABLED JPH_USE_SSE4_1 JPH_USE_SSE4_2
   JPH_USE_AVX JPH_USE_AVX2 JPH_USE_LZCNT JPH_USE_TZCNT JPH_USE_F16C
   JPH_USE_FMADD)
target_link_libraries(neon-physics-duel PRIVATE box2d::box2d "${_jolt_lib}")
adecc_test_install(neon-physics-duel RUNTIME_CLOSURE
   PACKAGE_ROOTS "${JOLT_PACKAGE}" "${BOX2D_PACKAGE}"
   TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)

foreach(_target IN ITEMS neon-physics-arena neon-physics-verify)
   adecc_test_install(${_target}
      PACKAGE_ROOTS "${SDL3_PACKAGE}" "${BOX2D_PACKAGE}"
      TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll libwinpthread-1-370.dll)
endforeach()

if(BUILD_TESTING)
   add_test(NAME NeonPhysicsDifferential
      COMMAND "$<TARGET_FILE:neon-physics-duel>")
   set_tests_properties(NeonPhysicsDifferential PROPERTIES
      LABELS "native;physics;jolt;box2d;differential" TIMEOUT 180)
   add_test(NAME NeonPhysicsDeterminism
      COMMAND "$<TARGET_FILE:neon-physics-verify>")
   set_tests_properties(NeonPhysicsDeterminism PROPERTIES
      LABELS "native;sdl3;box2d;physics" TIMEOUT 90)
   add_test(NAME NeonPhysicsStress
      COMMAND "$<TARGET_FILE:neon-physics-verify>" --stress)
   set_tests_properties(NeonPhysicsStress PROPERTIES
      LABELS "native;sdl3;box2d;stress" TIMEOUT 180)
endif()
