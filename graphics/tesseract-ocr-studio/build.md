# Build Tesseract OCR Studio

## Purpose and build contract

This application is a standalone Windows x64 C++23 showcase compiled with Embarcadero BCC64X (Clang) through the BuildEngine Ninja/CMake toolchain. Its primary purpose is to demonstrate the cooperation of **Tesseract 5.5.4, Leptonica 1.87.0, OpenCV 5.1, Dear ImGui and SDL2**, not to substitute bundled image processing, barcode readers, OCR engines or unrelated host SDK libraries. The program is built against installed, independently managed BuildEngine packages; active versions are selected exclusively from `admin/build-libraries.xml`. The test is executable without a camera, network connection or separately downloaded test fixture.

**Working directory:** the root of the `BuildEngine-Tests` checkout, normally `D:\local\embarcadero\test_v3\tests`. Use a Windows terminal with the BCC64X environment already initialized. Run each command as **one complete line**; every code block contains exactly one copyable command. Do not merge Release and Debug caches.

## 1. Prepare the installed toolchain

Bootstrap the existing BuildEngine test-toolchain integration. It reads the installed C++Builder tools and configures the native CMake startup cache. It does not download another compiler or install a fallback SDK.

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## 2. Configure Release

Run a fresh CMake configuration for the individual showcase. The initial cache file supplies the BCC64X toolchain, while `BUILDENGINE_CONFIGURATION` selects matching Release import libraries and DLLs from the managed packages. The install prefix is intentionally separate from the compiler's `cmake/runtime` build output.

```bat
cmake -S graphics/tesseract-ocr-studio -B ../build/Test/tesseract-ocr-studio/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/tesseract-ocr-studio/Release
```

If configuration reports a missing package, CMake target, import library or traineddata file, correct that package in BuildEngine and rerun configuration. Do not silence `find_package` failures or link a system-installed substitute.

## 3. Build Release and stage native DLL dependencies

Compile the application with four parallel Ninja jobs. The shared `adecc_test_install(... RUNTIME_CLOSURE ...)` infrastructure also resolves the recursive PE import closure from declared package roots and stages only the verified DLLs required by this executable. An unmanaged DLL is a build failure, not a reason to copy an entire SDK.

```bat
cmake --build ../build/Test/tesseract-ocr-studio/Release/cmake --parallel 4
```

After a successful build the **build-tree executable** is located under `../build/Test/tesseract-ocr-studio/Release/cmake/runtime/tesseract-ocr-studio/`. This executable is not necessarily the same file as the separately installed application.

## 4. Run the headless integration test

The CTest test name is `TesseractOCRStudioNative`. It uses the real native libraries and prints structured `SHOWCASE|CHECK` and `SHOWCASE|RESULT` evidence. A PASS should be accepted only if the process exits successfully and the actual recognition assertions pass.

```bat
ctest --test-dir ../build/Test/tesseract-ocr-studio/Release/cmake --output-on-failure
```

The same verification can be invoked without CTest on the staged build-tree executable:

```bat
..\build\Test\tesseract-ocr-studio\Release\cmake\runtime\tesseract-ocr-studio\tesseract-ocr-studio.exe --verify
```

**What is checked:** OpenCV generates and optionally binarizes a deterministic English text image. Leptonica provides the real PIX passed into the installed Tesseract LSTM engine, using the managed English traineddata. The extracted UTF-8 string must equal the expected sample and the confidence must be within the valid range.

## 5. Install the verified executable

Install the application and its staged runtime DLL closure into the configured `Release` application directory. This is a separate step: recompiling in the CMake build directory does **not** automatically replace an earlier installed executable.

```bat
cmake --install ../build/Test/tesseract-ocr-studio/Release/cmake
```

Launch the **installed** interactive application:

```bat
..\build\Test\tesseract-ocr-studio\Release\tesseract-ocr-studio.exe
```

The GUI offers synthetic document generation, PNG/JPEG file import, Otsu thresholding and single-line versus automatic-page-segmentation modes. It shows both source and preprocessed images, the recognized UTF-8 output, confidence and elapsed time. The `eng.traineddata` file is loaded from the active installed Tesseract package, not from a generic system directory.

## 6. Optional Debug variant

Debug is a fully separate configuration and must resolve Debug package import libraries, never Release artifacts. The package catalog still selects the same active *versions*; the configuration changes the corresponding binary variants and their runtime dependencies.

```bat
cmake -S graphics/tesseract-ocr-studio -B ../build/Test/tesseract-ocr-studio/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/tesseract-ocr-studio/Debug
```

```bat
cmake --build ../build/Test/tesseract-ocr-studio/Debug/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/tesseract-ocr-studio/Debug/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/tesseract-ocr-studio/Debug/cmake
```

```bat
..\build\Test\tesseract-ocr-studio\Debug\tesseract-ocr-studio.exe
```

## 7. Diagnosing stale builds or failed recognition

Reconfigure after changing an active library version, a package's CMake export, the showcase's direct dependencies, or the BuildEngine runtime-closure rules. Always check whether you started the `cmake/runtime` executable or the **installed** executable before investigating apparently unchanged behavior. Do not interpret successful compilation as proof that barcode decoding, OCR model initialization or the GUI works.

If the runtime closure rejects a DLL, investigate the original installed managed package root and its declared transitive dependencies. If CTest fails, preserve the exact `SHOWCASE|CHECK` diagnostics and native stack/compiler messages rather than masking detection failures with a different decoder or model.

**Validation status: [not verified].** The CMake commands follow the existing BuildEngine test layout and project configurations, but native BCC64X compilation, CTest execution and interactive behavior must be verified on the Windows host.
