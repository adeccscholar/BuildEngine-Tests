# Matplot++ Visual Mathematics Gallery

**Test purpose:** Show six mathematically different, high-density scenes in a native interactive SDL2 application. Matplot++ generates 12,000 samples per scene. The six gallery tiles can be selected to open a large detailed rendering with zoom and animated rotation.

The gallery demonstrates Lissajous Interference, Clifford Attractor, De Jong Attractor, Harmonic Bloom, Butterfly Curves, and Orbital Resonance. These are rendered directly in SDL2, not in a browser or an external gnuplot process. The window title shows the selected scene and controls. Click a tile to view it. Click the detail view or press Esc to return. Use the left/right keys to choose the next scene, Space to pause animation, mouse wheel or +/- to zoom, F11 for full screen, and S to export all six SVG graphics.

Run `--verify` to verify the six data sets without initializing graphics. Run `--export` to generate `artifacts/scene-1.svg` through `scene-6.svg` for independent viewing in the central server. The SVG generation is secondary to the native visual application.

[Build commands](build.md) | [License](LICENSE.md) | [SBOM](SBOM.md)

**[not verified]** The native Windows BCC64X build and interactive rendering still require execution on the target machine.
