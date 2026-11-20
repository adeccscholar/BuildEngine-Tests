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

## 8. Camera selection, live image and frozen OCR capture

The webcam mode reuses the OpenCV 5.1 `videoio` component, native Windows DirectShow device enumeration, shared SDL2 streaming-image renderer and the existing Leptonica/Tesseract image pipeline. The application never creates a second OCR engine or modifies upstream packages. The names reported by the Windows DirectShow video input category appear in a dropdown, without opening each camera in sequence. Busy or disconnected devices may appear in the list, while Windows-disabled devices can be absent. Manual DirectShow indices are also supported for diagnostics.

Use **REFRESH CAMERAS** to rebuild the registered device list. Choose the desired camera (including the fifth or sixth registered device when the first four are unavailable), then press **CONNECT SELECTED**. Alternatively enter a specific index and press **CONNECT BY INDEX**. The camera remains open until you explicitly disconnect, change selection or the driver stops returning valid frames. The preview uses actual captured pixels. **FREEZE FRAME** stops reading further frames and preserves a copy of the most recent image. **RECOGNIZE FRAME** applies the currently configured Otsu and Tesseract page-segmentation settings to that copy, while keeping the recognized UTF-8 text and confidence visible. **RESUME LIVE** returns to capture without automatically repeating OCR.

A list of registered cameras is available from the build-tree executable without requiring any active hardware input. This command emits `SHOWCASE|CAMERAS` and one `SHOWCASE|CAMERA` record per registered source.

```bat
..\build\Test\tesseract-ocr-studio\Release\cmake\runtime\tesseract-ocr-studio\tesseract-ocr-studio.exe --list-cameras
```

The installed application provides the same diagnostic:

```bat
..\build\Test\tesseract-ocr-studio\Release\tesseract-ocr-studio.exe --list-cameras
```

The webcam option requires the new `videoio` package component and the native DirectShow system import libraries `ole32`, `oleaut32` and `strmiids`. These are resolved explicitly in the showcase CMake contract. The original `--verify` test remains camera-independent and must pass before accepting the OCR build.

After updating the checkout, regenerate the CMake build tree to apply the new OpenCV component and Windows libraries:

```bat
cmake -S graphics/tesseract-ocr-studio -B ../build/Test/tesseract-ocr-studio/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/tesseract-ocr-studio/Release
```

Compile using the existing BCC64X Ninja toolchain:

```bat
cmake --build ../build/Test/tesseract-ocr-studio/Release/cmake --parallel 4
```

Run the camera-independent integration regression:

```bat
ctest --test-dir ../build/Test/tesseract-ocr-studio/Release/cmake --output-on-failure
```

Install the same tested build:

```bat
cmake --install ../build/Test/tesseract-ocr-studio/Release/cmake
```

Launch the interactive application:

```bat
..\build\Test\tesseract-ocr-studio\Release\tesseract-ocr-studio.exe
```

**[not verified]** BCC64X compilation and the live camera controls require execution on the actual Windows machine. The earlier OCR fixture-word-spacing correction also still requires CTest confirmation.

## 9. Recognize a complete camera document or multiple paragraphs

The former `PSM_SINGLE_LINE` default is unsuitable for a page of German prose, and a 640×480 capture may not contain enough pixels for small printed letters. The OCR Studio now defaults to `PSM_AUTO` and can instead use `PSM_SINGLE_BLOCK` for a known paragraph region or `PSM_SINGLE_LINE` for a line. The previous exact synthetic text regression remains present and is not weakened.

For camera capture, first choose a DirectShow source, then select an appropriate **Requested camera resolution**. Full HD 1920×1080 is the initial request. UHD 3840×2160 is available when the camera and backend support it. Inspect **Actual captured pixels** to confirm the negotiated format. Frame interpolation is not a substitute for an optically legible source. Place the book squarely under the camera, fill most of the field of view with the printed area, ensure focus and non-glare illumination, then use **FREEZE FRAME** before running OCR.

Select **Detect document quadrilateral** when a page has clear outer boundaries. The actual OpenCV 5.1 contour and perspective functions compute the rectangle; no mock bounding box is substituted. For a book back-cover with multiple nested boxes or only one small printed block, select **Manual text rectangle** and adjust the four sliders while observing the yellow crop border in the live preview. Choose **Single text block** for that selected paragraph, or **Automatic page** for several paragraphs. If the automatic contour cannot be found, the application transparently uses the whole frame. Compare the extracted document and the Otsu processing image in the new aspect-preserving SDL previews; both are the same pixels supplied to the OCR engine, not resized thumbnails used for recognition. The UTF-8 output is scrollable and can be copied with **COPY RECOGNIZED TEXT**.

For German printed pages, select **German (deu)** or **German + English**. The first active Tesseract package only installed `eng.traineddata`; the **BuildEngine-Admin** Tesseract 5.5.4 package has now been extended to provision `deu.traineddata` from the pinned `tessdata_fast` project revision. Rebuild and reinstall that managed package before selecting German. The UI reports a missing file explicitly and does not silently substitute English. This model provisioning change has not yet been validated through the BuildEngine FSM, and the German source lacks a separately confirmed SHA-256 digest (it is pinned to the repository revision). Upstream model integrity should be finalized with a recorded source digest before considering the packaging fully verified.

After updating both repositories and installing the model, regenerate the showcase configuration:

```bat
cmake -S graphics/tesseract-ocr-studio -B ../build/Test/tesseract-ocr-studio/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/tesseract-ocr-studio/Release
```

Compile the OCR showcase against the active managed libraries:

```bat
cmake --build ../build/Test/tesseract-ocr-studio/Release/cmake --parallel 4
```

Run the headless verification, which now checks exact text, visible fixture spacing and genuine page geometry:

```bat
ctest --test-dir ../build/Test/tesseract-ocr-studio/Release/cmake --output-on-failure
```

Install the built application:

```bat
cmake --install ../build/Test/tesseract-ocr-studio/Release/cmake
```

Launch the interactive studio:

```bat
..\build\Test\tesseract-ocr-studio\Release\tesseract-ocr-studio.exe
```

**[not verified]** The installed native German language model and camera-page OCR behavior require BCC64X Windows execution. A blank OCR result must remain a failing user-visible recognition outcome, not be rewritten to an expected phrase.
