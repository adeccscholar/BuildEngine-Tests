# Navigation Reactor — Terrain Intelligence

## Explore the boundary between safe ground and danger

An alien landscape of ridges and ravines glows turquoise when a mobile agent can cross it and magenta when the terrain is too steep. Drag the **maximum slope** and **terrain relief** sliders: every change rebuilds a genuine Recast navigation raster, replacing the entire coloured surface with newly computed results. Watch thousands of triangles become a navigability map.

## Actual Recast computation

The program generates a reproducible 81 by 51 terrain mesh from radial and oscillatory elevation functions. Its triangles are passed to `rcMarkWalkableTriangles` with the selected agent slope. Recast then allocates an `rcHeightfield`, calculates voxel dimensions and executes `rcRasterizeTriangles`. The resulting `rcSpan` area flags determine every green and magenta cell in the native ImGui draw list. The displayed triangle counts and rasterized spans are returned by the actual Recast calls, not fixed HUD placeholders.

The demonstration covers the **real walkability classification and rasterization stages** of Recast's navigation pipeline; it does not claim to implement Recast's complete polygon-mesh extraction, Detour path planning, crowd simulation or collision physics.

## Controls and evidence

Adjust agent slope from 5 to 80 degrees, terrain relief from 0.25 to 2.25, or rebuild the field explicitly. `navigation-reactor.exe --verify` creates two terrains with different slope limits and checks that higher permitted slopes yield more walkable triangles while both are successfully voxelized.

The program consumes the separately installed Recast Navigation 1.6.0 DLL and the existing native SDL2 + ImGui presentation stack. **[not verified]** Visual acceptance and Windows BCC64X CTest execution remain pending.
