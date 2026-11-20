#include "../common/GraphicsDllImports.h"
#include "../common/OpenGLShowcase.h"

#include <Recast.h>
#include "NavigationPath.h"
#include "TerrainView.h"
#include "TerrainGL.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <memory>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string_view>
#include <string>
#include <chrono>
#include <vector>

namespace {
constexpr int iColumns = 81;
constexpr int iRows = 51;
constexpr float flCellSize = 0.2F;

struct RecastField {
   struct FieldDeleter {
      void operator()(rcHeightfield* const pField) const noexcept {
         if(pField != nullptr) rcFreeHeightField(pField);
      }
   };
   std::unique_ptr<rcHeightfield, FieldDeleter> pHeightfield;
   std::vector<unsigned char> vecArea;
   std::vector<float> vecVertices;
   std::vector<int> vecTriangles;
   int iWalkableTriangles {};
   int iTotalTriangles {};
   int iCells {};
   NavigationRoute theRoute;
};

/**
\brief Builds and rasterizes a real Recast terrain heightfield.
\details A deterministic triangular landscape is passed through the upstream
         rcMarkWalkableTriangles and rcRasterizeTriangles algorithms.
\param flSlope Maximum walkable slope in degrees.
\param flRuggedness Vertical relief multiplier.
\returns Owned rasterized heightfield and count of walkable surface triangles.
\throw std::runtime_error If heightfield allocation or rasterization fails.
*/
RecastField BuildField(float const flSlope, float const flRuggedness) {
   RecastField theField;
   std::vector<float> vecVertices;
   std::vector<int> vecTriangles;
   vecVertices.reserve(static_cast<std::size_t>(iColumns * iRows * 3));
   vecTriangles.reserve(static_cast<std::size_t>((iColumns - 1) * (iRows - 1) * 6));
   for(int iZ = 0; iZ < iRows; ++iZ) {
      for(int iX = 0; iX < iColumns; ++iX) {
         float const flX = static_cast<float>(iX) * flCellSize;
         float const flZ = static_cast<float>(iZ) * flCellSize;
         float const flR = std::hypot(flX - 8.0F, flZ - 5.0F);
         float const flY = flRuggedness * (
            1.7F * std::sin(flR * 1.35F) * std::exp(-flR * .17F) +
            .38F * std::cos(flX * 2.2F) * std::sin(flZ * 1.9F));
         vecVertices.push_back(flX);
         vecVertices.push_back(flY);
         vecVertices.push_back(flZ);
      }
   }
   for(int iZ = 0; iZ + 1 < iRows; ++iZ) {
      for(int iX = 0; iX + 1 < iColumns; ++iX) {
         int const iA = iZ * iColumns + iX;
         int const iB = iA + 1;
         int const iC = iA + iColumns;
         int const iD = iC + 1;
         vecTriangles.insert(vecTriangles.end(), {iA, iC, iB, iB, iC, iD});
      }
   }
   theField.iTotalTriangles = static_cast<int>(vecTriangles.size() / 3);
   theField.vecArea.resize(static_cast<std::size_t>(theField.iTotalTriangles), RC_NULL_AREA);
   rcContext theContext { false };
   rcMarkWalkableTriangles(&theContext, flSlope, vecVertices.data(),
                           static_cast<int>(vecVertices.size() / 3),
                           vecTriangles.data(), theField.iTotalTriangles,
                           theField.vecArea.data());
   theField.iWalkableTriangles = static_cast<int>(std::count(
      theField.vecArea.begin(), theField.vecArea.end(), static_cast<unsigned char>(RC_WALKABLE_AREA)));

   float arrBoundsMin[3] {};
   float arrBoundsMax[3] {};
   rcCalcBounds(vecVertices.data(), static_cast<int>(vecVertices.size() / 3), arrBoundsMin, arrBoundsMax);
   arrBoundsMin[1] -= 2.0F;
   arrBoundsMax[1] += 2.0F;
   int iWidth {}, iHeight {};
   rcCalcGridSize(arrBoundsMin, arrBoundsMax, flCellSize, &iWidth, &iHeight);
   theField.pHeightfield.reset(rcAllocHeightfield());
   if(!theField.pHeightfield ||
      !rcCreateHeightfield(&theContext, *theField.pHeightfield, iWidth, iHeight,
                           arrBoundsMin, arrBoundsMax, flCellSize, .12F) ||
      !rcRasterizeTriangles(&theContext, vecVertices.data(),
                            static_cast<int>(vecVertices.size() / 3),
                            vecTriangles.data(), theField.vecArea.data(),
                            theField.iTotalTriangles, *theField.pHeightfield))
      throw std::runtime_error { "Recast native terrain rasterization failed" };
   theField.iCells = rcGetHeightFieldSpanCount(&theContext, *theField.pHeightfield);
   theField.vecVertices = std::move(vecVertices);
   theField.vecTriangles = std::move(vecTriangles);
   return theField;
}

void DrawField(RecastField const& theField, ImVec2 const aOrigin, ImVec2 const aSize) {
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, IM_COL32(6, 13, 28, 255), 15.0F);
   pDraw->PushClipRect(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, true);
   rcHeightfield const& theH = *theField.pHeightfield;
   float const flSize = std::min(aSize.x / static_cast<float>(theH.width),
                                 aSize.y / static_cast<float>(theH.height)) * .9F;
   ImVec2 const aStart {
      aOrigin.x + (aSize.x - flSize * theH.width) * .5F,
      aOrigin.y + (aSize.y - flSize * theH.height) * .5F
   };
   for(int iZ = 0; iZ < theH.height; ++iZ) {
      for(int iX = 0; iX < theH.width; ++iX) {
         rcSpan const* const pSpan = theH.spans[iX + iZ * theH.width];
         if(!pSpan) continue;
         bool bWalkable = false;
         for(rcSpan const* p = pSpan; p != nullptr; p = p->next)
            bWalkable = bWalkable || p->area == RC_WALKABLE_AREA;
         float const flX = aStart.x + iX * flSize;
         float const flY = aStart.y + iZ * flSize;
         pDraw->AddRectFilled({ flX, flY }, { flX + flSize + .3F, flY + flSize + .3F },
            bWalkable ? IM_COL32(28, 213, 191, 230) : IM_COL32(236, 66, 118, 220));
      }
   }
   pDraw->PopClipRect();
}

/**
\brief Draws the Detour-computed path and animated agent on the Recast raster.
\details Projects only the Detour path, without implementing an alternative pathfinder.
\param theField The Recast field and Detour path.
\param aOrigin Viewport position.
\param aSize Viewport size.
\param flPhase Fraction of the route traversed by the agent.
\param bObstacle Whether to display the excluded obstacle region.
\returns Nothing.
\throw Nothing intentionally.
*/
void DrawRoute(RecastField const& theField, ImVec2 const aOrigin, ImVec2 const aSize,
               float const flPhase, bool const bObstacle) {
   rcHeightfield const& theH = *theField.pHeightfield;
   float const flSize = std::min(aSize.x / static_cast<float>(theH.width),
      aSize.y / static_cast<float>(theH.height)) * .9F;
   ImVec2 const aStart {
      aOrigin.x + (aSize.x - flSize * theH.width) * .5F,
      aOrigin.y + (aSize.y - flSize * theH.height) * .5F
   };
   auto const Project = [&](std::array<float, 3> const& arrPoint) -> ImVec2 {
      return {aStart.x + (arrPoint[0] - theH.bmin[0]) / theH.cs * flSize,
              aStart.y + (arrPoint[2] - theH.bmin[2]) / theH.cs * flSize};
   };
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->PushClipRect(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, true);
   if(bObstacle) {
      ImVec2 const aCenter = Project({8.0F, 0.0F, 5.0F});
      pDraw->AddCircleFilled(aCenter, 1.8F / theH.cs * flSize, IM_COL32(8, 14, 29, 235));
      pDraw->AddCircle(aCenter, 1.8F / theH.cs * flSize, IM_COL32(255, 91, 131, 255), 0, 3.0F);
   }
   auto const& vecPoints = theField.theRoute.vecPoints;
   if(vecPoints.size() >= 2) {
      float flTotal = 0.0F;
      for(std::size_t u = 1; u < vecPoints.size(); ++u)
         flTotal += std::hypot(vecPoints[u][0] - vecPoints[u - 1][0],
            vecPoints[u][2] - vecPoints[u - 1][2]);
      ImVec2 aAgent = Project(vecPoints.front());
      float flRemaining = flTotal * std::clamp(flPhase, 0.0F, 1.0F);
      for(std::size_t u = 1; u < vecPoints.size(); ++u) {
         ImVec2 const aA = Project(vecPoints[u - 1]);
         ImVec2 const aB = Project(vecPoints[u]);
         pDraw->AddLine(aA, aB, IM_COL32(13, 33, 55, 255), 9.0F);
         pDraw->AddLine(aA, aB, IM_COL32(255, 218, 83, 255), 4.0F);
         float const flLength = std::hypot(vecPoints[u][0] - vecPoints[u - 1][0],
            vecPoints[u][2] - vecPoints[u - 1][2]);
         if(flRemaining > 0.0F) {
            float const flT = flLength > 0.0F ? std::min(flRemaining / flLength, 1.0F) : 1.0F;
            aAgent = { aA.x + (aB.x - aA.x) * flT, aA.y + (aB.y - aA.y) * flT };
            flRemaining -= flLength;
         }
      }
      pDraw->AddCircleFilled(Project(vecPoints.front()), 9.0F, IM_COL32(80, 240, 172, 255));
      pDraw->AddCircleFilled(Project(vecPoints.back()), 9.0F, IM_COL32(255, 104, 142, 255));
      pDraw->AddCircleFilled(aAgent, 12.0F, IM_COL32(255, 255, 255, 255));
      pDraw->AddCircleFilled(aAgent, 7.0F, IM_COL32(56, 116, 255, 255));
   }
   pDraw->PopClipRect();
}

bool Verify() {
   RecastField const theLow = BuildField(15.0F, 1.3F);
   RecastField const theHigh = BuildField(75.0F, 1.3F);
   bool const bMesh = theLow.iTotalTriangles == 8000 &&
                      theHigh.iTotalTriangles == theLow.iTotalTriangles;
   bool const bLowRaster = theLow.iCells > 0;
   bool const bHighRaster = theHigh.iCells > 0;
   bool const bSlopeResponse =
      theLow.iWalkableTriangles > 0 &&
      theHigh.iWalkableTriangles > theLow.iWalkableTriangles &&
      theHigh.iWalkableTriangles <= theHigh.iTotalTriangles;
   RecastField theRouteField = BuildField(70.0F, 0.5F);
   theRouteField.theRoute = BuildNavigationRoute(theRouteField.vecVertices,
      theRouteField.vecTriangles, 70.0F, 0.5F, false);
   bool const bDetour = theRouteField.theRoute.bPathComplete &&
      theRouteField.theRoute.vecPoints.size() >= 2;
   // Route samples must follow the detailed navigable surface, rather than
   // presenting only a handful of straight funnel-corner chords.
   bool bSurfaceSampling = bDetour &&
      theRouteField.theRoute.vecPoints.size() >= 40;
   if(bSurfaceSampling) {
      for(std::size_t u = 1; u < theRouteField.theRoute.vecPoints.size(); ++u) {
         auto const& arrPrevious = theRouteField.theRoute.vecPoints[u - 1];
         auto const& arrCurrent = theRouteField.theRoute.vecPoints[u];
         float const flStep = std::hypot(arrCurrent[0] - arrPrevious[0],
            arrCurrent[2] - arrPrevious[2]);
         if(flStep > 0.5F) {
            bSurfaceSampling = false;
            break;
         }
      }
   }
   int const iCorridorTriangles = static_cast<int>(std::count_if(
      theRouteField.theRoute.vecNavigationTriangles.begin(),
      theRouteField.theRoute.vecNavigationTriangles.end(),
      [](NavigationMeshTriangle const& theTriangle) {
         return theTriangle.bCorridor;
      }));
   bool const bNavMeshCorridor = theRouteField.theRoute.iCorridor > 0 &&
      !theRouteField.theRoute.vecNavigationTriangles.empty() &&
      iCorridorTriangles > 0 &&
      iCorridorTriangles <= static_cast<int>(
         theRouteField.theRoute.vecNavigationTriangles.size());
   bool const bPassed = bMesh && bLowRaster && bHighRaster &&
      bSlopeResponse && bDetour && bSurfaceSampling && bNavMeshCorridor;
   std::println("SHOWCASE|CHECK|mesh-topology|{}|trianglesLow={} trianglesHigh={} expected=8000",
      bMesh ? "PASS" : "FAIL", theLow.iTotalTriangles, theHigh.iTotalTriangles);
   std::println("SHOWCASE|CHECK|low-slope-raster|{}|spans={} threshold=nonzero",
      bLowRaster ? "PASS" : "FAIL", theLow.iCells);
   std::println("SHOWCASE|CHECK|high-slope-raster|{}|spans={} threshold=nonzero",
      bHighRaster ? "PASS" : "FAIL", theHigh.iCells);
   std::println("SHOWCASE|CHECK|slope-response|{}|lowWalkable={} highWalkable={} triangles={}",
      bSlopeResponse ? "PASS" : "FAIL", theLow.iWalkableTriangles,
      theHigh.iWalkableTriangles, theHigh.iTotalTriangles);
   std::println("SHOWCASE|CHECK|detour-route|{}|polygons={} points={} complete={}",
      bDetour ? "PASS" : "FAIL", theRouteField.theRoute.iPolygons,
      theRouteField.theRoute.vecPoints.size(), theRouteField.theRoute.bPathComplete);
   std::println("SHOWCASE|CHECK|detour-mesh-corridor|{}|detailTriangles={} corridorTriangles={} corridorPolygons={}",
      bNavMeshCorridor ? "PASS" : "FAIL",
      theRouteField.theRoute.vecNavigationTriangles.size(),
      iCorridorTriangles, theRouteField.theRoute.iCorridor);
   std::println("SHOWCASE|CHECK|detour-surface-sampling|{}|points={} max-horizontal-step=0.5m",
      bSurfaceSampling ? "PASS" : "FAIL",
      theRouteField.theRoute.vecPoints.size());
   std::println("SHOWCASE|RESULT|{}|Recast rasterization and Detour surface pathfinding",
                bPassed ? "PASS" : "FAIL");
   return bPassed;
}

void Interactive() {
   OpenGLShowcase aApp { "BuildEngine | NAVIGATION REACTOR - Mesa Expedition",
      ADECC_MESA_OPENGL_DLL, 1540, 900 };
   TerrainGL theRenderer;
   std::string const strGLVersion = aApp.VersionString();
   float flSlope = 70.0F;
   float flRelief = 0.50F;
   bool bObstacle = true;
   bool bAnimate = true;
   bool bView3D = true;
   bool bWireframe = false;
   bool bShowNavMesh = true;
   bool bShowCorridor = true;
   TerrainCamera theCamera;
   RecastField theField = BuildField(flSlope, flRelief);
   std::string strRouteError;
   auto const Refresh = [&]() {
      RecastField theNew = BuildField(flSlope, flRelief);
      theNew.theRoute = BuildNavigationRoute(theNew.vecVertices,
         theNew.vecTriangles, flSlope, flRelief, bObstacle);
      theRenderer.SetTerrain(theNew.vecVertices, theNew.vecTriangles, theNew.vecArea);
      theRenderer.SetRoute(theNew.theRoute.vecPoints);
      theRenderer.SetNavigationMesh(theNew.theRoute.vecNavigationTriangles);
      theField = std::move(theNew);
      strRouteError.clear();
   };
   try {
      Refresh();
   }
   catch(std::exception const& theError) {
      strRouteError = theError.what();
   }
   auto const theStart = std::chrono::steady_clock::now();
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0.0F, 0.0F});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("NAVIGATION REACTOR", nullptr,
         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      OpenGLShowcase::Header("NAVIGATION REACTOR / MESA EXPEDITION",
         "Mesa OpenGL + GLEW / Recast + Detour / native GPU depth buffer");
      ImGui::TextColored({.48F, .95F, .83F, 1.0F},
         "TRIANGLES %d / %d   |   VOXEL SPANS %d   |   NAV POLYGONS %d   |   WAYPOINTS %d",
         theField.iWalkableTriangles, theField.iTotalTriangles, theField.iCells,
         theField.theRoute.iPolygons, static_cast<int>(theField.theRoute.vecPoints.size()));
      bool bChange = ImGui::SliderFloat("Agent maximum slope", &flSlope, 5.0F, 80.0F, "%.1f deg");
      bChange = ImGui::SliderFloat("Terrain relief", &flRelief, .20F, 1.60F, "%.2fx") || bChange;
      bChange = ImGui::Checkbox("Central obstacle", &bObstacle) || bChange;
      ImGui::SameLine();
      ImGui::Checkbox("Animate agent", &bAnimate);
      ImGui::SameLine();
      bChange = ImGui::Button("REBUILD NAVMESH") || bChange;
      ImGui::Checkbox("3D terrain", &bView3D);
      ImGui::SameLine();
      ImGui::Checkbox("Mesh wireframe", &bWireframe);
      ImGui::SameLine();
      ImGui::Checkbox("NavMesh polygons", &bShowNavMesh);
      ImGui::SameLine();
      ImGui::Checkbox("Detour corridor", &bShowCorridor);
      if(bView3D) {
         ImGui::SetNextItemWidth(230.0F);
         ImGui::SliderFloat("Orbit azimuth", &theCamera.flAzimuth, -3.14F, 3.14F, "%.2f rad");
         ImGui::SameLine();
         ImGui::SetNextItemWidth(230.0F);
         ImGui::SliderFloat("Orbit elevation", &theCamera.flElevation, 0.18F, 1.50F, "%.2f rad");
         ImGui::SameLine();
         ImGui::SetNextItemWidth(230.0F);
         ImGui::SliderFloat("Camera distance", &theCamera.flDistance, 12.0F, 48.0F, "%.1f");
      }
      if(bChange) {
         try {
            Refresh();
         }
         catch(std::exception const& theError) {
            theField.theRoute = {};
            strRouteError = theError.what();
         }
      }
      if(!strRouteError.empty())
         ImGui::TextColored({1.0F, .4F, .45F, 1.0F}, "Navigation error: %s", strRouteError.c_str());
      ImGui::TextColored({.40F, .92F, .78F, 1.0F}, "TEAL: traversable");
      ImGui::SameLine();
      ImGui::TextColored({.97F, .37F, .54F, 1.0F}, "MAGENTA: blocked or steep");
      ImGui::SameLine();
      ImGui::TextColored({1.0F, .85F, .37F, 1.0F}, "GOLD: Detour route");
      ImGui::Text("Routing: %s   |   corridor polygons: %d",
         theField.theRoute.bPathComplete ? "COMPLETE" : "NO COMPLETE PATH",
         theField.theRoute.iCorridor);
      ImGui::TextDisabled("TEAL overlay: real Detour detail mesh | ORANGE overlay: findPath polygon corridor.");
      ImGui::TextDisabled("GPU depth-tested geometry comes from Recast input triangles; route from Detour.");
      ImGui::TextDisabled("OpenGL runtime: %s", strGLVersion.c_str());
      ImGui::TextDisabled("Drag the 3D view to orbit; use the mouse wheel to zoom.");
      ImVec2 const aOrigin = ImGui::GetCursorScreenPos();
      ImVec2 const aSize = ImGui::GetContentRegionAvail();
      ImVec2 const aPanel {aSize.x, std::max(260.0F, aSize.y - 10.0F)};
      // The transparent capture surface enables orbit and zoom without
      // allowing GUI controls above the viewport to hijack camera input.
      ImGui::InvisibleButton("terrain viewport", aPanel,
         ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
      if(bView3D && ImGui::IsItemHovered()) {
         ImGuiIO const& aIo = ImGui::GetIO();
         if(ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            theCamera.flAzimuth += aIo.MouseDelta.x * 0.007F;
            theCamera.flElevation = std::clamp(
               theCamera.flElevation + aIo.MouseDelta.y * 0.005F, 0.18F, 1.50F);
         }
         if(aIo.MouseWheel != 0.0F)
            theCamera.flDistance = std::clamp(
               theCamera.flDistance - aIo.MouseWheel * 1.5F, 12.0F, 48.0F);
      }
      float flPhase = 0.0F;
      if(bAnimate) {
         float const flElapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - theStart).count();
         flPhase = std::fmod(flElapsed / 16.0F, 1.0F);
      }
      if(bView3D) {
         GLuint const uTexture = theRenderer.Render(
            static_cast<int>(aPanel.x), static_cast<int>(aPanel.y),
            theCamera, flPhase, bWireframe, bShowNavMesh, bShowCorridor);
         // The OpenGL FBO origin is bottom-left; ImGui UVs invert it.
         ImGui::SetCursorScreenPos(aOrigin);
         ImGui::Image(static_cast<ImTextureID>(uTexture), aPanel,
            ImVec2 {0.0F, 1.0F}, ImVec2 {1.0F, 0.0F});
      }
      else {
         DrawField(theField, aOrigin, aPanel);
         DrawRoute(theField, aOrigin, aPanel, flPhase, bObstacle);
      }
      ImGui::End();
      aApp.EndFrame();
   }
}
}

int main(int const iArgc, char const* const* const pszArgv) {
   try {
      if(iArgc > 1 && std::string_view { pszArgv[1] } == "--verify-gl") {
         OpenGLShowcase aGL { "Navigation Reactor GPU Verification",
            ADECC_MESA_OPENGL_DLL, 640, 480 };
         TerrainGL theRenderer;
         RecastField const theField = BuildField(70.0F, 0.5F);
         theRenderer.SetTerrain(theField.vecVertices, theField.vecTriangles, theField.vecArea);
         NavigationRoute const theRoute = BuildNavigationRoute(
            theField.vecVertices, theField.vecTriangles, 70.0F, 0.5F, false);
         theRenderer.SetNavigationMesh(theRoute.vecNavigationTriangles);
         theRenderer.SetRoute(theRoute.vecPoints);
         GLuint const uTexture = theRenderer.Render(640, 480, TerrainCamera {}, 0.0F, false);
         bool const bValid = uTexture != 0 && theRoute.bPathComplete &&
            !theRoute.vecNavigationTriangles.empty() && glGetError() == GL_NO_ERROR;
         std::println("SHOWCASE|CHECK|mesa-gpu|{}|OpenGL={} texture={}",
            bValid ? "PASS" : "FAIL", aGL.VersionString(), uTexture);
         std::println("SHOWCASE|RESULT|{}|Managed Mesa GL context and terrain render",
            bValid ? "PASS" : "FAIL");
         return bValid ? 0 : 1;
      }
      if(iArgc > 1 && std::string_view { pszArgv[1] } == "--verify")
         return Verify() ? 0 : 1;
      Interactive();
      return 0;
   }
   catch(std::exception const& theException) {
      std::println(stderr, "SHOWCASE|ERROR|{}", theException.what());
      return 2;
   }
}
