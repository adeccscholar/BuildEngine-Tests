# Build and verify PolicyTest

From the `BuildEngine/tests` checkout in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S PolicyTest -B ../build/Test/PolicyTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/PolicyTest/Release
cmake --build ../build/Test/PolicyTest/Release/cmake
ctest --test-dir ../build/Test/PolicyTest/Release/cmake --output-on-failure
cmake --install ../build/Test/PolicyTest/Release/cmake
```

For Debug consistently use `Debug` instead of `Release`; no static adecc archive, local third-party fetch or system-wide search paths. The CMake project links the existing `adecc::domain` header-only INTERFACE target. The only staged DLLs should be the **actual PE import closure** and the declared C++Builder runtime components. Output is a normal console application, `adecc_policy_test.exe`. No database or network service is required.

The project is integrated into the BuildEngine-Tests Markdown server through its `Description.md`, `build.md`, `LICENSE.md` and CycloneDX JSON file.