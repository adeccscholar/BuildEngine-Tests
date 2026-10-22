cmake_minimum_required(VERSION 3.25)
# Invoke once from the BuildEngine/Test checkout:
# cmake -P bootstrap/Bootstrap.cmake
get_filename_component(TEST_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(BUILDENGINE_ROOT "${TEST_ROOT}/.." ABSOLUTE)
if(NOT DEFINED ENV{BDS} OR "$ENV{BDS}" STREQUAL "")
 message(FATAL_ERROR "BDS is not set; use a C++Builder developer command prompt")
endif()
set(BCC64X "$ENV{BDS}/bin64/bcc64x.exe")
set(TOOLCHAIN "${BUILDENGINE_ROOT}/admin/cmake/toolchains/bcc64x-buildengine-cxx.cmake")
set(NINJA "${BUILDENGINE_ROOT}/tools/ninja/1.13.2/ninja.exe")
foreach(f IN ITEMS "${BCC64X}" "${TOOLCHAIN}" "${NINJA}")
 if(NOT EXISTS "${f}")
   message(FATAL_ERROR "Missing BuildEngine tool: ${f}")
 endif()
endforeach()
file(WRITE "${TEST_ROOT}/CMakeUserPresets.json"
 "{\n  \"version\": 6,\n  \"include\": [\"CMakePresets.json\"],\n  \"configurePresets\": [\n"
 "    { \"name\": \"Release\", \"inherits\": \"base\", \"binaryDir\": \"${BUILDENGINE_ROOT}/build/Test/Release/cmake\", \"cacheVariables\": { \"CMAKE_BUILD_TYPE\": \"Release\", \"BUILDENGINE_CONFIGURATION\": \"Release\", \"CMAKE_INSTALL_PREFIX\": \"${BUILDENGINE_ROOT}/build/Test/Release\" } },\n"
 "    { \"name\": \"Debug\", \"inherits\": \"base\", \"binaryDir\": \"${BUILDENGINE_ROOT}/build/Test/Debug/cmake\", \"cacheVariables\": { \"CMAKE_BUILD_TYPE\": \"Debug\", \"BUILDENGINE_CONFIGURATION\": \"Debug\", \"CMAKE_INSTALL_PREFIX\": \"${BUILDENGINE_ROOT}/build/Test/Debug\" } }\n"
 "  ],\n  \"buildPresets\": [ { \"name\": \"Release\", \"configurePreset\": \"Release\" }, { \"name\": \"Debug\", \"configurePreset\": \"Debug\" } ]\n}\n")
message(STATUS "BuildEngine: ${BUILDENGINE_ROOT}")
message(STATUS "BCC64X: ${BCC64X}")
message(STATUS "Ninja: ${NINJA}")
message(STATUS "CMake presets ready. cmake --preset Release && cmake --build --preset Release")
