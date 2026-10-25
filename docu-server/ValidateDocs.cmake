if(NOT DEFINED ROOT)
 message(FATAL_ERROR "ROOT required")
endif()
foreach(_name IN ITEMS CompareTest financial geospatial-skia opengl-sdl2-earth
 openmp-galaxy openmp-mandelbrot pdf-image-extraction pdfium/Common
 pdfium/SDL2 pdfium/VCL skia-direct-showcase tests/pdf-image-extraction
 thirdparty-showcase)
 foreach(_file IN ITEMS Description.md build.md LICENSE.md SBOM.md)
  if(NOT EXISTS "${ROOT}/${_name}/${_file}")
   message(FATAL_ERROR "Missing test documentation: ${_name}/${_file}")
  endif()
 endforeach()
endforeach()
