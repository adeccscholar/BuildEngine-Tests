# Navigation Reactor — Detour Expedition

## Live terrain and navigation laboratory

The interactive showcase displays the native Recast walkability raster in teal and magenta, plus a gold route calculated through **Detour's actual navigation query**. Green and pink beacons mark the projected path endpoints; a blue agent travels the returned waypoint corridor. The visualization is an overlay of real path data, not a replacement graph search or pre-rendered animation.

Controls: maximum walkable slope, terrain relief, an optional central excluded obstacle, rebuild, and animated traversal. Each terrain/obstacle change builds a new Recast polygon navmesh and re-queries Detour; missing paths are shown as **NO COMPLETE PATH**, not invented routes.

## Real producer functionality

The BuildEngine managed Recast Navigation 1.6.0 package provides separate Recast and Detour dynamic libraries. The showcase uses Recast triangle classification and heightfield rasterization, then compact field construction, erosion, monotone regions, contours, polygon mesh and detail mesh generation. The polygon mesh is serialized into a Detour navigation mesh and queried using nearest-polygons, `findPath` and `findStraightPath`. The UI renders those returned 3D points projected onto the raster.

This is a reproducible navigation demonstration, **not** a full physics simulation, crowd AI, dynamic tile cache, interactive 3D camera or collision avoidance engine. The central obstacle is applied to navigation triangle classification and indicated visually; the background terrain raster remains a separate diagnostic layer.

## Validation

The native `--verify` mode keeps the original slope/raster checks and additionally requests a complete native Detour path on a fixed low-relief terrain without the central obstacle. The headless check must report `SHOWCASE|CHECK|detour-route|PASS` before accepting the full result. Native Windows/BCC64X build, runtime closure and route completion remain **[not verified]** until executed on the host.

## Dependency decision

The managed package already disables upstream `RECASTNAVIGATION_DEMO`, `RECASTNAVIGATION_TESTS` and `RECASTNAVIGATION_EXAMPLES`, builds shared Recast/Detour/DetourCrowd/DetourTileCache/DebugUtils artifacts and does not declare an external dependency in the BuildEngine library catalog. No upstream SDL/OpenGL demo is linked into the showcase. The application links only the installed Recast and Detour import libraries plus separately managed ImGui and SDL2; no additional rendering or physics library is introduced.
