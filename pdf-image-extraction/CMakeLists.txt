cmake_minimum_required(VERSION 3.25)

project(BuildEnginePdfImageExtraction LANGUAGES CXX)

find_package(qpdf CONFIG REQUIRED)

add_executable(pdf-image-extraction
   main.cpp)

set_property(TARGET pdf-image-extraction PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdf-image-extraction PRIVATE cxx_std_23)
target_link_libraries(pdf-image-extraction PRIVATE qpdf::libqpdf)
