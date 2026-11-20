#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#include <hnswlib/hnswlib.h>
#include <igl/per_vertex_normals.h>
#include <Eigen/Core>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <numbers>
#include <print>
#include <string_view>
#include <vector>

/**
\brief Generates a deterministic mathematical nebula and a native HNSW index.
\details Every drawn star belongs to a real three-dimensional feature vector.
         Nearest-neighbour highlights are computed by hnswlib, not by the renderer.
*/
class Galaxy final {
public:
   static constexpr int iColumns = 37;
   static constexpr int iRows = 23;
   static constexpr int iCount = iColumns * iRows;
   struct Point {
      std::array<float, 3> arrFeature {};
      float flHeight {};
   };
private:
   hnswlib::L2Space theSpace { 3 };
   hnswlib::HierarchicalNSW<float> theIndex { &theSpace, static_cast<std::size_t>(iCount), 12, 96 };
   std::array<Point, iCount> arrPoints {};
   Eigen::Matrix<double, Eigen::Dynamic, 3> theNormals;
   std::array<bool, iCount> arrHighlighted {};

public:
   Galaxy() {
      Eigen::Matrix<double, Eigen::Dynamic, 3> theVertices(iCount, 3);
      Eigen::Matrix<int, Eigen::Dynamic, 3> theFaces((iColumns - 1) * (iRows - 1) * 2, 3);
      for(int iY = 0; iY < iRows; ++iY) {
         for(int iX = 0; iX < iColumns; ++iX) {
            int const i = iY * iColumns + iX;
            double const flX = static_cast<double>(iX) / (iColumns - 1) * 2.0 - 1.0;
            double const flY = static_cast<double>(iY) / (iRows - 1) * 2.0 - 1.0;
            double const flRadius = std::hypot(flX, flY);
            double const flHeight = 0.36 * std::sin(11.0 * flRadius - 2.2) * std::exp(-1.7 * flRadius)
                                   + 0.15 * std::cos(8.0 * flX + 5.0 * flY);
            arrPoints[i].arrFeature = { static_cast<float>(flX), static_cast<float>(flY), static_cast<float>(flHeight) };
            arrPoints[i].flHeight = static_cast<float>(flHeight);
            theVertices.row(i) << flX, flY, flHeight;
            theIndex.addPoint(arrPoints[i].arrFeature.data(), static_cast<hnswlib::labeltype>(i));
         }
      }
      int iFace = 0;
      for(int iY = 0; iY + 1 < iRows; ++iY) {
         for(int iX = 0; iX + 1 < iColumns; ++iX) {
            int const iA = iY * iColumns + iX;
            int const iB = iA + 1;
            int const iC = iA + iColumns;
            int const iD = iC + 1;
            theFaces.row(iFace++) << iA, iB, iC;
            theFaces.row(iFace++) << iB, iD, iC;
         }
      }
      igl::per_vertex_normals(theVertices, theFaces, theNormals);
      Search({ 0.0F, 0.0F, 0.0F }, 24);
   }

   /**
   \brief Recomputes the nearest stars in the installed hnswlib implementation.
   \details Uses three-dimensional Euclidean feature distance.
   \param arrQuery Query vector in the feature space.
   \param iNeighbors Maximum number of results.
   \returns Number of distinct neighbours selected.
   \throw std::exception If the underlying ANN index fails.
   */
   int Search(std::array<float, 3> const& arrQuery, int const iNeighbors) {
      arrHighlighted.fill(false);
      auto theMatches = theIndex.searchKnn(arrQuery.data(), static_cast<std::size_t>(iNeighbors));
      int iFound = 0;
      while(!theMatches.empty()) {
         auto const [flDistance, uLabel] = theMatches.top();
         (void)flDistance;
         theMatches.pop();
         if(uLabel < static_cast<hnswlib::labeltype>(iCount)) {
            arrHighlighted[static_cast<std::size_t>(uLabel)] = true;
            ++iFound;
         }
      }
      return iFound;
   }

   [[nodiscard]] Point const& At(int const i) const { return arrPoints[static_cast<std::size_t>(i)]; }
   [[nodiscard]] bool Highlighted(int const i) const { return arrHighlighted[static_cast<std::size_t>(i)]; }
   [[nodiscard]] double NormalZ(int const i) const { return theNormals(i, 2); }
};

void DrawNebula(Galaxy const& theGalaxy, ImVec2 const aOrigin, ImVec2 const aSize,
                float const flTime, float const flZoom) {
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, IM_COL32(5, 13, 31, 255), 16.0F);
   pDraw->PushClipRect(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, true);
   ImVec2 const aCenter { aOrigin.x + aSize.x * 0.5F, aOrigin.y + aSize.y * 0.52F };
   float const flScale = std::min(aSize.x * .40F, aSize.y * .40F) * flZoom;
   float const flRotation = flTime * .13F;
   float const flCos = std::cos(flRotation), flSin = std::sin(flRotation);
   std::array<ImVec2, Galaxy::iCount> arrScreen {};
   for(int i = 0; i < Galaxy::iCount; ++i) {
      auto const& thePoint = theGalaxy.At(i);
      float const flX = thePoint.arrFeature[0];
      float const flY = thePoint.arrFeature[1];
      float const flR = flX * flCos - flY * flSin;
      float const flS = flX * flSin + flY * flCos;
      arrScreen[static_cast<std::size_t>(i)] = {
         aCenter.x + flScale * (flR + .28F * thePoint.flHeight),
         aCenter.y + flScale * (flS * .68F - .55F * thePoint.flHeight)
      };
   }
   for(int iY = 0; iY < Galaxy::iRows; ++iY) {
      for(int iX = 0; iX < Galaxy::iColumns; ++iX) {
         int const i = iY * Galaxy::iColumns + iX;
         ImVec2 const aP = arrScreen[static_cast<std::size_t>(i)];
         float const flNormal = static_cast<float>(theGalaxy.NormalZ(i));
         int const iLight = std::clamp(static_cast<int>(120.0F + 95.0F * std::abs(flNormal)), 75, 235);
         ImU32 const uLink = IM_COL32(25, iLight, 180, 62);
         if(iX + 1 < Galaxy::iColumns)
            pDraw->AddLine(aP, arrScreen[static_cast<std::size_t>(i + 1)], uLink, 1.0F);
         if(iY + 1 < Galaxy::iRows)
            pDraw->AddLine(aP, arrScreen[static_cast<std::size_t>(i + Galaxy::iColumns)], uLink, 1.0F);
      }
   }
   for(int i = 0; i < Galaxy::iCount; ++i) {
      ImVec2 const aP = arrScreen[static_cast<std::size_t>(i)];
      bool const bSelected = theGalaxy.Highlighted(i);
      if(bSelected) {
         pDraw->AddCircleFilled(aP, 12.0F, IM_COL32(57, 215, 252, 25), 16);
         pDraw->AddCircle(aP, 6.0F + 1.5F * std::sin(flTime * 3.0F), IM_COL32(133, 255, 234, 240), 16, 1.5F);
      }
      else {
         pDraw->AddCircleFilled(aP, 1.8F, IM_COL32(90, 146, 236, 210), 8);
      }
   }
   pDraw->PopClipRect();
}

bool Verify() {
   Galaxy aGalaxy;
   int const iFound = aGalaxy.Search({0.0F, 0.0F, 0.0F}, 21);
   int iActual = 0;
   bool bNormals = true;
   for(int i = 0; i < Galaxy::iCount; ++i) {
      iActual += aGalaxy.Highlighted(i) ? 1 : 0;
      bNormals = bNormals && std::isfinite(aGalaxy.NormalZ(i));
   }
   bool const bPassed = iFound == 21 && iActual == 21 && bNormals;
   std::println("SHOWCASE|CHECK|ann-neighbours|{}|selected={} expected=21", bPassed ? "PASS" : "FAIL", iActual);
   std::println("SHOWCASE|RESULT|{}|hnswlib nearest neighbours and libigl mesh normals", bPassed ? "PASS" : "FAIL");
   return bPassed;
}

void Interactive() {
   Galaxy aGalaxy;
   NativeShowcase aApp { "BuildEngine | VECTOR GALAXY - Mathematical Discovery", 1560, 920 };
   float arrQuery[3] { 0.0F, 0.0F, 0.0F };
   int iNeighbors = 30;
   int iFound = aGalaxy.Search({arrQuery[0], arrQuery[1], arrQuery[2]}, iNeighbors);
   float flTime = 0.0F;
   float flZoom = 1.0F;
   bool bPlaying = true;
   while(aApp.Running()) {
      aApp.BeginFrame();
      if(bPlaying) flTime += ImGui::GetIO().DeltaTime;
      ImGui::SetNextWindowPos({ 0, 0 });
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("VECTOR GALAXY", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("VECTOR GALAXY / THE SHAPE OF SIMILARITY",
          "851 indexed stars  /  hnswlib nearest neighbours  /  libigl mesh normals  /  native C++23");
      ImGui::TextColored({ .48F, .94F, .87F, 1.0F },
                         "DISCOVERY ENGINE     %d near stars     %d mesh vertices", iFound, Galaxy::iCount);
      bool bChanged = ImGui::SliderFloat3("Search position (x, y, z)", arrQuery, -1.0F, 1.0F);
      bChanged = ImGui::SliderInt("Nearest stars", &iNeighbors, 3, 110) || bChanged;
      if(ImGui::Button("Find similar stars") || bChanged)
         iFound = aGalaxy.Search({ arrQuery[0], arrQuery[1], arrQuery[2] }, iNeighbors);
      ImGui::SameLine();
      if(ImGui::Button(bPlaying ? "Freeze orbital motion" : "Resume orbital motion")) bPlaying = !bPlaying;
      ImGui::SameLine();
      if(ImGui::Button("Reset")) {
         arrQuery[0] = arrQuery[1] = arrQuery[2] = 0.0F;
         flTime = 0.0F;
         iFound = aGalaxy.Search({ 0.0F, 0.0F, 0.0F }, iNeighbors);
      }
      ImGui::SliderFloat("Telescope zoom", &flZoom, .55F, 1.65F);
      ImGui::TextDisabled("The glowing stars are real HNSW search results. Mesh shading derives from Eigen/libigl normals.");
      ImVec2 const aOrigin = ImGui::GetCursorScreenPos();
      ImVec2 const aSize = ImGui::GetContentRegionAvail();
      DrawNebula(aGalaxy, aOrigin, {aSize.x, std::max(300.0F, aSize.y - 8.0F)}, flTime, flZoom);
      ImGui::End();
      aApp.EndFrame();
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
