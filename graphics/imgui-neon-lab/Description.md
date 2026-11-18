# ImGui Neon Constellation

**Test purpose:** Native ImGui draw-list geometry exported as an offline neon particle constellation.

This independent BCC64X C++23 demonstration consumes the managed **imgui** library package(s) and exports a 1280 x 720 vector composition for full-screen presentation.

## What happens

A native ImGui draw-list constructs a luminous geometric constellation from 420 parametric nodes.

The result is written as an SVG in the current working directory, without external gnuplot, renderer installation, or network access. The SVG is an offline generated rendering, not GPU acceleration or a real-time application.

The `--verify` option exercises native library operations without producing an SVG and is executed by CTest. See [build.md](build.md), [license](LICENSE.md) and [declared SBOM](SBOM.md).

**[not verified]** Native Windows/BCC64X compilation and presentation appearance await execution on the target workstation.
