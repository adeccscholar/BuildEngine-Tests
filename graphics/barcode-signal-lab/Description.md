# Barcode Signal Lab — Decode the Invisible

## The experience

A retail barcode becomes a luminous signal in an interactive native scan chamber. Enter twelve digits; the showcase constructs a complete EAN-13 waveform, adds the mathematically correct check digit, renders the black-and-white symbol as real GUI geometry, and asks **ZXing-C++ 3.1.1** to recognize it. The decoded value is shown directly under the generated bars.

## Why this is a real integration

The generator is a small independent implementation of the public EAN-13 L/G/R bit tables, first-digit parity, guard patterns and modulo-10 checksum. **The decoder is not a stub**: `ZXing::ReadBarcodes` receives the generated grayscale pixel buffer through `ZXing::ImageView` and links to the BuildEngine-provided native ZXing DLL.

ZXing is configured as a reader-only package; the application deliberately does not depend on or sneak in a writer library. Change the input and scan again, toggle accuracy mode, or adjust the visual scale. All pixel and decoding work happens offline in the local process.

## Headless evidence

`barcode-signal-lab.exe --verify` generates a deterministic EAN-13 symbol and asserts that ZXing recovers the exact thirteen digits. The CTest named `BarcodeSignalNativeDecode` runs this independent round-trip check without any display server.

This is not an image-fuzzing, camera, or recognition-rate benchmark. **[not verified]** Native build, live desktop appearance and CTest success require testing on Windows with the BuildEngine BCC64X installation.
