# Navigation Reactor — 3D Detour Expedition

## Actual 3D terrain, not a coloured flat raster

The primary view renders **the very same 81 x 51 source terrain vertex grid and 8,000 triangles** that the native Recast pipeline classifies and rasterizes. Perspective projection, altitude-aware colouring, triangle lighting and optional wireframes make the landscape spatially legible. Terrain heights are not reconstructed from or approximated by the navigation result: the visualization and Recast both consume exactly the same world-coordinate xyz arrays.

Rotate the camera by dragging the main viewport with either mouse button, zoom with the mouse wheel, or use the explicit azimuth, elevation and camera-distance sliders. Disable **3D terrain** to inspect the previous 2D voxel/walkability raster. Toggle **Mesh wireframe** to inspect the actual terrain triangulation.

## Navigation and route

Recast performs walkability classification, rasterization, compact heightfield construction, erosion, region segmentation, contour extraction, polygon/detail mesh construction and serialization into a native Detour navmesh. Detour `findNearestPoly`, `findPath`, and `findStraightPath` produce the waypoints used by the visualizer. The route, start/goal beacons and animated agent are projected through the **same 3D camera** as the source mesh, including the returned Y elevations. A route is never drawn when Detour cannot establish a complete path.

Sliders for agent slope and terrain relief rebuild the Recast/Detour navigation, while the **Central obstacle** checkbox removes triangle areas from navigation and visualizes the exclusion zone. **Animate agent** controls the route traversal indicator. The overlay is a visualization of returned Detour coordinates, not a separately calculated shortest path.

## Capability boundaries

This is perspective 3D rendering through the existing native ImGui/SDL2 geometry path, without a new renderer library or graphics API dependency. It demonstrates geometry and pathfinding, **not** physically-based lighting, shadows, collision simulation, dynamic obstacles, multiple agents, DetourCrowd, DetourTileCache or user-selectable route endpoints. The reference 2D layer remains a separate voxel diagnostic.

## Verification

`navigation-reactor.exe --verify` checks the original Recast slope response, triangle count and rasterization, plus a complete Detour path on a deterministic terrain. It does not inspect pixels or mouse behaviour. Windows BCC64X compilation, screenshot/visual acceptance and real runtime execution are **[not verified]** until performed on the host.

## Dependency decisions

The installed Recast Navigation 1.6.0 package builds separate DLLs, with upstream demo, examples and tests disabled. The showcase links the managed Recast/Detour imports, ImGui and SDL2. No extra rendering framework, standalone bundled navigation sources, physics engine or SDK DLL sweep has been added.
