cmake_minimum_required(VERSION 3.25)

project(BuildEngineSmokePugiXMLInstalledConsumerCharconv LANGUAGES CXX)

find_package(pugixml CONFIG REQUIRED)

add_executable(pugixml-installed-consumer-charconv
   main.cpp)

target_compile_features(pugixml-installed-consumer-charconv PRIVATE cxx_std_23)
target_link_libraries(pugixml-installed-consumer-charconv PRIVATE pugixml::pugixml)
