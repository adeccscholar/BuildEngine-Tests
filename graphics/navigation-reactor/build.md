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

Prerequisites are the **installed, active, matching** Recast Navigation 1.6.0, Mesa OpenGL 26.2.1, GLEW 2.3.1, SDL2 2.32.10, and ImGui 1.93.0 packages. ImGui must first be rebuilt and reinstalled with the producer extension `imgui_sdl2_opengl3` (both configurations) from BuildEngine-Admin/main; the Navigation Reactor CMake contract requires the official backend import library and refuses to use a handwritten fallback. Run the existing configure command, then build, CTest and install. CTest now includes a display-dependent GPU test. The test executable also accepts `--verify-gl` directly. The Mesa runtime `opengl32.dll` and its dynamically loaded Gallium `libgallium_wgl.dll` are staged from the exact managed Mesa package, not copied from an SDK.

## Verify actual Detour polygons and corridor overlays

After rebuilding the managed packages, run `navigation-reactor.exe --verify` to verify native detail mesh triangles, corridor membership and densely sampled surface navigation. Run `navigation-reactor.exe --verify-gl` to validate Mesa OpenGL framebuffer rendering with those native triangles and the path. In the interactive viewport, toggle **NavMesh polygons** (teal) and **Detour corridor** (orange) independently. These overlays are direct Detour geometry, not a painted terrain approximation. **[not verified]** until the executable completes on the BCC64X host.
