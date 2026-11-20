# Native graphics demonstration gallery

This directory groups nine independent **interactive Windows C++23/BCC64X applications**, each with readable controls, animated scenarios, a headless correctness test and optional independent presentation artifacts.

| Showcase | Native user experience | Managed libraries |
| --- | --- | --- |
| [Dear ImGui Neon Laboratory](imgui-neon-lab/Description.md) | Four time-dependent particle fields, scene selection, zoom and playback controls | ImGui, SDL2 |
| [ImPlot Real-Time Signal Theater](implot-signal-theater/Description.md) | Four oscilloscope models, animated ImPlot curves, labeled axes, signal toggles and parameter sliders | ImPlot, ImGui, SDL2 |
| [Matplot++ Mathematical Discovery Gallery](matplot-chaos-atlas/Description.md) | Six tiled mathematical scenes, detailed progressive animation, zoom and optional six-file SVG export | Matplot++, ImGui, SDL2 |
| [Vector Galaxy](vector-galaxy/Description.md) | 851-star mathematical nebula with real HNSW similarity rings and libigl vertex-normal lighting | hnswlib, libigl, Eigen, ImGui, SDL2 |
| [Barcode Signal Lab](barcode-signal-lab/Description.md) | Universal barcode laboratory with five writer formats, real external Zint encoding and native ZXing round-trip decoding | ZXing-C++, Zint, ImGui, SDL2 |
| [OpenCV Barcode Vision](opencv-barcode-vision/Description.md) | Native OpenCV EAN-13 and QR detection, selectable DirectShow webcams, live SDL2 video, file import and reproducible ZXing/Zint symbols | OpenCV, ZXing-C++, Zint, ImGui, SDL2 |
| [Tesseract OCR Studio](tesseract-ocr-studio/Description.md) | OpenCV thresholding, Leptonica PIX, real English Tesseract OCR confidence and original/processed image comparison | Tesseract, Leptonica, OpenCV, ImGui, SDL2 |
| [Navigation Reactor](navigation-reactor/Description.md) | Real Recast triangle classification and voxelized heightfields; interactive slope and relief controls | Recast Navigation, ImGui, SDL2 |
| [Pulse Engine](pulse-engine/Description.md) | Glowing PCM oscilloscope, four true miniaudio oscillators, optional audible sound synthesis | miniaudio, ImGui, SDL2 |

Each application builds independently with CMake/Ninja, uses installed BuildEngine DLLs, and provides one-line build commands in its own `build.md`. The graphic is part of the native application, **not** a browser rendering of pre-generated SVGs. The central server can additionally display independently generated SVGs when available.

**[not verified]** The revised interactive GUI code requires native Windows BCC64X compilation and a visual desktop verification. Existing library smoke tests remain unchanged.
