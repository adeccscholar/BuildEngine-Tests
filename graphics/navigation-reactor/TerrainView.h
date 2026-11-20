#pragma once

/**
\brief Orbit-camera parameters for the native OpenGL terrain renderer.
\details Stores orientation, distance and focal point independently of
         the Recast input mesh, Detour navigation query and GPU renderer.
\note 3D rendering is provided exclusively by TerrainGL, not by CPU rasterization.
*/
struct TerrainCamera {
   float flAzimuth {-0.52F};
   float flElevation {0.76F};
   float flDistance {26.0F};
   float flFocusX {8.0F};
   float flFocusZ {5.0F};
};
