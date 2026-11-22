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

## Managed OpenGL / GLEW / official ImGui backend

The project resolves the unique active managed versions of xtensor, xsimd, xtl, SDL2, imgui, OpenGL and GLEW from the BuildEngine XML catalog. It requires the exact configuration-specific CMake exports and import libraries, including the official imgui_sdl2_opengl3 DLL. The managed Mesa opengl32.dll and libgallium_wgl.dll are staged as dynamic runtime modules. No alternative GL, MinGW, MSVC or downloaded SDK fallback is permitted.

### Headless native numerical verification

Execute the CPU assertions (also included in the normal CTest invocation):

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --verify
```

### Optional desktop OpenGL regression

Only when a Windows desktop session and working Mesa OpenGL 3.3 context are available, enable the GPU-specific CTest by reconfiguring the **existing** independent Release build tree:

```bat
cmake -S graphics/tensor-nebula -B ../build/Test/tensor-nebula/Release/cmake -DTENSOR_NEBULA_ENABLE_GL_CTEST=ON
```

Compile the updated test configuration:

```bat
cmake --build ../build/Test/tensor-nebula/Release/cmake --parallel 4
```

Run only the actual OpenGL desktop check:

```bat
ctest --test-dir ../build/Test/tensor-nebula/Release/cmake -R TensorNebulaMesaOpenGL --output-on-failure -V
```

Alternatively, launch the same GPU verification directly:

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --verify-gl
```

### Debug native build, separate from Release

Configure the Debug target without any Release-package fallback:

```bat
cmake -S graphics/tensor-nebula -B ../build/Test/tensor-nebula/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/tensor-nebula/Debug
```

Build Debug:

```bat
cmake --build ../build/Test/tensor-nebula/Debug/cmake --parallel 4
```

Run the standard headless Debug tests:

```bat
ctest --test-dir ../build/Test/tensor-nebula/Debug/cmake --output-on-failure
```

Install Debug with its own DLL closure:

```bat
cmake --install ../build/Test/tensor-nebula/Debug/cmake
```

Launch the Debug observatory:

```bat
..\build\Test\tensor-nebula\Debug\tensor-nebula.exe
```

The real screenshot button captures the current offscreen OpenGL framebuffer as tensor-nebula-capture.bmp. It does not render synthetic image evidence. F1 shows About and F11 toggles fullscreen through the shared OpenGLShowcase adapter.

**[not verified]** No native BCC64X build or OpenGL desktop acceptance has been executed as part of this update.


## Managed Mesa renderer selection (experimental)

The executable loads the managed Mesa WGL library. Gallium backend choices are applied **before** OpenGL context creation; switching in the GUI launches a new instance, rather than trying to mutate a live context. Always check the real GL vendor, renderer and version printed in the UI and GPU verification output. Selecting a GPU name does not prove the hardware was used.

**Use the proven Softpipe reference:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --mesa-backend=softpipe
```

**Attempt the managed D3D12 backend with NVIDIA adapter preference:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --mesa-backend=d3d12 --mesa-adapter=nvidia
```

**Attempt the managed D3D12 backend with Intel adapter preference:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --mesa-backend=d3d12 --mesa-adapter=intel
```

**Run a native OpenGL verification of D3D12 with explicit failure on reported software fallback:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --verify-gl --mesa-backend=d3d12 --mesa-adapter=nvidia
```

**Prepare a Zink probe only after the Vulkan Loader is integrated and a Zink-enabled Mesa WGL DLL has been built:**

```bat
..\build\Test\tensor-nebula\Release\tensor-nebula.exe --verify-gl --mesa-backend=zink
```

The Mesa 26.2.1 XML integration currently attempts `softpipe,d3d12`; Zink remains a separately staged integration blocked by Vulkan Loader/Detours and native build evidence. The application offers it for explicit diagnostics, **not** as a working verified feature.

**[not verified]** D3D12 compilation, Windows native adapter selection, physical GPU execution, and Zink are not yet verified with BCC64X.
