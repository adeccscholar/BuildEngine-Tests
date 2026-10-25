# Build OpenXlsxGrid (RAD Studio 13 Developer Command Prompt)

From `BuildEngine/tests`, use the productive BuildEngine root, existing installed libraries, and checked-out `adecc-core-lib` submodule:

```bat
git submodule update --init --recursive
cmake -P bootstrap/Bootstrap.cmake
cmake -S OpenXlsxGrid -B ../build/Test/OpenXlsxGrid/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/OpenXlsxGrid/Release
cmake --build ../build/Test/OpenXlsxGrid/Release/cmake
cmake --install ../build/Test/OpenXlsxGrid/Release/cmake
```

For Debug substitute `Debug` consistently. No local third-party bootstrap, no static archive or package-version guessing.

```bat
..\build\Test\OpenXlsxGrid\Release\adecc_openxlsx_grid_test.exe
```

**Server dependency:** None.
Results, license file and CycloneDX per-target records are accessible from the BuildEngine-Tests Markdown documentation server at `http://127.0.0.1:8781/docs/`.
