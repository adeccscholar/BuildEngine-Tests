# Declared software bill of materials

The [CycloneDX 1.6 declaration](SBOM-implot-signal-theater.cdx.json) records the native GUI application's declared managed library dependencies: **imgui, implot, sdl2**. SDL2 is the rendering/window layer, ImGui provides textured native text and UI widgets, and the package under test performs its native mathematical/plotting operation.

This is a declared source dependency inventory, not a verified binary SBOM. The exact SDL2 version selected via `adecc_package_current_root`, recursively imported DLLs and hashes must be verified on the Windows BCC64X host.
