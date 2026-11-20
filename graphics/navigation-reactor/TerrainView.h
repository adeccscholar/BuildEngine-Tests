#pragma once

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

/**
\brief Orbit camera used to view the same terrain triangles processed by Recast.
\details Pure CPU projection feeds the existing ImGui/SDL2 geometry renderer.
\note This is a perspective 3D geometry view, not an OpenGL dependency.
*/
struct TerrainCamera {
   float flAzimuth {-0.52F};
   float flElevation {0.76F};
   float flDistance {26.0F};
   float flFocusX {8.0F};
   float flFocusZ {5.0F};
};

/**
\brief Projects world coordinates into the perspective terrain viewport.
\details Uses a yaw/pitch orbit camera with clipping behind the near plane.
\param arrPoint Three dimensional terrain point.
\param aOrigin Viewport upper left corner.
\param aSize Viewport extent.
\param theCamera Current orbit camera.
\returns Screen coordinates with projected depth in the third element.
\throw Nothing.
*/
inline std::array<float, 3> ProjectTerrain(
   std::array<float, 3> const& arrPoint, ImVec2 const aOrigin,
   ImVec2 const aSize, TerrainCamera const& theCamera) {
   float const flX = arrPoint[0] - theCamera.flFocusX;
   float const flY = arrPoint[1];
   float const flZ = arrPoint[2] - theCamera.flFocusZ;
   float const flYawSin = std::sin(theCamera.flAzimuth);
   float const flYawCos = std::cos(theCamera.flAzimuth);
   float const flPitchSin = std::sin(theCamera.flElevation);
   float const flPitchCos = std::cos(theCamera.flElevation);
   float const flRight = flX * flYawCos - flZ * flYawSin;
   float const flForward = flX * flYawSin + flZ * flYawCos;
   float const flVertical = flY * flPitchCos - flForward * flPitchSin;
   float const flDepth = theCamera.flDistance - flForward * flPitchCos -
      flY * flPitchSin;
   float const flFocal = std::min(aSize.x, aSize.y) * 1.60F;
   float const flSafeDepth = std::max(flDepth, 0.1F);
   return {aOrigin.x + aSize.x * 0.5F + flRight * flFocal / flSafeDepth,
      aOrigin.y + aSize.y * 0.53F - flVertical * flFocal / flSafeDepth,
      flDepth};
}

/**
\brief Renders the actual Recast input triangles in a perspective 3D viewport.
\details Painter-sorts all terrain faces by depth and shades them using the
         native Recast triangle classification and surface inclination.
         The central excluded area is rendered as a translucent hazard zone.
\param vecVertices Shared xyz triangle mesh vertices.
\param vecTriangles Shared triangle indices.
\param vecAreas Recast walkability classification for each source triangle.
\param aOrigin Viewport screen origin.
\param aSize Viewport size.
\param theCamera Camera projection parameters.
\param bObstacle Whether the navigation exclusion is active.
\param bWireframe Whether to overlay triangle wireframes.
\returns Nothing.
\throw Nothing intentionally.
*/
inline void DrawTerrain3D(
   std::vector<float> const& vecVertices,
   std::vector<int> const& vecTriangles,
   std::vector<unsigned char> const& vecAreas,
   ImVec2 const aOrigin, ImVec2 const aSize,
   TerrainCamera const& theCamera,
   bool const bObstacle, bool const bWireframe) {
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   ImVec2 const aEnd {aOrigin.x + aSize.x, aOrigin.y + aSize.y};
   pDraw->AddRectFilled(aOrigin, aEnd, IM_COL32(7, 14, 31, 255), 13.0F);
   pDraw->PushClipRect(aOrigin, aEnd, true);
   std::size_t const uVertexCount = vecVertices.size() / 3;
   std::vector<std::array<float, 3>> vecProjected(uVertexCount);
   for(std::size_t u = 0; u < uVertexCount; ++u) {
      vecProjected[u] = ProjectTerrain({
         vecVertices[u * 3], vecVertices[u * 3 + 1], vecVertices[u * 3 + 2]
      }, aOrigin, aSize, theCamera);
   }
   std::size_t const uTriangles = vecTriangles.size() / 3;
   std::vector<std::size_t> vecOrder(uTriangles);
   std::iota(vecOrder.begin(), vecOrder.end(), 0);
   auto const Depth = [&](std::size_t const u) -> float {
      std::size_t const uBase = u * 3;
      return (vecProjected[static_cast<std::size_t>(vecTriangles[uBase])][2] +
         vecProjected[static_cast<std::size_t>(vecTriangles[uBase + 1])][2] +
         vecProjected[static_cast<std::size_t>(vecTriangles[uBase + 2])][2]) / 3.0F;
   };
   std::sort(vecOrder.begin(), vecOrder.end(), [&](std::size_t const uA,
      std::size_t const uB) { return Depth(uA) > Depth(uB); });
   for(std::size_t const u : vecOrder) {
      std::size_t const uBase = u * 3;
      std::size_t const uA = static_cast<std::size_t>(vecTriangles[uBase]);
      std::size_t const uB = static_cast<std::size_t>(vecTriangles[uBase + 1]);
      std::size_t const uC = static_cast<std::size_t>(vecTriangles[uBase + 2]);
      if(vecProjected[uA][2] <= 0.1F || vecProjected[uB][2] <= 0.1F ||
         vecProjected[uC][2] <= 0.1F) continue;
      auto const Point = [&](std::size_t const uV) -> ImVec2 {
         return {vecProjected[uV][0], vecProjected[uV][1]};
      };
      float const flAX = vecVertices[uA * 3], flAY = vecVertices[uA * 3 + 1],
         flAZ = vecVertices[uA * 3 + 2];
      float const flBX = vecVertices[uB * 3], flBY = vecVertices[uB * 3 + 1],
         flBZ = vecVertices[uB * 3 + 2];
      float const flCX = vecVertices[uC * 3], flCY = vecVertices[uC * 3 + 1],
         flCZ = vecVertices[uC * 3 + 2];
      float const flUX = flBX - flAX, flUY = flBY - flAY, flUZ = flBZ - flAZ;
      float const flVX = flCX - flAX, flVY = flCY - flAY, flVZ = flCZ - flAZ;
      float const flNY = flUZ * flVX - flUX * flVZ;
      float const flNLen = std::sqrt(
         std::pow(flUY * flVZ - flUZ * flVY, 2.0F) +
         flNY * flNY +
         std::pow(flUX * flVY - flUY * flVX, 2.0F));
      float const flLight = std::clamp(0.44F + 0.55F *
         std::abs(flNY) / std::max(flNLen, 0.001F), 0.30F, 1.0F);
      bool const bWalkable = u < vecAreas.size() && vecAreas[u] != 0;
      float const flCXMid = (flAX + flBX + flCX) / 3.0F;
      float const flCZMid = (flAZ + flBZ + flCZ) / 3.0F;
      bool const bExcluded = bObstacle &&
         std::hypot(flCXMid - 8.0F, flCZMid - 5.0F) < 1.8F;
      float const flHeight = (flAY + flBY + flCY) / 3.0F;
      float const flAltitude = std::clamp((flHeight + 1.7F) / 4.4F, 0.0F, 1.0F);
      auto const Channel = [flLight](float const flV) -> int {
         return static_cast<int>(std::clamp(flV * flLight, 0.0F, 255.0F));
      };
      ImU32 const uColor = bExcluded ?
         IM_COL32(Channel(87), Channel(39), Channel(89), 255) :
         bWalkable ?
            IM_COL32(Channel(25 + flAltitude * 90),
               Channel(150 + flAltitude * 65),
               Channel(133 + flAltitude * 65), 255) :
            IM_COL32(Channel(195 + flAltitude * 42),
               Channel(57 + flAltitude * 34),
               Channel(104 + flAltitude * 52), 255);
      ImVec2 const aA = Point(uA), aB = Point(uB), aC = Point(uC);
      pDraw->AddTriangleFilled(aA, aB, aC, uColor);
      if(bWireframe)
         pDraw->AddTriangle(aA, aB, aC, IM_COL32(7, 25, 40, 100), 0.65F);
   }
   pDraw->PopClipRect();
}

/**
\brief Projects native Detour waypoints into the same 3D view.
\details Never derives route coordinates from the rendered triangles.
\param vecPoints Detour straight-path output positions.
\param aOrigin View origin.
\param aSize View extent.
\param theCamera Current camera.
\param flPhase Normalized agent travel progress.
\returns Nothing.
\throw Nothing intentionally.
*/
inline void DrawTerrainRoute3D(
   std::vector<std::array<float, 3>> const& vecPoints,
   ImVec2 const aOrigin, ImVec2 const aSize,
   TerrainCamera const& theCamera, float const flPhase) {
   if(vecPoints.size() < 2) return;
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->PushClipRect(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, true);
   float flTotal = 0.0F;
   for(std::size_t u = 1; u < vecPoints.size(); ++u)
      flTotal += std::hypot(vecPoints[u][0] - vecPoints[u - 1][0],
         vecPoints[u][2] - vecPoints[u - 1][2]);
   float flRemaining = flTotal * std::clamp(flPhase, 0.0F, 1.0F);
   ImVec2 aAgent {};
   bool bAgentValid = false;
   for(std::size_t u = 1; u < vecPoints.size(); ++u) {
      auto const aA = ProjectTerrain(vecPoints[u - 1], aOrigin, aSize, theCamera);
      auto const aB = ProjectTerrain(vecPoints[u], aOrigin, aSize, theCamera);
      if(aA[2] <= 0.1F || aB[2] <= 0.1F) continue;
      ImVec2 const aStart {aA[0], aA[1] - 4.0F};
      ImVec2 const aEnd {aB[0], aB[1] - 4.0F};
      pDraw->AddLine(aStart, aEnd, IM_COL32(17, 22, 35, 255), 8.0F);
      pDraw->AddLine(aStart, aEnd, IM_COL32(255, 205, 64, 255), 4.0F);
      float const flLength = std::hypot(vecPoints[u][0] - vecPoints[u - 1][0],
         vecPoints[u][2] - vecPoints[u - 1][2]);
      if(!bAgentValid || flRemaining > 0.0F) {
         float const flT = flLength > 0.0F ? std::min(flRemaining / flLength, 1.0F) : 1.0F;
         aAgent = {aStart.x + (aEnd.x - aStart.x) * flT,
            aStart.y + (aEnd.y - aStart.y) * flT};
         bAgentValid = true;
         flRemaining -= flLength;
      }
   }
   auto const aFirst = ProjectTerrain(vecPoints.front(), aOrigin, aSize, theCamera);
   auto const aLast = ProjectTerrain(vecPoints.back(), aOrigin, aSize, theCamera);
   if(aFirst[2] > 0.1F)
      pDraw->AddCircleFilled({aFirst[0], aFirst[1] - 4.0F}, 9.0F,
         IM_COL32(80, 240, 172, 255));
   if(aLast[2] > 0.1F)
      pDraw->AddCircleFilled({aLast[0], aLast[1] - 4.0F}, 9.0F,
         IM_COL32(255, 104, 142, 255));
   if(bAgentValid) {
      pDraw->AddCircleFilled(aAgent, 13.0F, IM_COL32(244, 246, 255, 255));
      pDraw->AddCircleFilled(aAgent, 8.0F, IM_COL32(51, 115, 255, 255));
   }
   pDraw->PopClipRect();
}
