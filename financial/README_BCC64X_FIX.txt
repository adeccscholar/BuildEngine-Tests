Copyright (c) 2026 adecc Systemhaus GmbH
SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
Project: adecc Scholar

BCC64X / CMake compatibility layer - 2026-08-10-r4
====================================================

Verified problem
----------------
Stock Kitware CMake 4.0.1 recognizes Embarcadero 7.80/BCC64X but inherited
legacy Embarcadero/Borland rules during compiler checks. The observed failures
were:

  -Od
  -e<TARGET>
  -lS:1048576
  -lSc:4098
  -lH:1048576
  -lHc:8192
  import32.lib

The working solution is NOT to modify the global CMake installation and NOT to
modify rsvars.bat. The package uses:

  cmake\toolchains\bcc64x.cmake
  cmake\overrides\bcc64x-rules.cmake

The toolchain sets CMAKE_USER_MAKE_RULES_OVERRIDE. This makes the corrected
rules apply to CMake try_compile() as well as normal targets.

Verified result
---------------
The user's real machine successfully completed the direct BCC64X smoke test and
CMake/Ninja C and C++ smoke targets with BCC64X 7.80, CMake 4.0.1 and Ninja
1.11.1.

GLEW follow-up
--------------
The GLEW Git checkout did not contain generated src\glew.c. The package now
uses the official GLEW 2.3.1 source release ZIP. BCC64X also required
<stddef.h> before GLEW's ptrdiff_t typedefs, so the adapter injects it with a
target-local -include stddef.h option instead of modifying upstream files.

See docs\BUILDCHAIN.md and docs\PROJECT_ADAPTATIONS.md for details.


r9 RC integration:
- CMake uses the absolute Windows SDK x64 rc.exe path.
- No PATH modification.
- CGRC is diagnostic only; BRCC32 remains available for explicit RAD Studio/VCL resource steps.

R11: Windows SDK rc.exe receives matching SDK include directories explicitly via /I; PATH and INCLUDE remain unchanged.
