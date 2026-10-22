cmake_minimum_required(VERSION 3.25)
project(BuildEnginePDFiumSDL2Viewer LANGUAGES CXX)
find_package(PDFium CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# WIN32 uses the GUI entry point (WinMain, BCC64X -tW); this is NOT VCL.
add_executable(pdfium-sdl2-viewer WIN32 main.cpp)
set_property(TARGET pdfium-sdl2-viewer PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_compile_features(pdfium-sdl2-viewer PRIVATE cxx_std_23)
target_compile_definitions(pdfium-sdl2-viewer PRIVATE SDL_MAIN_HANDLED WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pdfium-sdl2-viewer PRIVATE PDFium::pdfium SDL2::SDL2 comdlg32)

# BuildEngine's existing generic DLL-staging contract.
if(NOT DEFINED ADECC_RUNTIME_CMAKE_DIR OR ADECC_RUNTIME_CMAKE_DIR STREQUAL "")
   message(FATAL_ERROR "Set ADECC_RUNTIME_CMAKE_DIR to BuildEngine-Admin/admin/cmake/consumer")
endif()
include("${ADECC_RUNTIME_CMAKE_DIR}/BuildEngineRuntime.cmake")
if(NOT DEFINED ADECC_RUNTIME_PACKAGE_ROOTS OR ADECC_RUNTIME_PACKAGE_ROOTS STREQUAL "")
   message(FATAL_ERROR "Set ADECC_RUNTIME_PACKAGE_ROOTS to exact installed package roots")
endif()
# Chromium's former internal Abseil component is retired. Old PDFium
# package/bin outputs may still contain this obsolete DLL; never deploy it.
# BuildEngineStageRuntime also removes stale copies from the app output on
# incremental builds. pdfium.dll's external libabseil_dll.dll import is
# already a mandatory PDFium producer/install gate in build-libraries.xml.
adecc_stage_runtime(pdfium-sdl2-viewer PACKAGE_ROOTS ${ADECC_RUNTIME_PACKAGE_ROOTS}
   TOOLCHAIN_DLLS libwinpthread-1-370.dll
   EXCLUDE_DLLS third_party_abseil-cpp_absl.dll)
adecc_install_application(pdfium-sdl2-viewer DESTINATION bin)
