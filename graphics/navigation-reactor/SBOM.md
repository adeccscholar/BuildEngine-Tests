# Declared dependency inventory

| BuildEngine package | Role |
| --- | --- |
| Recast Navigation 1.6.0 | Recast slope classification, rasterization, polygon navmesh generation and Detour route queries (separately linked Recast and Detour DLLs) |
| Dear ImGui 1.93.0 | UI core and separately built official SDL2/OpenGL3 backend DLL |
| SDL2 2.32.10 (active) | Window and OpenGL context, platform events via official ImGui backend |
| Mesa OpenGL 26.2.1 | Managed OpenGL/WGL and Gallium runtime, depth-tested terrain rendering |
| GLEW 2.3.1 | Managed OpenGL function resolution used by the terrain and ImGui backend |

The installed package SBOMs remain authoritative for licenses and binaries. **[not verified]** New showcase binary closure has not yet been audited on the Windows host.
