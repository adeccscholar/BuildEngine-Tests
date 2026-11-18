# Matplot++ Mathematical Discovery Gallery

**Test purpose:** Six mathematically distinct scenes sampled by Matplot++, visualized inside a native ImGui/SDL2 gallery with labeled previews, animated drawing, zoom and optional SVG export.

The native application displays six mathematical scenes: Lissajous Interference, Clifford Attractor, De Jong Attractor, Harmonic Bloom, Butterfly Curves and Orbital Resonance. Matplot++ 1.2.2 supplies 12,000 numerical samples per scene. The application uses native ImGui 1.93 and SDL2 for labeled previews, scene selection and progressively drawn, rotating detail visualizations. The font atlas is rendered as an actual SDL texture; labels are not converted to textless SVG triangles.

Select one of six tiles and choose Explore scene. Pause, restart, adjust speed and zoom, switch to the next or previous scene, or return to the gallery. SVG export is available but not needed for live viewing.

The headless CTest verifies all six numerical scenes without opening a GUI. The separate `--export` mode creates six SVG files for independent presentation in the central BuildEngine Server. These exports are optional and not the main user experience.

See [build instructions](build.md), [license](LICENSE.md) and [SBOM](SBOM.md).

**Verification:** Native BCC64X build and interactive operation confirmed by the operator on 2026-11-18.
