# Declared dependency inventory

| BuildEngine managed library | Purpose |
| --- | --- |
| ZXing-C++ 3.1.1 | Native writer, image creation and multi-symbology decoding |
| Zint 2.16.0 | External shared writer backend, direct native API capability check |
| libpng (active) | PNG-enabled Zint runtime dependency, included in exact PE import closure |
| zlib (active) | Compression dependency of the Zint/libpng runtime closure |
| Dear ImGui 1.93.0 | Live controls and actual barcode image visualization |
| SDL2 (active) | Native window and renderer |

The runtime imports must load **the separately installed Zint DLL**, not ZXing's previously bundled encoder sources. These are declarations, not a validated binary SBOM. **[not verified]** Native DLL closure and host execution are pending.
