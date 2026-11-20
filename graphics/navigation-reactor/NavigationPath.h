#pragma once

#include <Recast.h>
#include <DetourAlloc.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

struct NavigationRoute {
   std::vector<std::array<float, 3>> vecPoints;
   int iPolygons {};
   int iCorridor {};
   bool bPathComplete {};
};

/**
\brief Builds a Recast polygon navmesh and a genuine Detour path.
\details Rasterizes the same source triangles, optionally cuts an obstacle in
         the walkable area, builds compact regions and polygons, then performs
         Detour nearest-poly, corridor and straight-path queries. No grid-based
         or graphic-only pathfinding is used.
\param vecVertices Terrain vertex triples.
\param vecTriangles Triangle vertex indices.
\param flSlope Maximum walkable surface slope.
\param flRelief Terrain relief used to widen the height envelope.
\param bObstacle Whether a circular central obstacle is excluded.
\returns Detour route and polygon counts; an empty route means no complete path.
\throw std::runtime_error When native mesh construction or initialization fails.
*/
inline NavigationRoute BuildNavigationRoute(
   std::vector<float> const& vecVertices, std::vector<int> const& vecTriangles,
   float const flSlope, float const flRelief, bool const bObstacle) {
   auto const freeField = [](rcHeightfield* p) { if(p) rcFreeHeightField(p); };
   auto const freeCompact = [](rcCompactHeightfield* p) { if(p) rcFreeCompactHeightfield(p); };
   auto const freeContours = [](rcContourSet* p) { if(p) rcFreeContourSet(p); };
   auto const freePoly = [](rcPolyMesh* p) { if(p) rcFreePolyMesh(p); };
   auto const freeDetail = [](rcPolyMeshDetail* p) { if(p) rcFreePolyMeshDetail(p); };
   auto const freeNav = [](dtNavMesh* p) { if(p) dtFreeNavMesh(p); };
   auto const freeQuery = [](dtNavMeshQuery* p) { if(p) dtFreeNavMeshQuery(p); };
   std::unique_ptr<rcHeightfield, decltype(freeField)> pField(rcAllocHeightfield(), freeField);
   std::unique_ptr<rcCompactHeightfield, decltype(freeCompact)> pCompact(rcAllocCompactHeightfield(), freeCompact);
   std::unique_ptr<rcContourSet, decltype(freeContours)> pContours(rcAllocContourSet(), freeContours);
   std::unique_ptr<rcPolyMesh, decltype(freePoly)> pPoly(rcAllocPolyMesh(), freePoly);
   std::unique_ptr<rcPolyMeshDetail, decltype(freeDetail)> pDetail(rcAllocPolyMeshDetail(), freeDetail);
   if(!pField || !pCompact || !pContours || !pPoly || !pDetail)
      throw std::runtime_error("Recast navigation allocation failed");

   constexpr float flCell = 0.2F;
   constexpr float flHeight = 0.12F;
   int const iVertices = static_cast<int>(vecVertices.size() / 3);
   int const iTriangles = static_cast<int>(vecTriangles.size() / 3);
   std::vector<unsigned char> vecArea(static_cast<std::size_t>(iTriangles), RC_NULL_AREA);
   rcContext theContext {false};
   rcMarkWalkableTriangles(&theContext, flSlope, vecVertices.data(), iVertices,
      vecTriangles.data(), iTriangles, vecArea.data());
   if(bObstacle) {
      for(int i = 0; i < iTriangles; ++i) {
         float flX = 0.0F, flZ = 0.0F;
         for(int j = 0; j < 3; ++j) {
            int const iVertex = vecTriangles[static_cast<std::size_t>(i * 3 + j)];
            flX += vecVertices[static_cast<std::size_t>(iVertex * 3)];
            flZ += vecVertices[static_cast<std::size_t>(iVertex * 3 + 2)];
         }
         flX /= 3.0F;
         flZ /= 3.0F;
         if(std::hypot(flX - 8.0F, flZ - 5.0F) < 1.8F)
            vecArea[static_cast<std::size_t>(i)] = RC_NULL_AREA;
      }
   }
   float arrMin[3] {}, arrMax[3] {};
   rcCalcBounds(vecVertices.data(), iVertices, arrMin, arrMax);
   arrMin[1] -= 2.0F + flRelief;
   arrMax[1] += 2.0F + flRelief;
   int iWidth {}, iHeight {};
   rcCalcGridSize(arrMin, arrMax, flCell, &iWidth, &iHeight);
   constexpr int iWalkHeight = 12;
   constexpr int iWalkClimb = 5;
   constexpr int iWalkRadius = 1;
   if(!rcCreateHeightfield(&theContext, *pField, iWidth, iHeight, arrMin, arrMax, flCell, flHeight) ||
      !rcRasterizeTriangles(&theContext, vecVertices.data(), iVertices,
         vecTriangles.data(), vecArea.data(), iTriangles, *pField, iWalkClimb) ||
      !rcBuildCompactHeightfield(&theContext, iWalkHeight, iWalkClimb, *pField, *pCompact) ||
      !rcErodeWalkableArea(&theContext, iWalkRadius, *pCompact) ||
      !rcBuildRegionsMonotone(&theContext, *pCompact, 0, 4, 16) ||
      !rcBuildContours(&theContext, *pCompact, 1.3F, 12, *pContours) ||
      !rcBuildPolyMesh(&theContext, *pContours, 6, *pPoly))
      throw std::runtime_error("Recast navmesh construction failed");
   NavigationRoute theRoute;
   theRoute.iPolygons = pPoly->npolys;
   if(pPoly->npolys == 0)
      return theRoute;
   if(!rcBuildPolyMeshDetail(&theContext, *pPoly, *pCompact, 1.2F, .25F, *pDetail))
      throw std::runtime_error("Recast detail mesh construction failed");
   for(int i = 0; i < pPoly->npolys; ++i) {
      pPoly->flags[i] = 1;
      if(pPoly->areas[i] == RC_WALKABLE_AREA)
         pPoly->areas[i] = 0;
   }
   dtNavMeshCreateParams theParams {};
   theParams.verts = pPoly->verts;
   theParams.vertCount = pPoly->nverts;
   theParams.polys = pPoly->polys;
   theParams.polyAreas = pPoly->areas;
   theParams.polyFlags = pPoly->flags;
   theParams.polyCount = pPoly->npolys;
   theParams.nvp = pPoly->nvp;
   theParams.detailMeshes = pDetail->meshes;
   theParams.detailVerts = pDetail->verts;
   theParams.detailVertsCount = pDetail->nverts;
   theParams.detailTris = pDetail->tris;
   theParams.detailTriCount = pDetail->ntris;
   theParams.walkableHeight = iWalkHeight * flHeight;
   theParams.walkableRadius = iWalkRadius * flCell;
   theParams.walkableClimb = iWalkClimb * flHeight;
   rcVcopy(theParams.bmin, pPoly->bmin);
   rcVcopy(theParams.bmax, pPoly->bmax);
   theParams.cs = flCell;
   theParams.ch = flHeight;
   theParams.buildBvTree = true;
   unsigned char* pData = nullptr;
   int iDataSize = 0;
   if(!dtCreateNavMeshData(&theParams, &pData, &iDataSize))
      throw std::runtime_error("Detour navmesh serialization failed");
   std::unique_ptr<dtNavMesh, decltype(freeNav)> pNav(dtAllocNavMesh(), freeNav);
   if(!pNav) {
      dtFree(pData);
      throw std::runtime_error("Detour navmesh allocation failed");
   }
   dtStatus const iInit = pNav->init(pData, iDataSize, DT_TILE_FREE_DATA);
   if(dtStatusFailed(iInit)) {
      dtFree(pData);
      throw std::runtime_error("Detour navmesh initialization failed");
   }
   std::unique_ptr<dtNavMeshQuery, decltype(freeQuery)> pQuery(dtAllocNavMeshQuery(), freeQuery);
   if(!pQuery || dtStatusFailed(pQuery->init(pNav.get(), 4096)))
      throw std::runtime_error("Detour query initialization failed");

   float const arrStart[3] {1.1F, 0.0F, 1.1F};
   float const arrEnd[3] {14.9F, 0.0F, 8.9F};
   float const arrExtent[3] {1.0F, 10.0F, 1.0F};
   float arrNearStart[3] {}, arrNearEnd[3] {};
   dtPolyRef iStartRef {}, iEndRef {};
   dtQueryFilter theFilter;
   if(dtStatusFailed(pQuery->findNearestPoly(arrStart, arrExtent, &theFilter, &iStartRef, arrNearStart)) ||
      dtStatusFailed(pQuery->findNearestPoly(arrEnd, arrExtent, &theFilter, &iEndRef, arrNearEnd)) ||
      !iStartRef || !iEndRef)
      return theRoute;
   std::array<dtPolyRef, 1024> arrCorridor {};
   int iPathCount = 0;
   dtStatus const iPath = pQuery->findPath(iStartRef, iEndRef, arrNearStart,
      arrNearEnd, &theFilter, arrCorridor.data(), &iPathCount,
      static_cast<int>(arrCorridor.size()));
   if(dtStatusFailed(iPath) || iPathCount == 0 || arrCorridor[static_cast<std::size_t>(iPathCount - 1)] != iEndRef)
      return theRoute;
   theRoute.iCorridor = iPathCount;
   std::array<float, 256 * 3> arrStraight {};
   std::array<unsigned char, 256> arrFlags {};
   int iStraight = 0;
   dtStatus const iStraightStatus = pQuery->findStraightPath(arrNearStart, arrNearEnd,
      arrCorridor.data(), iPathCount, arrStraight.data(), arrFlags.data(), nullptr,
      &iStraight, 256);
   if(dtStatusFailed(iStraightStatus) || iStraight < 2)
      return theRoute;
   for(int i = 0; i < iStraight; ++i)
      theRoute.vecPoints.push_back({arrStraight[static_cast<std::size_t>(i * 3)],
         arrStraight[static_cast<std::size_t>(i * 3 + 1)],
         arrStraight[static_cast<std::size_t>(i * 3 + 2)]});
   theRoute.bPathComplete = true;
   return theRoute;
}
