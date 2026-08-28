cmake_minimum_required(VERSION 3.25)

project(BuildEngineSmokeCurlInstalledConsumerRuntime LANGUAGES CXX)

find_package(CURL CONFIG REQUIRED)

add_executable(curl-installed-consumer-runtime
   main.cpp)

target_compile_features(curl-installed-consumer-runtime PRIVATE cxx_std_23)
target_link_libraries(curl-installed-consumer-runtime PRIVATE CURL::libcurl)
