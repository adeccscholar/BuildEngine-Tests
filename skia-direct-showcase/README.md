# Skia Direct BCC64X CPU/Raster Test Showcase

This executable is a direct consumer of the installed BCC64X-built `skia.lib`.
It does not use SkiaBridge and creates exactly one borderless fullscreen Win32 window.

Scenes:

1. Images - reconstructs the supplied control composition from four source PNG files.
2. Primitives - anti-aliased and aliased rectangles, round rectangles, circles, lines and ovals.
3. Paths - cubic and quadratic Beziers, fill and stroke, even-odd fill.
4. Gradients - linear, radial and mirrored gradients with multiple stops.
5. Transform/Clip - translate, rotate, scale, save/restore and clipping using the supplied C++ logo.
6. BlendModes - SrcOver, Multiply, Screen, Overlay, Plus and Difference.
7. Stress/FPS - adjustable 2,000..40,000 Skia draw calls per frame with a Skia-rendered FPS/render-time overlay.

Keyboard:

- `Esc`: exit
- `Left` / `Right`: previous / next scene
- `1` .. `7`: select scene
- `F`: toggle fit-to-screen / 1:1 design pixels
- `Space`: toggle animation / pause-resume the stress loop
- `Up` / `Down`: increase / decrease stress load in scene 7
- `R`: reset to scene 1, fit enabled, animation disabled

PNG decoding remains WIC-only because the proven CPU/Raster Skia package was built without bundled PNG codec dependencies. Rendering, scaling, paths, gradients, transforms, clipping and blending are performed by the directly linked Skia library.

## r83 pinned-Skia gradient API

The pinned Skia revision no longer provides `include/effects/SkGradientShader.h`.
Scene 4 therefore uses `include/effects/SkGradient.h` and the current public
`SkShaders::LinearGradient` / `SkShaders::RadialGradient` factories with `SkColor4f`.
The Win32 macros `WIN32_LEAN_AND_MEAN` and `NOMINMAX` are supplied by CMake only.

## r84 installed-header closure

`include/core/SkColorSpace.h` includes `modules/skcms/skcms.h`; that header in turn
includes its local `src/skcms_public.h`. The direct Skia package therefore installs
the `modules/skcms/**/*.h` tree next to the normal `include/**` tree. Script 50
auto-refreshes an older installed package through script 47 when these transitive
headers are missing; no Skia rebuild is required.

## r85 stress/FPS scene

Scene 7 continuously invalidates and renders as fast as the Win32 message loop permits,
without the 16 ms animation timer used by the normal animated scene. The default load is
20,000 Skia draw calls per frame; Up/Down selects 2,000, 5,000, 10,000, 20,000 or 40,000.
The overlay is drawn with Skia rectangles through a tiny built-in 5x7 bitmap glyph set, so
no Skia font/text subsystem is introduced yet. FPS is a smoothed frame-start rate; MS is
the smoothed CPU time from frame start through Skia raster rendering before the final GDI blit.

## RAD Studio IDE project (r89)

`SkiaDirectShowcase.cbproj` opens the same source directly in RAD Studio 13 Florence for `Win64x` Debug/Release editing, building and debugging. See `IDE-PROJECT.md` for package paths, output layout and the MSBuild verification command.
