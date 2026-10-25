# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
# Called before project(), including on automatic Ninja/CMake reconfiguration.
# Rehydrate the BuildEngine BCC64X environment without guessing tool versions.
if(NOT DEFINED ENV{CB_BDS} OR "$ENV{CB_BDS}" STREQUAL "")
   if(DEFINED ENV{BDS} AND NOT "$ENV{BDS}" STREQUAL "")
      set(ENV{CB_BDS} "$ENV{BDS}")
   elseif(DEFINED CMAKE_CXX_COMPILER AND
          CMAKE_CXX_COMPILER MATCHES "[/\\\\]bin64[/\\\\]bcc64x[.]exe$")
      get_filename_component(_adecc_bin64 "${CMAKE_CXX_COMPILER}" DIRECTORY)
      get_filename_component(_adecc_bds "${_adecc_bin64}" DIRECTORY)
      set(ENV{CB_BDS} "${_adecc_bds}")
   endif()
endif()
if(NOT DEFINED ENV{CB_BDS} OR "$ENV{CB_BDS}" STREQUAL "")
   message(FATAL_ERROR "BCC64X environment unavailable: set BDS/CB_BDS or configure with bootstrap/BuildEngineInit.cmake")
endif()
if(NOT DEFINED ENV{CB_BCC64X} OR "$ENV{CB_BCC64X}" STREQUAL "")
   set(ENV{CB_BCC64X} "$ENV{CB_BDS}/bin64/bcc64x.exe")
endif()
if(NOT EXISTS "$ENV{CB_BCC64X}")
   message(FATAL_ERROR "BCC64X compiler does not exist: $ENV{CB_BCC64X}")
endif()
unset(_adecc_bin64)
unset(_adecc_bds)
