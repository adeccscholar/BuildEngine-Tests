cmake_minimum_required(VERSION 3.25)
get_filename_component(TEST_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(BUILDENGINE_ROOT "${TEST_ROOT}/.." ABSOLUTE)
if("$ENV{BDS}" STREQUAL "")
 message(FATAL_ERROR "BDS missing; use RAD Studio 13 developer environment")
endif()
set(TOOLCHAIN "${BUILDENGINE_ROOT}/admin/cmake/toolchains/bcc64x-buildengine-cxx.cmake")
set(NINJA "${BUILDENGINE_ROOT}/tools/ninja/1.13.2/ninja.exe")
foreach(path IN ITEMS
 "$ENV{BDS}/bin64/bcc64x.exe"
 "${TOOLCHAIN}"
 "${NINJA}"
 "${BUILDENGINE_ROOT}/admin/cmake/consumer/BuildEngineRuntime.cmake")
 if(NOT EXISTS "${path}")
   message(FATAL_ERROR "BuildEngine input missing: ${path}")
 endif()
endforeach()
# One shared initial cache; never generate per-project CMD files or presets.
file(WRITE "${TEST_ROOT}/bootstrap/BuildEngineInit.cmake"
 "set(CMAKE_TOOLCHAIN_FILE \"${TOOLCHAIN}\" CACHE FILEPATH \"\" FORCE)\n"
 "set(CMAKE_MAKE_PROGRAM \"${NINJA}\" CACHE FILEPATH \"\" FORCE)\n"
 "set(BUILDENGINE_ROOT \"${BUILDENGINE_ROOT}\" CACHE PATH \"\" FORCE)\n"
 "set(BUILDENGINE_INSTALL_ROOT \"${BUILDENGINE_ROOT}/install\" CACHE PATH \"\" FORCE)\n")
message(STATUS "Bootstrap configured: ${TEST_ROOT}/bootstrap/BuildEngineInit.cmake")
message(STATUS "Per project: cmake -S CompareTest -B ../build/Test/CompareTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CompareTest/Release")
message(STATUS "Build: cmake --build ../build/Test/CompareTest/Release/cmake")
message(STATUS "Install: cmake --install ../build/Test/CompareTest/Release/cmake")
