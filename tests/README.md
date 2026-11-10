# Complex Integration Tests

This directory contains cross-package consumer tests that deliberately go beyond the simple per-library smoke pattern.

A test may combine multiple published packages from `Win64x`, multiple processes or services, and its own application-level validation. Small tests that verify only the usability of one library belong in the library contract under `BuildEngine-Admin/admin/smokes`, not here.
