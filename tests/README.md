# Complex Integration Tests

This directory contains cross-package consumer tests that deliberately go beyond the simple per-library smoke pattern.

A test may combine multiple published packages from `Win64x`, multiple processes or services, and its own application-level validation. Small tests that verify only the usability of one library belong in the library contract under `BuildEngine-Admin/admin/smokes`, not here.

The cross-package [Neon Physics Arena](../neon-physics-arena/) is intentionally a separate top-level standalone BuildEngine-Tests CMake project, like the VTK visualization and OpenMP demos. It combines SDL3 and Box2D DLLs in a real interactive renderer and independent deterministic CTest workload.
