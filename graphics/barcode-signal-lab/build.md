# Build Barcode Signal Lab

From the BuildEngine-Tests checkout root, in a configured BCC64X command prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S graphics/barcode-signal-lab -B ../build/Test/barcode-signal-lab/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/barcode-signal-lab/Release
```

```bat
cmake --build ../build/Test/barcode-signal-lab/Release/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/barcode-signal-lab/Release/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/barcode-signal-lab/Release/cmake
```

```bat
..\build\Test\barcode-signal-lab\Release\barcode-signal-lab.exe
```

```bat
..\build\Test\barcode-signal-lab\Release\barcode-signal-lab.exe --verify
```

The exact PE runtime closure resolves ZXing-C++, Zint, ImGui and SDL2 plus the transitive, separately installed libpng and zlib package roots. The manifest declares libpng and zlib as transitive dependencies. Do not stage an entire SDK, guess DLL locations or disable PNG support. Reconfigure CMake after changing active package declarations, then rebuild.

Use independent Debug/Release build folders and matching installed native packages. **[not verified]** Windows host compilation and graphical operation remain pending.

After pulling `main`, **reconfigure** with the command above, then build, run CTest and **install**. `cmake --build` alone does not update `../build/Test/barcode-signal-lab/Release/barcode-signal-lab.exe`; it updates the `cmake/runtime/barcode-signal-lab/` executable. Inspect the exact path of the running process when old UI text is still visible. The current UI supports twelve selectable symbologies and uses the external Zint writer backend, with no substitute generator.
