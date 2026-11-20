# Build OpenCV Barcode Vision

## Purpose and build contract

This application is a standalone Windows x64 C++23 showcase compiled with Embarcadero BCC64X (Clang) through the BuildEngine Ninja/CMake toolchain. Its primary purpose is to demonstrate the cooperation of **OpenCV 5.1, ZXing-C++ 3.1.1, Zint 2.16.0, Dear ImGui and SDL2**, not to substitute bundled image processing, barcode readers, OCR engines or unrelated host SDK libraries. The program is built against installed, independently managed BuildEngine packages; active versions are selected exclusively from `admin/build-libraries.xml`. The test is executable without a camera, network connection or separately downloaded test fixture.

**Working directory:** the root of the `BuildEngine-Tests` checkout, normally `D:\local\embarcadero\test_v3\tests`. Use a Windows terminal with the BCC64X environment already initialized. Run each command as **one complete line**; every code block contains exactly one copyable command. Do not merge Release and Debug caches.

## 1. Prepare the installed toolchain

Bootstrap the existing BuildEngine test-toolchain integration. It reads the installed C++Builder tools and configures the native CMake startup cache. It does not download another compiler or install a fallback SDK.

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## 2. Configure Release

Run a fresh CMake configuration for the individual showcase. The initial cache file supplies the BCC64X toolchain, while `BUILDENGINE_CONFIGURATION` selects matching Release import libraries and DLLs from the managed packages. The install prefix is intentionally separate from the compiler's `cmake/runtime` build output.

```bat
cmake -S graphics/opencv-barcode-vision -B ../build/Test/opencv-barcode-vision/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/opencv-barcode-vision/Release
```

If configuration reports a missing package, CMake target, import library or traineddata file, correct that package in BuildEngine and rerun configuration. Do not silence `find_package` failures or link a system-installed substitute.

## 3. Build Release and stage native DLL dependencies

Compile the application with four parallel Ninja jobs. The shared `adecc_test_install(... RUNTIME_CLOSURE ...)` infrastructure also resolves the recursive PE import closure from declared package roots and stages only the verified DLLs required by this executable. An unmanaged DLL is a build failure, not a reason to copy an entire SDK.

```bat
cmake --build ../build/Test/opencv-barcode-vision/Release/cmake --parallel 4
```

After a successful build the **build-tree executable** is located under `../build/Test/opencv-barcode-vision/Release/cmake/runtime/opencv-barcode-vision/`. This executable is not necessarily the same file as the separately installed application.

## 4. Run the headless integration test

The CTest test name is `OpenCVBarcodeVisionNative`. It uses the real native libraries and prints structured `SHOWCASE|CHECK` and `SHOWCASE|RESULT` evidence. A PASS should be accepted only if the process exits successfully and the actual recognition assertions pass.

```bat
ctest --test-dir ../build/Test/opencv-barcode-vision/Release/cmake --output-on-failure
```

The same verification can be invoked without CTest on the staged build-tree executable:

```bat
..\build\Test\opencv-barcode-vision\Release\cmake\runtime\opencv-barcode-vision\opencv-barcode-vision.exe --verify
```

**What is checked:** ZXing-C++ with its external Zint backend generates actual EAN-13 and QR bitmap pixels. The native OpenCV 5.1 barcode and QR detectors then independently decode those pixels. The expected text must match the OpenCV result; successful image generation alone is not sufficient.

## 5. Install the verified executable

Install the application and its staged runtime DLL closure into the configured `Release` application directory. This is a separate step: recompiling in the CMake build directory does **not** automatically replace an earlier installed executable.

```bat
cmake --install ../build/Test/opencv-barcode-vision/Release/cmake
```

Launch the **installed** interactive application:

```bat
..\build\Test\opencv-barcode-vision\Release\opencv-barcode-vision.exe
```

The GUI offers EAN-13 and QR modes, editable payload generation, and PNG/JPEG file import. It reports the actual OpenCV decoded text, a detection overlay and processing time. Imported files are not treated as a synthetic round-trip assertion.

## 6. Optional Debug variant

Debug is a fully separate configuration and must resolve Debug package import libraries, never Release artifacts. The package catalog still selects the same active *versions*; the configuration changes the corresponding binary variants and their runtime dependencies.

```bat
cmake -S graphics/opencv-barcode-vision -B ../build/Test/opencv-barcode-vision/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/opencv-barcode-vision/Debug
```

```bat
cmake --build ../build/Test/opencv-barcode-vision/Debug/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/opencv-barcode-vision/Debug/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/opencv-barcode-vision/Debug/cmake
```

```bat
..\build\Test\opencv-barcode-vision\Debug\opencv-barcode-vision.exe
```

## 7. Diagnosing stale builds or failed recognition

Reconfigure after changing an active library version, a package's CMake export, the showcase's direct dependencies, or the BuildEngine runtime-closure rules. Always check whether you started the `cmake/runtime` executable or the **installed** executable before investigating apparently unchanged behavior. Do not interpret successful compilation as proof that barcode decoding, OCR model initialization or the GUI works.

If the runtime closure rejects a DLL, investigate the original installed managed package root and its declared transitive dependencies. If CTest fails, preserve the exact `SHOWCASE|CHECK` diagnostics and native stack/compiler messages rather than masking detection failures with a different decoder or model.

**Validation status: [not verified].** The CMake commands follow the existing BuildEngine test layout and project configurations, but native BCC64X compilation, CTest execution and interactive behavior must be verified on the Windows host.

## 8. Enumerate and select webcams without opening them

The interactive application offers a dropdown of DirectShow video input devices, including available friendly names. The **REFRESH REGISTERED CAMERAS** action enumerates devices but does not open them, avoiding unnecessary delays on inactive inputs. Use **CONNECT SELECTED** to open the selected DirectShow index through the installed OpenCV `videoio` component. You can also supply a numeric index with **CONNECT BY INDEX**. A disconnected or disabled source may refuse to open; this is reported as a connection error, not silently redirected to a different camera.

For a machine with six registered devices where the first four are currently inactive, the UI is intended to let you select the fifth or sixth device directly, without trying the inactive devices sequentially. The actual indices and ordering must be confirmed on that machine. Windows may exclude fully disabled devices from enumeration, and reconnecting hardware can reorder indices.

Inspect the registered DirectShow inventory using the installed application. This operation only enumerates source names and device paths and does **not** open them:

```bat
..\build\Test\opencv-barcode-vision\Release\opencv-barcode-vision.exe --list-cameras
```

For a quick check before installation, the same diagnostic can be run from the build-tree runtime directory:

```bat
..\build\Test\opencv-barcode-vision\Release\cmake\runtime\opencv-barcode-vision\opencv-barcode-vision.exe --list-cameras
```

In the GUI, select **EAN-13 / UPC barcode** or **QR Code**, connect a camera and enable **Scan live frames**. A live SDL2 image displays frames captured by OpenCV and overlays any detector-supplied barcode boundaries. If a device stops returning frames repeatedly, the stream is released with a visible diagnostic. Use **DISCONNECT** before switching to another physical capture source; selecting a new device or running a file-based test also releases the previous capture.

The existing `--verify` remains camera-independent. It generates known barcode fixtures with managed ZXing/Zint and requires native OpenCV to decode them. The new `--list-cameras` is a diagnostic rather than an automated availability test.

**[not verified]** DirectShow enumeration and live camera behavior must be tested on the BCC64X Windows host.
