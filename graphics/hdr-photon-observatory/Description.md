# HDR Photon Observatory — Where Light Meets Optimization

## Invitation

Step into a laboratory where photons become data, data become an inverse problem, and the inverse problem becomes a living picture. The left-hand side of the experiment is an authentic floating-point light field: a luminous point source and diffraction streak with radiance exceeding the range of an ordinary RGB display. The right-hand side is a sensor-calibration problem solved by the **installed Ceres Solver 2.3.0 native DLL** rather than by a custom hand-written numerical optimizer.

The application is a native Windows desktop showcase running through SDL2 and Dear ImGui. It is not a website simulation, a static diagram or a mock OpenEXR encoder. It produces and processes real OpenEXR images using the installed OpenEXR 3.5.4 and Imath libraries.

## Three systems meet

1. **HDR photons:** generate a deterministic two-dimensional field in floating-point radiance units. The central light source has adjustable brightness, position and diffusion width. Separate red, green and blue channels retain values greater than 1.0.
2. **OpenEXR preservation:** export the generated field to a real OpenEXR half-float RGBA file, read it back through `Imf::RgbaInputFile`, check the data window and report the maximum channel error. The display always applies its own explicit exposure/tone mapping; the EXR file itself is not reduced to eight-bit pixels.
3. **Ceres inverse calibration:** create 48 observed sensor samples from a known nonlinear Gaussian light profile. An automatically differentiated three-parameter `ceres::Problem` estimates amplitude, light position and diffusion width. An independent real solver summary provides initial and final costs and iteration count.

**Cyan dots** show measured sensor samples; **gold** shows the current parameterized nonlinear model. After pressing **FIT WITH CERES**, the actual fitted parameters replace the initial guess. Slide the exposure control and watch the light-field appearance change without changing the underlying HDR samples.

## Controls

- **Peak radiance / Light position / Diffusion width:** adjust the synthetic physical scene and press **SYNTHESIZE HDR FIELD** to regenerate true floating-point pixels and measured samples.
- **Display exposure:** continuously adjust the HDR-to-display tone-map. This does not change the written EXR file.
- **FIT WITH CERES:** solve an actual, constrained nonlinear least-squares problem, showing the fit in the gold plot.
- **EXR WRITE / READ:** use the real installed OpenEXR writer and reader; report measured pixel errors or a real failure.
- **F11:** switch the native SDL2 window to fullscreen using the shared BuildEngine showcase renderer.

The written fixture is `buildengine-hdr-photon-observatory.exr` in the operating system temporary directory. The application does not access the network or require any external images.

## Native proof

`hdr-photon-observatory.exe --verify` exercises both primary dependencies without opening a window. It requires:

- Ceres to return a usable solution and recover amplitude, position and diffusion width to specified tolerances, while reducing the real nonlinear cost.
- OpenEXR to write and read a full 320×192 RGBA floating-point raster and maintain maximum pixel error at most 0.015 in half precision.
- The recovered HDR scene to contain radiance values greater than 1.0, proving that the underlying data were not silently clamped to standard display RGB.

This is a deterministic integration exercise, **not** radiometrically calibrated imaging and not proof of production-grade HDR codec robustness, malformed-image handling or camera calibration.

## Dependency provenance

[Active Ceres Solver](/library/ceres-solver/2.3.0/) is a pinned **unreleased development snapshot**, not an official 2.3.0 release. [OpenEXR 3.5.4](/library/openexr/3.5.4/) uses installed Imath 3.2.3, OpenJPH 0.33.0, libdeflate 1.26 and zstd 1.5.7. Ceres consumes independently installed Abseil and Eigen; SDL2 and Dear ImGui are only the presentation layer. The executable uses the BuildEngine BCC64X (Clang/EMBT) toolchain and its dynamic runtime policy. No MinGW or MSVC-produced fallback binaries are used.

**Verification status: [not verified].** Library-level native smokes passed, but this new interactive application has not been built or executed on the Windows BCC64X host.
