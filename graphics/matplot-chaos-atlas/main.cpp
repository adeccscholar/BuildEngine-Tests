#include "../common/GraphicsDllImports.h"
#include "../common/VectorStage.h"
#include "../common/NativeShowcase.h"
#include <SDL.h>
#include <matplot/matplot.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct Point { double flX; double flY; };
struct Scene {
   std::string strTitle;
   SDL_Color theColor;
   bool bPolyline;
   std::vector<Point> vecPoints;
};

[[nodiscard]] std::vector<Scene> BuildScenes() {
   std::array<std::string,6> const arrNames {
      "LISSAJOUS", "CLIFFORD", "DE JONG", "HARMONIC BLOOM",
      "BUTTERFLY", "ORBITAL RESONANCE"
   };
   std::array<SDL_Color,6> const arrColors {{
      {22,240,223,255}, {247,107,204,255}, {246,186,57,255},
      {126,143,255,255}, {179,250,112,255}, {211,116,254,255}
   }};
   auto const vecSamples = matplot::linspace(0.0,48.0*std::numbers::pi,12000);
   std::vector<Scene> vecScenes;
   for(int iScene=0;iScene<6;++iScene) {
      Scene aScene { arrNames[iScene],arrColors[iScene],iScene!=1&&iScene!=2,{} };
      aScene.vecPoints.reserve(vecSamples.size());
      double flX=.1,flY=.1;
      for(double const flT : vecSamples) {
         double flNewX{},flNewY{};
         switch(iScene) {
            case 0:
               flNewX=std::sin(3.0*flT+.35*std::cos(flT*.07));
               flNewY=std::sin(4.0*flT+.38*std::sin(flT*.09));
               break;
            case 1:
               flNewX=.5*(std::sin(1.7*flY)+.93*std::cos(1.7*flX));
               flNewY=.5*(std::sin(-1.8*flX)+.72*std::cos(-1.8*flY));
               break;
            case 2:
               flNewX=.5*(std::sin(1.4*flY)-std::cos(-2.3*flX));
               flNewY=.5*(std::sin(2.4*flX)-std::cos(-2.1*flY));
               break;
            case 3: {
               double const flRadius=.48+.40*std::sin(11.0*flT)+.08*std::cos(23.0*flT);
               flNewX=flRadius*std::cos(flT);flNewY=flRadius*std::sin(flT);
               break;
            }
            case 4: {
               double const flRadius=std::exp(std::sin(flT))-2.0*std::cos(4.0*flT)+
                  std::pow(std::sin((2.0*flT-std::numbers::pi)/24.0),5.0);
               flNewX=.25*flRadius*std::sin(flT);
               flNewY=.25*flRadius*std::cos(flT);
               break;
            }
            default:
               flNewX=.52*std::cos(5.0*flT)+.29*std::cos(11.0*flT);
               flNewY=.52*std::sin(7.0*flT)-.29*std::sin(13.0*flT);
               break;
         }
         flX=flNewX;flY=flNewY;
         if(!std::isfinite(flX)||!std::isfinite(flY))
            throw std::runtime_error{"Nonfinite numeric sample"};
         aScene.vecPoints.push_back({flX,flY});
      }
      vecScenes.push_back(std::move(aScene));
   }
   return vecScenes;
}

void Draw(SDL_Renderer* const pRenderer, Scene const& aScene,
          SDL_Rect const& aArea,double const flZoom,double const flAngle,
          std::size_t const uVisible) {
   SDL_SetRenderDrawColor(pRenderer,10,21,47,255);
   SDL_RenderFillRect(pRenderer,&aArea);
   SDL_RenderSetClipRect(pRenderer,&aArea);
   SDL_SetRenderDrawBlendMode(pRenderer,SDL_BLENDMODE_BLEND);
   SDL_SetRenderDrawColor(pRenderer,aScene.theColor.r,aScene.theColor.g,
                          aScene.theColor.b,aScene.bPolyline?166:145);
   double const flC=std::cos(flAngle),flS=std::sin(flAngle);
   double const flScale=.43*std::min(aArea.w,aArea.h)*flZoom;
   SDL_Point aPrevious{};bool bPrevious=false;
   for(std::size_t uI=0;uI<std::min(uVisible,aScene.vecPoints.size());++uI) {
      Point const& aPoint=aScene.vecPoints[uI];
      double const flX=aPoint.flX*flC-aPoint.flY*flS;
      double const flY=aPoint.flX*flS+aPoint.flY*flC;
      SDL_Point const aCurrent {
         static_cast<int>(aArea.x+aArea.w*.5+flX*flScale),
         static_cast<int>(aArea.y+aArea.h*.5+flY*flScale)
      };
      if(aScene.bPolyline&&bPrevious) {
         if(std::abs(aCurrent.x-aPrevious.x)<aArea.w/3 &&
            std::abs(aCurrent.y-aPrevious.y)<aArea.h/3)
            SDL_RenderDrawLine(pRenderer,aPrevious.x,aPrevious.y,
                               aCurrent.x,aCurrent.y);
      }
      else SDL_RenderDrawPoint(pRenderer,aCurrent.x,aCurrent.y);
      aPrevious=aCurrent;bPrevious=true;
   }
   SDL_RenderSetClipRect(pRenderer,nullptr);
}

void Export(std::vector<Scene> const& vecScenes) {
   std::filesystem::create_directories("artifacts");
   for(std::size_t uIndex=0;uIndex<vecScenes.size();++uIndex) {
      Scene const& aScene=vecScenes[uIndex];
      VectorStage aStage {
         std::filesystem::path{"artifacts"}/std::format("scene-{}.svg",uIndex+1),
         aScene.strTitle
      };
      for(std::size_t uI=1;uI<aScene.vecPoints.size();++uI) {
         Point const& a=aScene.vecPoints[uI-1];
         Point const& b=aScene.vecPoints[uI];
         double const flX=640+a.flX*235,flY=414+a.flY*235;
         double const flXX=640+b.flX*235,flYY=414+b.flY*235;
         if(aScene.bPolyline) {
            if(std::abs(flXX-flX)<300&&std::abs(flYY-flY)<300)
               aStage.Line(flX,flY,flXX,flYY,"#52e5ec");
         }
         else aStage.Line(flXX,flYY,flXX+.4,flYY+.4,"#eb88fd");
      }
   }
}

void DrawNativeScene(ImDrawList* const pDraw, Scene const& aScene,
                     ImVec2 const aPos, ImVec2 const aSize,
                     double const flZoom, double const flPhase,
                     std::size_t const uPoints) {
   pDraw->AddRectFilled(aPos,{aPos.x+aSize.x,aPos.y+aSize.y},
                        IM_COL32(9,19,43,255),12.0F);
   pDraw->PushClipRect(aPos,{aPos.x+aSize.x,aPos.y+aSize.y},true);
   float const flScale=std::min(aSize.x,aSize.y)*.42F*static_cast<float>(flZoom);
   float const flC=static_cast<float>(std::cos(flPhase));
   float const flS=static_cast<float>(std::sin(flPhase));
   ImU32 const uColor=IM_COL32(aScene.theColor.r,aScene.theColor.g,
                              aScene.theColor.b,190);
   ImVec2 aPrevious {};
   bool bPrevious {};
   for(std::size_t uI=0;uI<std::min(uPoints,aScene.vecPoints.size());++uI) {
      Point const& aPoint=aScene.vecPoints[uI];
      float const flX=static_cast<float>(aPoint.flX)*flC-
                      static_cast<float>(aPoint.flY)*flS;
      float const flY=static_cast<float>(aPoint.flX)*flS+
                      static_cast<float>(aPoint.flY)*flC;
      ImVec2 const aCurrent {
         aPos.x+aSize.x*.5F+flX*flScale,
         aPos.y+aSize.y*.5F+flY*flScale
      };
      if(aScene.bPolyline&&bPrevious) {
         if(std::abs(aCurrent.x-aPrevious.x)<aSize.x*.3F &&
            std::abs(aCurrent.y-aPrevious.y)<aSize.y*.3F)
            pDraw->AddLine(aPrevious,aCurrent,uColor,1.1F);
      }
      else pDraw->AddCircleFilled(aCurrent,1.25F,uColor,6);
      aPrevious=aCurrent;
      bPrevious=true;
   }
   pDraw->PopClipRect();
}

void Run(std::vector<Scene> const& vecScenes) {
   NativeShowcase aApp{"BuildEngine | Matplot++ Visual Mathematics Gallery"};
   int iSelected=-1;
   bool bPlaying=true;
   bool bProgressive=true;
   float flZoom=1.0F;
   float flSpeed=1.0F;
   double flTime=0.0;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGuiIO const& aIo=ImGui::GetIO();
      if(bPlaying) flTime+=aIo.DeltaTime*flSpeed;
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(aIo.DisplaySize);
      ImGui::Begin("Mathematics gallery",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|
         ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("VISUAL MATHEMATICS GALLERY",
         "Matplot++ 1.2.2  /  six mathematical scenes  /  native ImGui and SDL2");
      if(iSelected<0) {
         ImGui::TextColored({.42F,.94F,.88F,1.0F},
                            "SELECT A MATHEMATICAL WORLD");
         ImGui::SameLine();
         if(ImGui::Button("Export all six SVG files"))
            Export(vecScenes);
         ImVec2 const aAvailable=ImGui::GetContentRegionAvail();
         float const flSpacing=14.0F;
         float const flTileW=std::max(180.0F,(aAvailable.x-2*flSpacing)/3);
         float const flTileH=std::max(165.0F,(aAvailable.y-flSpacing)/2);
         for(int i=0;i<6;++i) {
            if(i%3) ImGui::SameLine(0,flSpacing);
            ImGui::PushID(i);
            ImGui::BeginChild("Math scene", {flTileW,flTileH},ImGuiChildFlags_Borders);
            ImGui::TextColored({.87F,.95F,1.0F,1.0F},"%02d  %s",
                               i+1,vecScenes[i].strTitle.c_str());
            ImVec2 const aPos=ImGui::GetCursorScreenPos();
            ImVec2 const aSize { flTileW-22.0F,flTileH-89.0F };
            DrawNativeScene(ImGui::GetWindowDrawList(),vecScenes[i],
               aPos,aSize,1.0,0.0,vecScenes[i].vecPoints.size()/3);
            ImGui::Dummy(aSize);
            if(ImGui::Button("Explore scene",{-1,31}))
               {iSelected=i;flZoom=1.0F;flTime=0.0;}
            ImGui::EndChild();
            ImGui::PopID();
         }
      }
      else {
         if(ImGui::Button("Back to gallery"))iSelected=-1;
         ImGui::SameLine();
         if(ImGui::Button(bPlaying?"Pause":"Resume"))bPlaying=!bPlaying;
         ImGui::SameLine();
         if(ImGui::Button("Restart"))flTime=0.0;
         ImGui::SameLine();
         if(ImGui::Button("Export SVG"))Export(vecScenes);
         ImGui::SliderFloat("Zoom",&flZoom,.40F,2.25F,"%.2fx");
         ImGui::SameLine();
         ImGui::SetNextItemWidth(190);
         ImGui::SliderFloat("Animation speed",&flSpeed,.1F,3.0F);
         ImGui::Checkbox("Progressive drawing",&bProgressive);
         ImGui::SameLine();
         ImGui::TextColored({.50F,.93F,.88F,1.0F},
                            "%s",vecScenes[iSelected].strTitle.c_str());
         if(ImGui::Button("Previous"))iSelected=(iSelected+5)%6;
         ImGui::SameLine();
         if(ImGui::Button("Next"))iSelected=(iSelected+1)%6;
         ImVec2 const aPos=ImGui::GetCursorScreenPos();
         ImVec2 const aAvailable=ImGui::GetContentRegionAvail();
         std::size_t const uCount=bProgressive?
            std::min<std::size_t>(vecScenes[iSelected].vecPoints.size(),
               80+static_cast<std::size_t>(std::fmod(flTime*1300.0,11920.0))):
            vecScenes[iSelected].vecPoints.size();
         DrawNativeScene(ImGui::GetWindowDrawList(),vecScenes[iSelected],
            aPos,{std::max(200.0F,aAvailable.x),
                  std::max(190.0F,aAvailable.y-8.0F)},
            flZoom,flTime*.09,uCount);
      }
      ImGui::End();
      aApp.EndFrame();
   }
}

int main(int const iArgc,char const* const* const pszArgv) {
   try {
      std::vector<Scene> const vecScenes=BuildScenes();
      bool const bPassed=vecScenes.size()==6&&std::ranges::all_of(vecScenes,
         [](Scene const& aScene){return aScene.vecPoints.size()==12000;});
      std::println("SHOWCASE|CHECK|sampling|{}|Six Matplot++ scenes",bPassed?"PASS":"FAIL");
      if(!bPassed)return 1;
      if(iArgc>1&&std::string_view{pszArgv[1]}=="--verify")return 0;
      if(iArgc>1&&std::string_view{pszArgv[1]}=="--export"){Export(vecScenes);return 0;}
      Run(vecScenes);
      return 0;
   }
   catch(std::exception const& aException) {
      std::println(stderr,"SHOWCASE|ERROR|{}",aException.what());
      return 2;
   }
}
