# Tesseract OCR Studio

## Experience

A native OCR workbench that renders a synthetic English document with OpenCV or accepts an existing PNG/JPEG. The operator can enable/disable **Otsu thresholding**, select **single-line or automatic page segmentation**, and inspect the original image, processed grayscale pixels, recognized UTF-8 text, confidence and recognition time.

## Architecture

- **OpenCV 5.1** provides synthetic input rendering, grayscale conversion, thresholding and optional external image loading.
- **Leptonica 1.87.0** owns the actual 8-bit `PIX` image supplied to Tesseract.
- **Tesseract 5.5.4** performs native LSTM recognition with the **managed** `share/tessdata/eng.traineddata` model. No model is downloaded by the showcase.
- **SDL2 and Dear ImGui** provide native controls and visual diagnostics.

The model path comes from the exact installed active Tesseract package. A missing model is an explicit error; it is not silently replaced by an SDK or system-wide OCR installation.

## Verification

`--verify` renders `OCR TEST 4711`, preprocesses it using OpenCV, transfers pixels through Leptonica, and requires Tesseract to recognize the exact string and return a valid confidence. A failing OCR result is an explicit failing test, never reported as success.

**[not verified]** Native Windows BCC64X compilation and exact OCR output are pending.

## Processing and recognition stages

The input comes from OpenCV's deterministic text renderer or from a user-selected PNG/JPEG image. OpenCV first normalizes the image to grayscale, then optionally performs Otsu thresholding. These are distinct visual stages and the interface displays the original image separately from the exact processed pixels. This makes it possible to inspect the effects of threshold selection instead of judging recognition quality from the OCR text alone.

The resulting eight-bit pixel values are copied into a Leptonica `PIX` image owned by a scoped RAII wrapper. Tesseract's `TessBaseAPI` is initialized with the explicit `share/tessdata` directory of the **active installed** package and the English LSTM model. The application selects `PSM_SINGLE_LINE` for a known one-line sample or `PSM_AUTO` for general document layout and invokes Tesseract's native `Recognize` and `GetUTF8Text` APIs. The returned text and confidence originate from Tesseract; no expected value is substituted on recognition failure.

The deterministic headless test verifies an exact recognized text instead of merely checking that Tesseract can load the model. Real-world documents may require different page segmentation, layout detection, camera correction and language-specific traineddata. Those capabilities are not disabled by this showcase; its currently installed model provides English recognition only.

For complete Release/Debug configuration, one-line commands, CTest, installation and troubleshooting, see [build.md](build.md).

## Selectable webcam and deliberate frame recognition (20 November 2026)

The application now provides a named **DirectShow camera combobox**, **REFRESH CAMERAS**, **CONNECT SELECTED**, **CONNECT BY INDEX**, **DISCONNECT**, **FREEZE FRAME**, **RESUME LIVE**, and **RECOGNIZE FRAME**. The camera list uses the shared Windows COM DirectShow enumeration component in `graphics/common/CameraDevices.h`. It does not open any cameras merely to discover them. Consequently, a registered but currently unavailable device can remain visible. The manual camera index field is available for devices omitted or reordered by Windows. The actual availability of the user's camera hardware is determined only when opening a selected device.

Live frames are acquired through the installed **OpenCV 5.1 `videoio`** component using `cv::VideoCapture::open(index, cv::CAP_DSHOW)`. A shared `CameraFrameView` wraps an SDL2 streaming texture to display a real camera image through the existing ImGui/SDL2 renderer. This avoids rendering each video pixel as individual ImGui geometry. The current camera selection, capture status, acquired-frame count and driver failures remain visible.

**OCR is an explicit operation, not a continuous per-frame task.** Pressing **FREEZE FRAME** creates an immutable copy of the latest camera image and pauses capture. **RECOGNIZE FRAME** sends either that frozen image or the current live frame into the existing OpenCV grayscale/Otsu → Leptonica PIX → Tesseract English LSTM pipeline. The recognized text, confidence and processing duration remain available while the camera continues to display new frames. Pressing **RESUME LIVE** resumes capture without discarding the last successful OCR result. Changing the camera releases the previous device; the synthetic-document and imported-image workflows remain independent and available.

For diagnostic purposes, `--list-cameras` prints the registered DirectShow device indices, friendly names and paths without opening any capture device. The original `--verify` is still **camera-independent** and asserts actual recognized text, confidence and measurable spacing in the synthetic source image. The webcam feature does not reduce that verification coverage.

**[not verified]** The live DirectShow source, GUI rendering and Windows device order have not yet been executed on the BCC64X host.
