# Barcode Signal Lab — Universal Encode / Decode

## Experience

Choose **QR Code, Data Matrix, Code 128, EAN-13 or PDF417**, enter a payload, press **ENCODE + SCAN**, and watch the resulting barcode appear. A native ZXing-C++ reader independently decodes the generated pixel image and the application reports the actual recovered text and round-trip status. The rendering is based on pixels returned by the writer, not a painted imitation.

## Integration

The installed **ZXing-C++ 3.1.1** DLL uses the upstream new writer backend with **external Zint 2.16.0** instead of a bundled encoder. A call to `ZBarcode_ValidID` additionally exercises the independent Zint DLL. Both shared libraries must be present and version-coherent. The live GUI is implemented with the existing ImGui and SDL2 packages.

`ZXing::CreateBarcodeFromText` creates a selected symbol; `ZXing::WriteBarcodeToImage` produces its bitmap; `ZXing::ReadBarcodes` interprets that image. Generated bitmap pixels are converted into ImGui draw rectangles for a sharp native display. The application is offline and does not fetch a second barcode backend.

The five selectable formats represent explicit tested use cases, not a promise that every conceivable barcode symbology accepts every arbitrary payload. For EAN-13, supply valid numeric content and checksum.

## Verification

The `--verify` CTest runs all five format round-trips and checks the Zint DLL capability, image dimensions and successful returned payloads. It prints `SHOWCASE|CHECK` and `SHOWCASE|RESULT` evidence. Invalid payloads are reported instead of silently generating another barcode.

**[not verified]** The modified reader/writer DLL, native barcode laboratory compilation, screenshot and full multi-format CTest still require Windows BCC64X execution.
