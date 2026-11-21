# Build HDR Photon Observatory

All commands are standalone **one-line commands** from the BuildEngine-Tests checkout root. Use the managed BCC64X environment and installed BuildEngine packages, not MSVC/MinGW fallbacks.

## Configure Release

```bat
cmake -S graphics/hdr-photon-observatory -B ../build/Test/hdr-photon-observatory/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/hdr-photon-observatory/Release
```

## Compile and stage the verified DLL runtime closure

```bat
cmake --build ../build/Test/hdr-photon-observatory/Release/cmake --parallel 4
```

## Run native CTest

```bat
ctest --test-dir ../build/Test/hdr-photon-observatory/Release/cmake --output-on-failure
```

## Install

```bat
cmake --install ../build/Test/hdr-photon-observatory/Release/cmake
```

## Interactive application

```bat
..\build\Test\hdr-photon-observatory\Release\hdr-photon-observatory.exe
```

## Direct headless verification

```bat
..\build\Test\hdr-photon-observatory\Release\cmake\runtime\hdr-photon-observatory\hdr-photon-observatory.exe --verify
```

For Debug, use a separate Debug build tree, matching `BUILDENGINE_CONFIGURATION=Debug` and managed Debug import libraries. Never substitute a system-installed Ceres, OpenEXR, Abseil or Imath.

The native Ceres optimization and OpenEXR file round trip are actual executable assertions; no simulated PASS messages are accepted. **[not verified]** Native compilation, CTest execution, runtime closure and interactive graphics are pending Windows BCC64X validation.
