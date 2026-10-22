cmake_minimum_required(VERSION 3.25)
get_filename_component(TEST_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(BUILDENGINE_ROOT "${TEST_ROOT}/.." ABSOLUTE)
if("$ENV{BDS}" STREQUAL "")
 message(FATAL_ERROR "BDS missing; use RAD Studio 13 developer environment")
endif()
foreach(path IN ITEMS
 "$ENV{BDS}/bin64/bcc64x.exe"
 "${BUILDENGINE_ROOT}/admin/cmake/toolchains/bcc64x-buildengine-cxx.cmake"
 "${BUILDENGINE_ROOT}/tools/ninja/1.13.2/ninja.exe"
 "${BUILDENGINE_ROOT}/admin/cmake/consumer/BuildEngineRuntime.cmake")
 if(NOT EXISTS "${path}")
   message(FATAL_ERROR "BuildEngine input missing: ${path}")
 endif()
endforeach()
message(STATUS "Tests root: ${TEST_ROOT}")
message(STATUS "BuildEngine root: ${BUILDENGINE_ROOT}")
message(STATUS "Use: cmake --preset Release")
message(STATUS "Then: cmake --build --preset Release")
message(STATUS "Finally: cmake --install ../build/Test/Release/cmake")
