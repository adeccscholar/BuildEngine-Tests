# Build — Direct Skia showcase

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S skia-direct-showcase -B ../build/Test/skia-direct-showcase/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/skia-direct-showcase/Release
```

```bat
cmake --build ../build/Test/skia-direct-showcase/Release/cmake
```

```bat
cmake --install ../build/Test/skia-direct-showcase/Release/cmake
```

```bat
ctest --test-dir ../build/Test/skia-direct-showcase/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Applications and operator instructions

**SkiaDirectShowcase:** Fullscreen Skia rendering showcase with seven scenes and adjustable stress benchmark.

```bat
..\build\Test\skia-direct-showcase\Release\SkiaDirectShowcase.exe
```

### Controls

Left/Right or 1–7 chooses Images, Primitives, Paths, Gradients, Transform/Clip, BlendModes or Stress/FPS. F toggles fit-to-screen vs native design pixels; Space pauses/resumes animation; Up/Down adjusts stress load from 2,000 to 40,000 drawing calls in scene 7; R resets; Esc quits. FPS and CPU render time appear in the stress overlay.
