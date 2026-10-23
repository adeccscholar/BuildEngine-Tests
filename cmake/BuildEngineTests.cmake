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
# Explicit per-target package closures. CMake does not guess libraries from PATH.
function(adecc_test_install target)
 cmake_parse_arguments(ARG "" "" "PACKAGE_ROOTS;TOOLCHAIN_DLLS;EXCLUDE_DLLS" ${ARGN})
 if(ARG_PACKAGE_ROOTS)
   adecc_stage_runtime(${target} PACKAGE_ROOTS ${ARG_PACKAGE_ROOTS}
     TOOLCHAIN_DLLS ${ARG_TOOLCHAIN_DLLS} EXCLUDE_DLLS ${ARG_EXCLUDE_DLLS})
 endif()
 # Install a self-contained application in its own target directory.
 # The same exact staged DLLs that are validated in the build tree are
 # installed beside the executable; no PATH lookup or library-wide copy.
 install(TARGETS ${target} RUNTIME DESTINATION "${target}")
 install(DIRECTORY "$<TARGET_FILE_DIR:${target}>/"
    DESTINATION "${target}"
    FILES_MATCHING PATTERN "*.dll" PATTERN "*.DLL")

endfunction()
function(adecc_package_root output id version)
 set(root "${BUILDENGINE_INSTALL_ROOT}/packages/${id}/${version}")
 if(NOT IS_DIRECTORY "${root}")
   message(FATAL_ERROR "Required package missing: ${root}")
 endif()
 set(${output} "${root}" PARENT_SCOPE)
endfunction()
