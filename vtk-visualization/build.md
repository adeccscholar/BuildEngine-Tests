# VTK Visualization: Build and run

Run from the `BuildEngine-Tests` checkout under the BuildEngine root.
Requires the BCC64X RAD Studio 13 development environment and installed
**active** BuildEngine packages:
`vtk@9.7.2`, `onetbb@2023.2.0`, `llvm-openmp@20.1.7`,
`opengl@26.2.1`.

The OpenGL package provides the producer-time import library. The
interactive executable uses the native Windows OpenGL driver (ICD), not
the BuildEngine Mesa softpipe DLL. Its runtime closure intentionally
excludes `opengl32.dll` and does not deploy the Mesa WGL DLL.

## Release

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/vtk-visualization/Release
cmake --build ../build/Test/vtk-visualization/Release/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake --output-on-failure
cmake --install ../build/Test/vtk-visualization/Release/cmake
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --resolution 161 --backend TBB
```

## Debug

```bat
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/vtk-visualization/Debug
cmake --build ../build/Test/vtk-visualization/Debug/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Debug/cmake --output-on-failure
cmake --install ../build/Test/vtk-visualization/Debug/cmake
```

## Focused benchmarks and screenshot

```bat
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -R VTK972Load --output-on-failure -V
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --stress --resolution 257 --iterations 6 --backend OpenMP
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --screenshot vtk-lab.png --resolution 161 --backend TBB
```

A screenshot can also be included in CTest with
`-DVTK_ENABLE_GRAPHICS_CTEST=ON`. Use an interactive desktop,
not a headless build worker or Windows service session.

## Interpretation

Numerical tolerances are 3.5% for volume and area, with exactly one
connected surface and no boundary/nonmanifold edges. All runs emit
`VTK|METRIC|`, `VTK|CHECK|` and `VTK|RESULT|` lines for log analysis.
Performance is reported without brittle machine-specific timing thresholds.
Native BCC64X and GPU rendering remain **not verified** until run on
the target Windows system.
