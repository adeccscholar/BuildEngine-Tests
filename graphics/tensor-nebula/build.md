# Build Tensor Nebula
In the RAD Studio 13 BCC64X developer shell, from BuildEngine-Tests root:
**Bootstrap and validate the managed toolchain:**

```bat
cmake -P bootstrap/Bootstrap.cmake
```

**Configure Release with Ninja and the BuildEngine initial cache:**

```bat
cmake -S graphics/tensor-nebula -B ../build/Test/tensor-nebula/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/tensor-nebula/Release
```

**Compile the native application:**

```bat
cmake --build ../build/Test/tensor-nebula/Release/cmake --parallel 4
```

**Execute the registered CTest checks:**

```bat
ctest --test-dir ../build/Test/tensor-nebula/Release/cmake --output-on-failure
```

**Install the executable and exact runtime closure:**

```bat
cmake --install ../build/Test/tensor-nebula/Release/cmake
```

**Launch the interactive showcase:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe
```
Use separate Debug paths and matching packages. No alternate compiler or package probing.
**[not verified]** Pending BCC64X native execution.