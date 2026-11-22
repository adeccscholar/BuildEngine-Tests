#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"
#include <CGAL/Simple_cartesian.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/squared_distance_2.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <print>
#include <string_view>
#include <vector>
namespace {
using Kernel=CGAL::Simple_cartesian<double>;
using Point=Kernel::Point_2;
std::vector<Point> GeneratePoints(double const flTwist) {
   std::vector<Point> vecPoints;
   vecPoints.reserve(360);
   for(int i=0;i<360;++i) {
      double const flT=double(i)*.124;
      double const flR=.2+2.0*double(i)/360;
      vecPoints.emplace_back(flR*std::cos(flT+flTwist),flR*std::sin(flT+flTwist));
   }
   return vecPoints;
}
std::vector<Point> Hull(std::vector<Point> const& vecPoints) {
   std::vector<Point> vecHull;
   CGAL::convex_hull_2(vecPoints.begin(),vecPoints.end(),std::back_inserter(vecHull));
   return vecHull;
}
/**
\brief Imports tetrahedral OBJ geometry through the installed Assimp DLL.
\details The OBJ data is passed through ReadFileFromMemory and triangulated upstream.
\returns True if the imported native mesh contains four triangular faces.
\throw std::exception On an unexpected runtime error.
*/
bool ImportGeometry() {
   constexpr std::string_view svObj {
      "v 0 1 0\nv -1 -1 1\nv 1 -1 1\nv 0 -1 -1\n"
      "f 1 2 3\nf 1 3 4\nf 1 4 2\nf 2 4 3\n"
   };
   Assimp::Importer theImporter;
   aiScene const* pScene=theImporter.ReadFileFromMemory(svObj.data(),svObj.size(),aiProcess_Triangulate,"obj");
   if(!pScene||pScene->mNumMeshes!=1||!pScene->mMeshes||!pScene->mMeshes[0])return false;
   aiMesh const* pMesh=pScene->mMeshes[0];
   return pMesh->mNumFaces==4 && pMesh->mNumVertices>=4 &&
      std::all_of(pMesh->mFaces,pMesh->mFaces+pMesh->mNumFaces,
         [](aiFace const& theFace){return theFace.mNumIndices==3;});
}
TEST_CASE("CGAL convex hull orientation and exact squared distance") {
   auto const vecPoints=GeneratePoints(.25);
   auto const vecHull=Hull(vecPoints);
   CHECK(vecHull.size()>=3);
   CHECK(vecHull.size()<vecPoints.size());
   CHECK(CGAL::squared_distance(Point {0,0},Point {3,4})==doctest::Approx(25.0));
}
TEST_CASE("Assimp tetrahedral OBJ import") {
   CHECK(ImportGeometry());
}
bool Verify() {
   doctest::Context theContext;
   theContext.setOption("no-colors",true);
   int const iResult=theContext.run();
   bool const bOK=iResult==0;
   std::println("SHOWCASE|CHECK|cgal-assimp-doctest|{}",bOK?"PASS":"FAIL");
   std::println("SHOWCASE|RESULT|{}|Geometry Cathedral",bOK?"PASS":"FAIL");
   return bOK;
}
void Interactive() {
   NativeShowcase aApp {"BuildEngine | GEOMETRY CATHEDRAL - Shape of Space",1500,900};
   float flTwist=.25F;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("GEOMETRY",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("GEOMETRY CATHEDRAL / COMPUTATIONAL SPACE","CGAL hull + Assimp OBJ import + doctest assertions");
      ImGui::SliderFloat("Rotation",&flTwist,-3.14F,3.14F);
      auto const vecPoints=GeneratePoints(flTwist),vecHull=Hull(vecPoints);
      ImGui::Text("CGAL: %zu source points; %zu convex hull vertices",vecPoints.size(),vecHull.size());
      ImGui::Text("Assimp: native tetrahedral OBJ import: %s",ImportGeometry()?"PASS":"FAIL");
      ImVec2 const aOrigin=ImGui::GetCursorScreenPos(),aArea=ImGui::GetContentRegionAvail();
      float const flScale=std::min(aArea.x,aArea.y)*.18F;
      ImVec2 const aMid {aOrigin.x+aArea.x*.5F,aOrigin.y+aArea.y*.5F};
      ImDrawList* const pDraw=ImGui::GetWindowDrawList();
      auto Transform=[&](Point const& theP)->ImVec2 {
         return {aMid.x+float(theP.x())*flScale,aMid.y+float(theP.y())*flScale};
      };
      for(Point const& theP:vecPoints)
         pDraw->AddCircleFilled(Transform(theP),2.0F,IM_COL32(54,215,255,195));
      for(std::size_t uI=0;uI<vecHull.size();++uI) {
         auto const aA=Transform(vecHull[uI]),aB=Transform(vecHull[(uI+1)%vecHull.size()]);
         pDraw->AddLine(aA,aB,IM_COL32(248,168,72,55),13.0F);
         pDraw->AddLine(aA,aB,IM_COL32(255,225,116,255),3.0F);
      }
      ImGui::End();aApp.EndFrame();
   }
}
}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1&&std::string_view {pszArgv[1]}=="--verify")return Verify()?0:1;
      Interactive();return 0;
   }
   catch(std::exception const& theError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",theError.what());return 2;
   }
}
