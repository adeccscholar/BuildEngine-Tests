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

Use independent Debug/Release build folders and matching installed native packages. **[not verified]** Windows host compilation and graphical operation remain pending.
