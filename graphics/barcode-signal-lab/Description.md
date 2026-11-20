# Barcode Signal Lab — Universal Encode / Decode

## Experience

Choose **QR Code, Data Matrix, Aztec, PDF417, Code 128, Code 39, Code 93, Codabar, ITF, EAN-13, EAN-8 or UPC-A**, enter a payload, press **ENCODE + SCAN**, and watch the resulting barcode appear. A native ZXing-C++ reader independently decodes the generated pixel image and the application reports the actual recovered text and round-trip status. The rendering is based on pixels returned by the writer, not a painted imitation.

## Integration

The installed **ZXing-C++ 3.1.1** DLL uses the upstream new writer backend with **external Zint 2.16.0** instead of a bundled encoder. A call to `ZBarcode_ValidID` additionally exercises the independent Zint DLL. Both shared libraries must be present and version-coherent. The live GUI is implemented with the existing ImGui and SDL2 packages.

`ZXing::CreateBarcodeFromText` creates a selected symbol; `ZXing::WriteBarcodeToImage` produces its bitmap; `ZXing::ReadBarcodes` interprets that image. Generated bitmap pixels are converted into ImGui draw rectangles for a sharp native display. The application is offline and does not fetch a second barcode backend.

The twelve selectable formats represent explicit tested use cases, not a promise that every conceivable barcode symbology accepts every arbitrary payload. For EAN-13, supply valid numeric content and checksum.

## Verification

The `--verify` CTest runs all twelve format round-trips and checks the Zint DLL capability, image dimensions and successful returned payloads. It prints `SHOWCASE|CHECK` and `SHOWCASE|RESULT` evidence. Invalid payloads are reported instead of silently generating another barcode.

**[not verified]** The modified reader/writer DLL, native barcode laboratory compilation, screenshot and full multi-format CTest still require Windows BCC64X execution.

## Writer backend and executable identity

The native implementation uses ZXing's **new writer API**, linked against the separately installed Zint shared library. It does not use an independently generated demonstration symbol or a reader-only fallback. A missing or invalid writer is an explicit error. Changing the symbology loads a matching sample payload; EAN/UPC and ITF require numeric payloads and can impose length/check digit constraints. The CTest checks the 12 example payloads.

If a running application still reports "no write backend" or mentions independently generated barcodes, confirm its executable path: these strings do not occur in this `main.cpp`. Running `cmake --build` updates the build-tree runtime binary, **not** necessarily the separately installed application. Reconfigure, build, test and install, then launch the documented installed executable. This explanation is not proof that an older executable was used.
