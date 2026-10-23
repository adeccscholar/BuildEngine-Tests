cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_SOURCE_DIR}/../bootstrap/PrepareToolchain.cmake")
project(BuildEngineFinancial LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BuildEngineTests.cmake")

adecc_package_root(BOOST_PACKAGE boost 1.92.0)
adecc_package_root(PUGIXML_PACKAGE pugixml 1.16)
adecc_package_root(OPENSSL_PACKAGE openssl 3.5.8)
list(PREPEND CMAKE_PREFIX_PATH "${PUGIXML_PACKAGE}" "${OPENSSL_PACKAGE}" "${BOOST_PACKAGE}")
find_package(pugixml CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)

add_executable(financial_demo
 main.cpp FinancialJson.cpp HttpsClient.cpp EcbHttpsDownload.cpp
 OpenHolidaysQuery.cpp EcbPublicationCheck.cpp EcbHistoryCache.cpp
 BoostJsonCompiled.cpp EcbData.cpp)
set_property(TARGET financial_demo PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(financial_demo PRIVATE cxx_std_23)
target_include_directories(financial_demo PRIVATE "${BOOST_PACKAGE}/include")
target_compile_definitions(financial_demo PRIVATE
 BOOST_ALL_NO_LIB BOOST_JSON_NO_LIB BOOST_SYSTEM_NO_LIB _WIN32_WINNT=0x0A00)
target_link_libraries(financial_demo PRIVATE
 pugixml::pugixml OpenSSL::SSL OpenSSL::Crypto crypt32)

set(FINANCIAL_RUNTIME_ROOTS
 "${PUGIXML_PACKAGE};${OPENSSL_PACKAGE};${BOOST_PACKAGE}")
adecc_test_install(financial_demo PACKAGE_ROOTS ${FINANCIAL_RUNTIME_ROOTS}
 TOOLCHAIN_DLLS libc++-370.dll libunwind-370.dll)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Tests/" DESTINATION "Tests"
 FILES_MATCHING PATTERN "*.json")
