# Build and run — Matplot++ Chaos Atlas

Run from the **BuildEngine-Tests checkout root** in the RAD Studio 13 developer command prompt. Do not reuse another test's binary directory.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S graphics/matplot-chaos-atlas -B ../build/Test/matplot-chaos-atlas/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/matplot-chaos-atlas/Release
cmake --build ../build/Test/matplot-chaos-atlas/Release/cmake --parallel 4
ctest --test-dir ../build/Test/matplot-chaos-atlas/Release/cmake --output-on-failure
cmake --install ../build/Test/matplot-chaos-atlas/Release/cmake
cd ..\build\Test\matplot-chaos-atlas\Release
matplot-chaos-atlas.exe
start matplot-chaos-atlas.svg
```

Press F11 in the browser for the full-screen show. Use independent Debug paths and `-DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug` for Debug. The build selects exact managed package roots, not arbitrary DLLs on PATH. `--verify` is the headless correctness gate. **[not verified]** The resulting native executable and graphic output require host execution.
