# Software Bill of Materials

This is a **declared dependency register**, not a verified
binary SBOM. The complete runtime PE import closure must be established
after a successful BCC64X build.

- VTK 9.7.2: VTK modules in `CMakeLists.txt`, BSD licenses as supplied upstream
- oneTBB 2023.2.0: native SMP backend
- LLVM OpenMP 20.1.7: native SMP backend
- Mesa OpenGL 26.2.1: managed link-time import library only
- Windows system OpenGL32: actual runtime ICD dispatch, not bundled Mesa
- RAD Studio 13 / BCC64X: Windows native C++23 compiler/runtime
