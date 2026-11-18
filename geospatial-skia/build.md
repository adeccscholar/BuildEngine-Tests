# Build — Geospatial and Skia

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S geospatial-skia -B ../build/Test/geospatial-skia/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/geospatial-skia/Release
```

```bat
cmake --build ../build/Test/geospatial-skia/Release/cmake
```

```bat
cmake --install ../build/Test/geospatial-skia/Release/cmake
```

```bat
ctest --test-dir ../build/Test/geospatial-skia/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Launch individual applications

**geospatial_benchmark:** Geospatial calculation and raster benchmark.

```bat
..\build\Test\geospatial-skia\Release\geospatial_benchmark.exe
```

**geospatial_viewer:** Interactive native Win32/Skia 2D address-point, density and relief view.

```bat
..\build\Test\geospatial-skia\Release\geospatial_viewer.exe
```

**geospatial_terrain:** Interactive SDL2/OpenGL synthetic address-density terrain.

```bat
..\build\Test\geospatial-skia\Release\geospatial_terrain.exe
```

### Controls

2D Skia viewer: Ctrl+O opens flat address data, Ctrl+D opens street data/hierarchy, 1/2/3 selects point/density/relief view; S/T/P selects serial/oneTBB/OpenMP; +/- changes cell size; wheel zooms around pointer, left-drag pans, 0 resets extent. 3D terrain viewer: F opens file, D opens directory, L reloads, drag-and-drop changes source, left-drag rotates, wheel zooms, 1/2/3 selects top/side/oblique view and Esc exits. Synthetic elevations describe address density, not physical terrain.
