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
