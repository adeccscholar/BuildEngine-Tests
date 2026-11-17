# Software Bill of Materials

The following CycloneDX 1.6 declarations cover the **declared** native library dependencies of each application, not a verified binary SBOM.

- [Interactive GUI declaration](SBOM-neon-physics-arena.cdx.json)
- [Headless test declaration](SBOM-neon-physics-verify.cdx.json)

Both targets depend on BuildEngine's active SDL3 3.4.20 and Box2D 3.2.0 DLL packages and the C++Builder 13 dynamic RTL. The CMake helper stages only the recursive PE import closure from the exact managed package directories plus declared compiler runtime DLLs.

**[not verified]** Final runtime versions, DLL checksums and transitive binary dependencies require real Windows BCC64X installation and execution evidence. No generated CycloneDX record is presented as verified.
