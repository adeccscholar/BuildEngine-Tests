# Matplot++ Chaos Atlas

**Test purpose:** Parametric mathematics and 2400-point sampled attractor illustration.

This independent BCC64X C++23 demonstration consumes the managed **matplotplusplus** library package(s) and exports a 1280 x 720 vector composition for full-screen presentation.

## What happens

Matplot++ generates 2,400 mathematically defined samples, forming five richly layered parametric attractor structures.

The result is written as an SVG in the current working directory, without external gnuplot, renderer installation, or network access. The SVG is an offline generated rendering, not GPU acceleration or a real-time application.

The `--verify` option exercises native library operations without producing an SVG and is executed by CTest. See [build.md](build.md), [license](LICENSE.md) and [declared SBOM](SBOM.md).

**[not verified]** Native Windows/BCC64X compilation and presentation appearance await execution on the target workstation.
