#include "../common/NativeShowcase.h"
#include "../common/ImGuiVector.h"

#include <array>
#include <cmath>
#include <exception>
#include <numbers>
#include <print>
#include <string_view>

/** \brief Generates real-time procedural ImGui particle geometry and an interactive gallery. */
struct NeonScene {
   char const* szName;
   char const* szDescription;
   ImU32 uColor;
};
std::array<NeonScene, 4> const arrScenes {{
   { "Nebula Vortex", "Spiral turbulence / 680 live particles", IM_COL32(66,226,245,255) },
   { "Quantum Orbit", "Resonant orbitals / phase modulation", IM_COL32(205,119,255,255) },
   { "Wave Interference", "Coupled harmonic fields / 720 links", IM_COL32(81,250,172,255) },
   { "Stellar Bloom", "Expanding radial photon structures", IM_COL32(255,165,103,255) }
}};

void DrawScene(ImDrawList* const pDraw, ImVec2 const aTop, ImVec2 const aSize,
               int const iMode, double const flTime, float const flZoom) {
   ImVec2 const aCenter { aTop.x + aSize.x*.5F, aTop.y + aSize.y*.5F };
   pDraw->AddRectFilled(aTop, {aTop.x+aSize.x,aTop.y+aSize.y},
      IM_COL32(7,16,38,255),12.0F);
   pDraw->PushClipRect(aTop, {aTop.x+aSize.x,aTop.y+aSize.y},true);
   float const flScale=std::min(aSize.x,aSize.y)*.39F*flZoom;
   ImU32 const uColor=arrScenes[iMode].uColor;
   for(int i=0;i<700;++i) {
      double const flU=static_cast<double>(i)/700.0;
      double const flT=flU*16.0*std::numbers::pi;
      double flX{},flY{};
      switch(iMode) {
         case 0: {
            double const flRadius=.05+.92*flU;
            flX=flRadius*std::cos(flT*.84+flTime*.29);
            flY=flRadius*std::sin(flT*.84+flTime*.29);
            break;
            }
         case 1:
            flX=.64*std::cos(flT*.39+flTime*.23)+.24*std::cos(flT*1.17-flTime*.17);
            flY=.60*std::sin(flT*.67-flTime*.22)+.23*std::sin(flT*1.49+flTime*.13);
            break;
         case 2:
            flX=2.0*flU-1.0;
            flY=.56*std::sin(flU*18.0*std::numbers::pi+flTime*1.8)*
                std::cos(flU*7.0*std::numbers::pi-flTime*.41);
            break;
         default: {
            double const flR=.35+.40*std::sin(14.0*flT+flTime*.62);
            flX=flR*std::cos(flT+flTime*.19);
            flY=flR*std::sin(flT+flTime*.19);
            break;
            }
         }
      float const flPX=aCenter.x+static_cast<float>(flX)*flScale;
      float const flPY=aCenter.y+static_cast<float>(flY)*flScale;
      float const flSize=1.0F+static_cast<float>((i%6))*.42F;
      pDraw->AddCircleFilled({flPX,flPY},flSize,uColor,8);
      }
   pDraw->PopClipRect();
}

void RenderApp(bool const bInteractive, int const iFrames=1) {
   NativeShowcase aApp { "BuildEngine | Dear ImGui Neon Laboratory" };
   int iSelected=0;
   bool bPlaying=true;
   float flZoom=1.0F;
   double flTime=0.0;
   int iFrame=0;
   while(aApp.Running() && (bInteractive || iFrame<iFrames)) {
      aApp.BeginFrame();
      ImGuiIO const& aIo=ImGui::GetIO();
      if(bPlaying) flTime+=aIo.DeltaTime;
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(aIo.DisplaySize);
      ImGui::Begin("Neon control room",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|
         ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoBringToFrontOnFocus);
      NativeShowcase::Header("NEON CONSTELLATION",
         "Real-time procedural simulation  /  native ImGui 1.93 drawing and controls");
      ImGui::TextColored({.50F,.92F,.88F,1.0F},"SCENE SELECTOR");
      float const flButtonWidth =
         (ImGui::GetContentRegionAvail().x-3.0F*ImGui::GetStyle().ItemSpacing.x)/4.0F;
      for(int i=0;i<static_cast<int>(arrScenes.size());++i) {
         if(i) ImGui::SameLine();
         ImGui::PushID(i);
         bool const bSelected=i==iSelected;
         if(bSelected)
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4{.08F,.47F,.53F,1.0F});
         if(ImGui::Button(arrScenes[i].szName,{std::max(105.0F,flButtonWidth),35.0F}))
            iSelected=i;
         if(bSelected) ImGui::PopStyleColor();
         ImGui::PopID();
         }
      ImGui::Spacing();
      if(ImGui::Button(bPlaying?"Pause animation":"Resume animation"))bPlaying=!bPlaying;
      ImGui::SameLine();
      if(ImGui::Button("Reset")){flTime=0.0;flZoom=1.0F;}
      ImGui::SameLine();
      ImGui::SetNextItemWidth(180);
      ImGui::SliderFloat("Zoom",&flZoom,.5F,1.8F,"%.2fx");
      ImGui::TextDisabled("%s  |  %s",arrScenes[iSelected].szName,
                          arrScenes[iSelected].szDescription);
      ImVec2 const aPos=ImGui::GetCursorScreenPos();
      ImVec2 const aSize=ImGui::GetContentRegionAvail();
      DrawScene(ImGui::GetWindowDrawList(),aPos,
                {aSize.x,std::max(180.0F,aSize.y-12.0F)},iSelected,flTime,flZoom);
      ImGui::End();
      aApp.EndFrame();
      ++iFrame;
      }
}

int main(int const iArgc,char const* const* const pszArgv) {
   try {
      bool const bVerify=iArgc>1&&std::string_view{pszArgv[1]}=="--verify";
      if(bVerify) {
         IMGUI_CHECKVERSION();
         ImGuiContext* const pContext=ImGui::CreateContext();
         ImGuiIO& aIo=ImGui::GetIO();
         aIo.DisplaySize={1280,720};
         aIo.DeltaTime=1.0F/60.0F;
         if(!aIo.Fonts->Build()){ImGui::DestroyContext(pContext);return 2;}
         ImGui::NewFrame();
         DrawScene(ImGui::GetBackgroundDrawList(),{0,0},{1280,720},0,1.0,1.0F);
         ImGui::EndFrame();ImGui::Render();
         ImDrawData const* const pData=ImGui::GetDrawData();
         bool const bPassed=pData && pData->TotalVtxCount>3000;
         std::println("SHOWCASE|CHECK|geometry|{}|ImGui vertices={}",
            bPassed?"PASS":"FAIL",pData?pData->TotalVtxCount:0);
         ImGui::DestroyContext(pContext);
         return bPassed?0:1;
         }
      RenderApp(true);
      return 0;
      }
   catch(std::exception const& aError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",aError.what());
      return 3;
      }
}
