# Build Navigation Reactor

Use the configured BCC64X toolchain, from the BuildEngine-Tests checkout root.

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S graphics/navigation-reactor -B ../build/Test/navigation-reactor/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/navigation-reactor/Release
```

```bat
cmake --build ../build/Test/navigation-reactor/Release/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/navigation-reactor/Release/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/navigation-reactor/Release/cmake
```

```bat
..\build\Test\navigation-reactor\Release\navigation-reactor.exe
```

```bat
..\build\Test\navigation-reactor\Release\navigation-reactor.exe --verify
```

Debug and Release must use separate CMake caches and exact matching managed import DLL variants. **[not verified]** Native Windows build and visual operation are pending.

## 3D view validation

The current main branch defaults to the perspective 3D terrain viewport, with mouse-drag orbit, mouse-wheel zoom, azimuth/elevation/distance sliders and a wireframe toggle. The '3D terrain' checkbox returns to the prior voxel raster. After updating the source, reconfigure and rebuild before installing the new executable. The headless test does not validate the visual output; inspect the terrain elevation, route projection and interaction on Windows.

## Managed official ImGui GPU backend

Prerequisites are the **installed, active, matching** Recast Navigation 1.6.0, Mesa OpenGL 26.2.1, GLEW 2.3.1, SDL2 2.32.10, and ImGui 1.93.0 packages. ImGui must first be rebuilt and reinstalled with the producer extension `imgui_sdl2_opengl3` (both configurations) from BuildEngine-Admin/main; the Navigation Reactor CMake contract requires the official backend import library and refuses to use a handwritten fallback. Run the existing configure command, then build, CTest and install. CTest now includes a display-dependent GPU test. The test executable also accepts `--verify-gl` directly. The Mesa runtime `opengl32.dll` and its dynamically loaded Gallium `libgallium_wgl.dll` are declared through the shared `DYNAMIC_DLLS` runtime-closure contract. The closure removes old local staging copies before PE scanning, resolves these DLL names exclusively from declared active BuildEngine package roots, validates uniqueness and recursive imports, and records them in the same runtime manifest. No per-showcase `POST_BUILD` DLL copying remains.

## Verify actual Detour polygons and corridor overlays

After rebuilding the managed packages, run `navigation-reactor.exe --verify` to verify native detail mesh triangles, corridor membership and densely sampled surface navigation. Run `navigation-reactor.exe --verify-gl` to validate Mesa OpenGL framebuffer rendering with those native triangles and the path. In the interactive viewport, toggle **NavMesh polygons** (teal) and **Detour corridor** (orange) independently. These overlays are direct Detour geometry, not a painted terrain approximation. **[not verified]** until the executable completes on the BCC64X host.

## Runtime closure correction (20 November 2026)

The previous `buildengine-stage-navigation-reactor` target copied the Mesa WGL DLL pair in a separate `POST_BUILD` command after PE scanning. A subsequent incremental build saw the copied `libgallium_wgl.dll` in the target directory and correctly refused it as an unmanaged PE import. The shared `adecc_test_install(... RUNTIME_CLOSURE DYNAMIC_DLLS ...)` contract now controls the Mesa DLL pair inside the same integrity-checked PE closure. `DYNAMIC_DLLS` is a list of explicit managed DLL basenames, never an instruction to scan or copy an arbitrary SDK directory. CMake regeneration is required to remove the previous build-rule invocation. **[not verified]** Native BCC64X staging and runtime tests must be executed on the Windows host.
