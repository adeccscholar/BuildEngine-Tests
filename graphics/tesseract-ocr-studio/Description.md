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
