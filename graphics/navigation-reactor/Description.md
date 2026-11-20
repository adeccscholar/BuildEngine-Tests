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

The installed Recast Navigation 1.6.0 package builds separate DLLs, with upstream demo, examples and tests disabled. The showcase links managed Recast/Detour, Mesa OpenGL 26.2.1, GLEW 2.3.1, Dear ImGui 1.93.0 with its **official SDL2/OpenGL3 backend DLL**, and SDL2 2.32.10. The core ImGui library remains independent of the backend and the backend sources are compiled by the ImGui producer, not bundled with or reimplemented by this showcase. No additional rendering framework, physics engine or SDK DLL sweep is added.

## Official backend integration and GPU acceptance

The custom ImGui draw-command adapter has been removed. `OpenGLShowcase.h` is only SDL2/WGL context lifecycle and calls `ImGui_ImplSDL2_InitForOpenGL`, `ImGui_ImplOpenGL3_Init`, `ImGui_ImplSDL2_ProcessEvent`, both official `NewFrame` functions and `ImGui_ImplOpenGL3_RenderDrawData`. These symbols are exported by the extended, managed ImGui backend DLL. `TerrainGL.h` retains the showcase-specific GPU scene renderer for the genuine Recast vertex mesh and native Detour path: custom scene shaders are appropriate here, unlike replacing Dear ImGui's upstream renderer.

The GPU renderer uses a dedicated FBO with depth attachment, GPU vertex buffers and GLSL 330 core shaders. On startup it requests an OpenGL 3.3 core profile, initializes the existing managed GLEW and displays the actual context version. Mesa 26.2.1 is installed in its configured Windows Gallium softpipe profile, so hardware acceleration and frame rate are not asserted. The optional higher OpenGL profiles are not disabled by the code; the 3.3 requirement is the chosen minimum API.

`NavigationReactorMesaOpenGL` invokes `--verify-gl`, requiring a real SDL2/OpenGL context and successful terrain renderer execution, whereas the original CTest covers Recast/Detour without opening a window. Neither is considered PASS until the Windows host reports it. All producer and showcase reports remain **[not verified]** until executed.
