#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#include <Recast.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <memory>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string_view>
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
   int iWalkableTriangles {};
   int iTotalTriangles {};
   int iCells {};
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
   bool const bPassed = bMesh && bLowRaster && bHighRaster && bSlopeResponse;
   std::println("SHOWCASE|CHECK|mesh-topology|{}|trianglesLow={} trianglesHigh={} expected=8000",
      bMesh ? "PASS" : "FAIL", theLow.iTotalTriangles, theHigh.iTotalTriangles);
   std::println("SHOWCASE|CHECK|low-slope-raster|{}|spans={} threshold=nonzero",
      bLowRaster ? "PASS" : "FAIL", theLow.iCells);
   std::println("SHOWCASE|CHECK|high-slope-raster|{}|spans={} threshold=nonzero",
      bHighRaster ? "PASS" : "FAIL", theHigh.iCells);
   std::println("SHOWCASE|CHECK|slope-response|{}|lowWalkable={} highWalkable={} triangles={}",
      bSlopeResponse ? "PASS" : "FAIL", theLow.iWalkableTriangles,
      theHigh.iWalkableTriangles, theHigh.iTotalTriangles);
   std::println("SHOWCASE|RESULT|{}|Recast walkability classification and heightfield rasterization",
                bPassed ? "PASS" : "FAIL");
   return bPassed;
}

void Interactive() {
   NativeShowcase aApp { "BuildEngine | NAVIGATION REACTOR - Terrain Intelligence", 1540, 900 };
   float flSlope = 36.0F;
   float flRelief = 1.25F;
   RecastField theField = BuildField(flSlope, flRelief);
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0.0F, 0.0F});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("NAVIGATION REACTOR", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("NAVIGATION REACTOR / TERRAIN INTELLIGENCE",
                             "Live native Recast slope classification and voxelized surface spans");
      ImGui::TextColored({.48F, .95F, .83F, 1.0F},
         "WALKABLE TRIANGLES: %d / %d     RASTERIZED SPANS: %d",
         theField.iWalkableTriangles, theField.iTotalTriangles, theField.iCells);
      bool bChange = ImGui::SliderFloat("Agent maximum slope", &flSlope, 5.0F, 80.0F, "%.1f deg");
      bChange = ImGui::SliderFloat("Terrain relief", &flRelief, .25F, 2.25F, "%.2fx") || bChange;
      if(bChange || ImGui::Button("Rebuild navigation field"))
         theField = BuildField(flSlope, flRelief);
      ImGui::TextColored({.18F, .89F, .75F, 1.0F}, "GREEN: Recast walkable surface");
      ImGui::SameLine();
      ImGui::TextColored({.97F, .39F, .51F, 1.0F}, "PINK: steep or blocked span");
      ImGui::TextDisabled("A genuine Recast raster, not a pre-rendered texture. Lower the slope to close dangerous routes.");
      ImVec2 const aOrigin = ImGui::GetCursorScreenPos();
      ImVec2 const aSize = ImGui::GetContentRegionAvail();
      DrawField(theField, aOrigin, {aSize.x, std::max(260.0F, aSize.y - 10.0F)});
      ImGui::End();
      aApp.EndFrame();
   }
}
}

int main(int const iArgc, char const* const* const pszArgv) {
   try {
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
