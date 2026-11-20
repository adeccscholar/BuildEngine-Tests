# OpenCV Barcode Vision

## Experience

Use the **OpenCV 5.1** barcode detector to locate and decode EAN-13 symbols, or select the native OpenCV QR-code detector. The application displays actual decoded content, image overlays, measured processing time and optional exact round-trip verification. Load your own PNG/JPEG image, or generate a deterministic test image using the separately managed **ZXing-C++ 3.1.1** writer with its external **Zint 2.16.0** backend.

## Architecture

- **OpenCV**: `cv::barcode::BarcodeDetector::detectAndDecodeWithType` for EAN/UPC, `cv::QRCodeDetector::detectAndDecode` for QR, `cv::imread` for imported images and `cv::imgproc` for rendering diagnostic boundaries.
- **ZXing + Zint**: real test symbol generation only, never a hidden decoder fallback.
- **SDL2 + ImGui**: native interactive display; OpenCV image pixels and detector output provide every visible result.

The EAN/UPC test is a specific valid **EAN-13** sample; OpenCV does not promise to decode every linear symbology supported by ZXing. The QR detector is an independent OpenCV API.

## Verification

Run `--verify` to generate EAN-13 and QR pixels with ZXing/Zint and require OpenCV itself to decode the original expected content. Reported results use `SHOWCASE|CHECK` and `SHOWCASE|RESULT`. The test does not need camera access or an external sample file.

**[not verified]** Native Windows BCC64X compilation and EAN/QR recognition success are pending.

## Step-by-step image processing pipeline

The sample begins by creating a valid EAN-13 or QR symbol through ZXing-C++ and the separately linked Zint writer. The original generated pixels are transferred to an OpenCV image buffer, with quiet zones preserved and a neutral white canvas around the symbol. This is a genuine cross-library exercise: ZXing is responsible for encoding, while OpenCV is solely responsible for the detection and decoded result. The OpenCV barcode detector and QR detector are independent APIs selected by the UI; unsupported or undecodable symbols remain genuine unsuccessful detection outcomes.

For imported images, the program calls OpenCV `imread` and applies the selected detector directly. The application does not replace the input content with a generated fixture and does not silently switch to ZXing as a fallback decoder. A green quadrilateral is drawn only from actual detector points, and its coordinates come from OpenCV. The original source image, recognized strings, detected symbology and measured decoding time are displayed together, allowing real inspection of the recognition quality.

The `--verify` path uses deterministic data to exercise the complete writer-to-reader dependency chain without needing hardware, files outside the repository or network access. The exact comparison establishes that the result was decoded by OpenCV rather than merely drawn by a generator. This test is an integration assertion, not a guarantee of recognition quality for damaged, rotated or low-resolution real-world barcodes.

For full single-line configuration, CTest, installation and direct execution commands, see [build.md](build.md). Commands are intentionally separated into individually copyable lines.
