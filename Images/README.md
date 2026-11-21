# Shared showcase image assets

This directory belongs to the synchronized `BuildEngine-Tests` checkout at
`<ProductionRoot>/tests/Images`. The BuildEngine HTTP server exposes **static**
images at `/images/*`, where the URL suffix maps to the same relative path below
this directory. Supported media types: SVG, PNG, JPEG, GIF, WebP.

## Structure

```text
Images/
  README.md
  showcase/<manifest-path-with-slashes-replaced-by-dashes>/
    tile.svg        # cinematic, repository-owned, topic-specific concept artwork
    tile.webp       # optional original generated bitmap or runtime screenshot
    hero.png        # optional large original generated or captured image
  manuals/<same-key>/
    concept/window.svg # generated provisional desktop application window
    runtime/        # ONLY genuine screenshots from an executed native application
    concept/        # explanatory artwork, never represented as screenshots
```

The fourteen gallery tiles currently covered by this image contract are original, high-contrast, generated SVG concept illustrations under
`Images/showcase/`. Their showcase descriptions also embed a **second**
image: a provisional desktop-window mockup stored under
`Images/manuals/<key>/concept/window.svg`. Neither image is a runtime screenshot. The original high-resolution generated PNG/WebP
illustrations are also available as a separately delivered asset package for
manual checkout installation. Their existence in that package does **not**
mean the corresponding PNG files have been committed to the repository.

## Manifest contract

```json
{
  "images": {
    "tile": "/images/showcase/graphics-hdr-photon-observatory/tile.svg",
    "hero": "/images/showcase/graphics-hdr-photon-observatory/tile.svg",
    "kind": "generated-concept",
    "alt": "Original technical concept illustration; not an application screenshot"
  }
}
```

An image URL must point to an actual local file; the browser never fetches
third-party artwork. The server rejects path traversal, symlink aliases and
unsupported extensions. Image provenance is shown in the gallery and in the
showcase detail page.

## Image roles and acceptance

- The **gallery tile** illustrates the showcase theme and must remain visually compelling at small sizes.
- The **desktop-window mockup** is embedded directly in the showcase `Description.md` and illustrates a possible UI; synthetic numerical values, screen labels and controls are not functional evidence.
- When a working native application is available, put actual runtime captures under the separate `runtime/` directory and replace only the corresponding documentation references. Keep concepts identifiable as concepts.
- The source manifest `images.kind` stays `generated-concept` until its tile/hero URL references genuine captured imagery. A mockup in the description does not change that classification.

## Replacing an illustration with an actual screenshot

1. Run the native application with its normal dependencies and library closure.
2. Where supported, use **F11** to switch to fullscreen and **F1** to show About.
3. Capture the actual application screen, without generating or recreating the
   screen contents with an image model.
4. Store the screenshot under `Images/manuals/<key>/runtime/` or as a
   `showcase/<key>/tile.webp`, preserving an untouched original when possible.
5. Point the specific `showcase.json` `images.tile` or `images.hero` field
   at the new file and set `images.kind` to `runtime-screenshot`.
6. Update the corresponding `Description.md` image URL and caption; do not
   retain the "generated concept" caption for a real screenshot.
7. Commit the source image, description and manifest together. Run the local
   BuildEngine presentation-cache synchronization and verify HTTP 200 on the
   static asset, `/showcases/` and the showcase detail view.

The supplied key visuals are illustrations of *intent*, not measured
outputs or reproducibility evidence. They do not replace native tests,
SBOMs, screenshots or verification logs.

## Native keyboard policy

For interactive GUI targets that have a window and a key event loop:
- **F1**: About with application title, technologies and help keys.
- **F11**: reversible fullscreen/windowed toggle.
- Headless tests and command-line consumers have no window and therefore no
  F1/F11 shortcut.

For shared SDL2/ImGui showcases, both actions live in
`graphics/common/NativeShowcase.h` or `OpenGLShowcase.h` to avoid divergence.
The standalone VCL, SDL2/SDL3, VTK and Win32 frontends implement the same
shortcuts through their platform event-handling mechanisms. GUI integration
and visual behavior require a native BCC64X run for final verification.

## Gallery and documentation presentation contract

The same showcase image is used in two visible places:

1. The showcase overview gallery renders `images.tile` as the image at the top of its card, linking **only to the showcase description**, not to source files. PostgreSQL and filesystem renderer paths both support the display.
2. The showcase description embeds the gallery artwork as a hero illustration and separately embeds the provisional desktop UI image from `Images/manuals/<showcase-key>/concept/window.svg`.

The provisional UI image is *not* a runtime screenshot and should remain so labelled. Replace its documentation URL with a verified asset under `runtime/` once the native application has been accepted. Concept art and screenshots are distinct evidence types.

Do not link directly to `.cpp`, `.h`, or `CMakeLists.txt` from showcase descriptions. Explaining architecture, algorithms and dependencies is the purpose of these pages; sources remain internal project implementation. Names of code files may be shown as inline text without linking.

The server serves all such artwork from the synchronized tests checkout under `/images/`. A remote Git commit by itself does not synchronize files into the local server: the image files and updated `showcase.json` must be present in the checkout, and PostgreSQL presentation caches must be refreshed before a cached gallery can display newly declared tiles.

## Additional showcase image coverage (2026-11-21)

The following previously unillustrated showcases now include an original SVG tile,
a separate provisional desktop-window illustration and matching embedded image
references in `Description.md`:

- `graphics/imgui-neon-lab`: Dear ImGui Neon Constellation
- `graphics/implot-signal-theater`: ImPlot Real-Time Signal Theater
- `graphics/opencv-barcode-vision`: OpenCV Barcode Vision
- `openmp-mandelbrot`: Parallel Mandelbrot Explorer

On the gallery cards, the image is constrained to 260 by 148 pixels with
`object-fit: contain` and a dark padded background. This preserves the entire
illustration and avoids clipping. The caption reads `AI generated concept illustration;
not an application screenshot`. Manifest synchronization state and verification
state remain available in their source metadata, but are not repeated as boilerplate
text on every card.
