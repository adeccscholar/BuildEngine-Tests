cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineTestsDocuServer LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")
adecc_package_current_root(BOOST_PACKAGE boost)
adecc_package_current_root(CMARK_PACKAGE cmark-gfm)
add_executable(buildengine_tests_docu_server
 main.cpp docu_server.cpp docu_server.h markdown_renderer.cpp markdown_renderer.h)
set_property(TARGET buildengine_tests_docu_server PROPERTY CXX_SCAN_FOR_MODULES OFF)
set_target_properties(buildengine_tests_docu_server PROPERTIES
 CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_features(buildengine_tests_docu_server PRIVATE cxx_std_23)
adecc_test_use_boost(buildengine_tests_docu_server "${BOOST_PACKAGE}")
target_compile_definitions(buildengine_tests_docu_server PRIVATE
 BOOST_ALL_NO_LIB BOOST_ERROR_CODE_HEADER_ONLY _WIN32_WINNT=0x0A00)
target_link_libraries(buildengine_tests_docu_server PRIVATE ws2_32 mswsock)
adecc_test_install(buildengine_tests_docu_server PACKAGE_ROOTS
 "${BOOST_PACKAGE}" "${CMARK_PACKAGE}"
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
include(CTest)
if(BUILD_TESTING)
 add_test(NAME DocuServerDocumentationLayout
    COMMAND "${CMAKE_COMMAND}" -DROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
    -P "${CMAKE_CURRENT_SOURCE_DIR}/ValidateDocs.cmake")
endif()
