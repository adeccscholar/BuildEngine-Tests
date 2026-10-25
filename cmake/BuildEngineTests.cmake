# SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

include_guard(GLOBAL)
get_filename_component(BUILDENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BUILDENGINE_ROOT "${BUILDENGINE_ROOT}" CACHE PATH "BuildEngine workspace root")
set(BUILDENGINE_INSTALL_ROOT "${BUILDENGINE_ROOT}/install" CACHE PATH "Installed BuildEngine packages")
set(BUILDENGINE_CONFIGURATION "Release" CACHE STRING "BuildEngine binary variant")
set(ADECC_RUNTIME_CMAKE_DIR "${BUILDENGINE_ROOT}/admin/cmake/consumer" CACHE PATH "Existing BuildEngine runtime staging")
if(NOT EXISTS "${ADECC_RUNTIME_CMAKE_DIR}/BuildEngineRuntime.cmake")
 message(FATAL_ERROR "Missing BuildEngine runtime integration: ${ADECC_RUNTIME_CMAKE_DIR}")
endif()
include("${ADECC_RUNTIME_CMAKE_DIR}/BuildEngineRuntime.cmake")
set(_SDK "${BUILDENGINE_INSTALL_ROOT}/Win64x")
if(EXISTS "${_SDK}/lib/cmake")
 list(PREPEND CMAKE_PREFIX_PATH "${_SDK}")
endif()
# Use the exact Boost.Config compiler classification from the production
# BuildEngine-Admin Boost consumer/producer contract. BCC64X must be classified
# as Clang, not as a legacy CodeGear compiler or emulated GCC.
function(adecc_test_use_boost target package_root)
 if(NOT TARGET "${target}")
   message(FATAL_ERROR "Unknown Boost consumer target: ${target}")
 endif()
 set(_boost_config_root "${BUILDENGINE_ROOT}/admin/cmake/boost/bcc64x-native-clang")
 if(NOT EXISTS "${_boost_config_root}/boost/config/user.hpp")
   message(FATAL_ERROR "Missing production BCC64X Boost.Config override: ${_boost_config_root}/boost/config/user.hpp")
 endif()
 if(NOT EXISTS "${package_root}/include/boost/config.hpp")
   message(FATAL_ERROR "Missing managed Boost headers: ${package_root}/include/boost/config.hpp")
 endif()
 target_include_directories(${target} BEFORE PRIVATE
   "${_boost_config_root}" "${package_root}/include")
 message(STATUS "BuildEngine Boost.Config policy: production BCC64X native Clang")
endfunction()


# Use the already checked-out, separately licensed adecc-core-lib as true
# CMake INTERFACE targets (no copied sources, and no static core archive).
set(BUILDENGINE_TESTS_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "BuildEngine tests checkout")

function(adecc_test_use_core target component)
   set(_source "${BUILDENGINE_TESTS_ROOT}/adecc-core-lib")
   if(NOT EXISTS "${_source}/CMakeLists.txt")
      message(FATAL_ERROR
         "adecc-core-lib submodule missing: ${_source}; run git submodule update --init --recursive")
   endif()
   if(NOT TARGET adecc::core)
      add_subdirectory("${_source}" "${CMAKE_CURRENT_BINARY_DIR}/adecc-core-lib" EXCLUDE_FROM_ALL)
   endif()
   if(NOT TARGET adecc::${component})
      message(FATAL_ERROR "adecc-core-lib does not provide INTERFACE target adecc::${component}")
   endif()
   target_link_libraries(${target} PRIVATE adecc::${component})
   # Legacy exercises use short public header names; the include paths are
   # provided as an INTERFACE adapter rather than editing the exercises.
   if(NOT TARGET buildengine_adecc_header_paths)
      add_library(buildengine_adecc_header_paths INTERFACE)
      target_include_directories(buildengine_adecc_header_paths INTERFACE
         "${_source}/adecc/cpp_core"
         "${_source}/adecc/cpp_domain"
         "${_source}/adecc/mssql"
         "${_source}/adecc/postgre"
         "${_source}/adecc/openXlsxTools")
   endif()
   target_link_libraries(${target} PRIVATE buildengine_adecc_header_paths)
endfunction()

# Explicit per-target package closures. CMake does not guess libraries from PATH.
function(adecc_test_install target)
 cmake_parse_arguments(ARG "RUNTIME_CLOSURE" "" "PACKAGE_ROOTS;TOOLCHAIN_DLLS;EXCLUDE_DLLS" ${ARGN})
 if(ARG_UNPARSED_ARGUMENTS OR ARG_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "Invalid runtime arguments for ${target}")
 endif()
 if(ARG_RUNTIME_CLOSURE)
    set_target_properties(${target} PROPERTIES
       RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime/${target}")
    set(_roots_file "${CMAKE_CURRENT_BINARY_DIR}/${target}.runtime-roots.txt")
    set(_toolchain_file "${CMAKE_CURRENT_BINARY_DIR}/${target}.runtime-toolchain.txt")
    string(REPLACE ";" "\n" _roots "${ARG_PACKAGE_ROOTS}")
    string(REPLACE ";" "\n" _dlls "${ARG_TOOLCHAIN_DLLS}")
    file(WRITE "${_roots_file}" "${_roots}\n")
    file(WRITE "${_toolchain_file}" "${_dlls}\n")
    if(ARG_TOOLCHAIN_DLLS)
       if(NOT DEFINED ENV{CB_BDS} OR "$ENV{CB_BDS}" STREQUAL "")
          message(FATAL_ERROR "BCC64X runtime requires CB_BDS")
       endif()
       set(_toolbin "$ENV{CB_BDS}/bin64")
    else()
       set(_toolbin "")
    endif()
    add_custom_target("buildengine-stage-${target}" ALL
       COMMAND "${CMAKE_COMMAND}"
          "-DADECC_RUNTIME_EXECUTABLE:FILEPATH=$<TARGET_FILE:${target}>"
          "-DADECC_RUNTIME_OUTPUT_DIR:PATH=$<TARGET_FILE_DIR:${target}>"
          "-DADECC_RUNTIME_ROOTS_FILE:FILEPATH=${_roots_file}"
          "-DADECC_RUNTIME_TOOLCHAIN_DLLS_FILE:FILEPATH=${_toolchain_file}"
          "-DADECC_RUNTIME_TOOLCHAIN_BIN:PATH=${_toolbin}"
          "-DADECC_RUNTIME_CONFIGURATION=${BUILDENGINE_CONFIGURATION}"
          -P "${BUILDENGINE_TESTS_ROOT}/cmake/StageRuntimeClosure.cmake"
       COMMENT "Stage exact PE import closure for ${target}" VERBATIM)
    add_dependencies("buildengine-stage-${target}" ${target})
 else()
    if(ARG_TOOLCHAIN_DLLS AND NOT ARG_PACKAGE_ROOTS)
       message(FATAL_ERROR "Avoid SDK fallback: specify PACKAGE_ROOTS or RUNTIME_CLOSURE for ${target}")
    endif()
    if(ARG_PACKAGE_ROOTS)
       adecc_stage_runtime(${target} PACKAGE_ROOTS ${ARG_PACKAGE_ROOTS}
          TOOLCHAIN_DLLS ${ARG_TOOLCHAIN_DLLS} EXCLUDE_DLLS ${ARG_EXCLUDE_DLLS})
    endif()
 endif()
 install(TARGETS ${target} RUNTIME DESTINATION ".")
 install(DIRECTORY "$<TARGET_FILE_DIR:${target}>/"
    DESTINATION "." FILES_MATCHING PATTERN "*.dll" PATTERN "*.DLL")
endfunction()

function(adecc_package_root output id version)
 set(root "${BUILDENGINE_INSTALL_ROOT}/packages/${id}/${version}")
 if(NOT IS_DIRECTORY "${root}")
   message(FATAL_ERROR "Required package missing: ${root}")
 endif()
 set(${output} "${root}" PARENT_SCOPE)
endfunction()

# Select the unique active package version from the BuildEngine XML contract.
# supersededBy entries are intentionally not eligible. No directory guessing.
function(adecc_package_current_root output id)
 set(_catalog "${BUILDENGINE_ROOT}/admin/build-libraries.xml")
 if(NOT EXISTS "${_catalog}")
   message(FATAL_ERROR "BuildEngine library catalog missing: ${_catalog}")
 endif()
 # A catalog update must reconfigure consumers and refresh active versions.
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_catalog}")
 file(STRINGS "${_catalog}" _definitions REGEX "<library id=\"${id}\" version=\"")
 set(_current_versions "")
 foreach(_line IN LISTS _definitions)
   if(_line MATCHES "<library id=\"${id}\" version=\"([^\"]+)\"")
     set(_version "${CMAKE_MATCH_1}")
     if(NOT _line MATCHES "supersededBy=")
       list(APPEND _current_versions "${_version}")
     endif()
   endif()
 endforeach()
 list(LENGTH _current_versions _count)
 if(NOT _count EQUAL 1)
   message(FATAL_ERROR "Expected exactly one active version of ${id} in ${_catalog}, got: ${_current_versions}")
 endif()
 list(GET _current_versions 0 _version)
 adecc_package_root(_package "${id}" "${_version}")
 message(STATUS "BuildEngine active package: ${id}@${_version}")
 set(${output} "${_package}" PARENT_SCOPE)
endfunction()
