# Dear ImGui Neon Constellation

**Test purpose:** Present a responsive, readable native control room with four animated scientific particle scenes. Real ImGui 1.93 DrawLists render 700 time-dependent particles in Nebula Vortex, Quantum Orbit, Wave Interference and Stellar Bloom. The native UI uses SDL2 geometry rendering with the font-atlas texture, fixing the earlier SVG text loss.

Features include a scene selector, play/pause, restart and a zoom slider. The high-DPI-capable SDL2 window is resizable. Scene graphics animate through deterministic parametric equations and their geometry changes every frame. `--verify` remains a headless DrawList validation, independent of SDL2 video initialization.

[Build](build.md) · [License](LICENSE.md) · [SBOM](SBOM.md)

**Verification:** Native BCC64X build and interactive operation confirmed by the operator on 2026-11-18.
