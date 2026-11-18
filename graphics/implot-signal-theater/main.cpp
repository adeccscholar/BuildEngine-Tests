#include "../common/NativeShowcase.h"

#include <implot.h>
#include <array>
#include <cmath>
#include <exception>
#include <numbers>
#include <print>
#include <string_view>

namespace {
constexpr std::size_t uSamples=1024;
struct Signal {
   std::array<double,uSamples> arrTime {};
   std::array<double,uSamples> arrA {};
   std::array<double,uSamples> arrB {};
   std::array<double,uSamples> arrC {};
};

[[nodiscard]] Signal Calculate(int const iMode,double const flTime,
                               double const flFrequency,double const flAmplitude) {
   Signal aSignal;
   for(std::size_t uI=0;uI<uSamples;++uI) {
      double const flX=static_cast<double>(uI)*.018;
      aSignal.arrTime[uI]=flX;
      double const flPhase=flX*flFrequency+flTime;
      switch(iMode) {
         case 0:
            aSignal.arrA[uI]=flAmplitude*std::sin(flPhase*2.0);
            aSignal.arrB[uI]=flAmplitude*.65*std::cos(flPhase*3.2);
            aSignal.arrC[uI]=aSignal.arrA[uI]+aSignal.arrB[uI];
            break;
         case 1:
            aSignal.arrA[uI]=flAmplitude*std::sin(flPhase*3.4)*
               std::cos(flPhase*.31);
            aSignal.arrB[uI]=flAmplitude*.75*std::sin(flPhase*2.8+.25);
            aSignal.arrC[uI]=aSignal.arrA[uI]-aSignal.arrB[uI];
            break;
         case 2: {
            double const flEnvelope=std::exp(-.012*(flX-9.0)*(flX-9.0));
            aSignal.arrA[uI]=flAmplitude*flEnvelope*std::sin(flPhase*4.6);
            aSignal.arrB[uI]=flAmplitude*flEnvelope*std::cos(flPhase*4.6);
            aSignal.arrC[uI]=std::sqrt(
               aSignal.arrA[uI]*aSignal.arrA[uI]+
               aSignal.arrB[uI]*aSignal.arrB[uI]);
            break;
            }
         default:
            aSignal.arrA[uI]=flAmplitude*std::sin(flPhase*5.2+
               .9*std::sin(flPhase*.33));
            aSignal.arrB[uI]=flAmplitude*.38*std::cos(flPhase*8.1);
            aSignal.arrC[uI]=aSignal.arrA[uI]+aSignal.arrB[uI];
            break;
         }
      }
   return aSignal;
}

[[nodiscard]] bool HeadlessVerify() {
   IMGUI_CHECKVERSION();
   ImGuiContext* const pGui=ImGui::CreateContext();
   ImPlotContext* const pPlot=ImPlot::CreateContext();
   ImGuiIO& aIo=ImGui::GetIO();
   aIo.DisplaySize={1280,720};
   aIo.DeltaTime=1.0F/60.0F;
   if(!aIo.Fonts->Build()){
      ImPlot::DestroyContext(pPlot);
      ImGui::DestroyContext(pGui);
      return false;
      }
   ImGui::NewFrame();
   ImGui::SetNextWindowSize({1100,550});
   ImGui::Begin("SIGNAL LAB");
   Signal const aSignal=Calculate(0,.4,1.0,1.0);
   bool bPlotted=false;
   if(ImPlot::BeginPlot("Real ImPlot Waveforms",{1030,420})) {
      ImPlot::PlotLine("Carrier A",aSignal.arrTime.data(),
         aSignal.arrA.data(),static_cast<int>(uSamples));
      ImPlot::PlotLine("Carrier B",aSignal.arrTime.data(),
         aSignal.arrB.data(),static_cast<int>(uSamples));
      ImPlot::EndPlot();
      bPlotted=true;
      }
   ImGui::End();ImGui::Render();
   ImDrawData const* const pData=ImGui::GetDrawData();
   bool const bPassed=bPlotted&&pData&&pData->TotalVtxCount>1000;
   std::println("SHOWCASE|CHECK|plots|{}|ImPlot vertices={}",
      bPassed?"PASS":"FAIL",pData?pData->TotalVtxCount:0);
   ImPlot::DestroyContext(pPlot);
   ImGui::DestroyContext(pGui);
   return bPassed;
}

void Interactive() {
   NativeShowcase aApp{"BuildEngine | ImPlot Signal Theater"};
   ImPlotContext* const pPlot=ImPlot::CreateContext();
   struct PlotGuard {
      ImPlotContext* p;
      ~PlotGuard(){ImPlot::DestroyContext(p);}
      } const aPlotGuard{pPlot};
   std::array<char const*,4> const arrModes {
      "Interference","Modulated waves","Wave packets","Frequency drift"
   };
   int iMode=0;
   float flFrequency=1.0F;
   float flAmplitude=1.0F;
   float flSpeed=1.0F;
   bool bPlaying=true;
   bool bSeriesA=true,bSeriesB=true,bSeriesC=true;
   double flTime=0.0;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGuiIO const& aIo=ImGui::GetIO();
      if(bPlaying) flTime+=aIo.DeltaTime*flSpeed;
      Signal const aSignal=Calculate(iMode,flTime,flFrequency,flAmplitude);
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(aIo.DisplaySize);
      ImGui::Begin("Signal Theater",nullptr,ImGuiWindowFlags_NoDecoration|
         ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("SIGNAL THEATER",
         "Real-time scientific signal synthesis  /  ImPlot 1.1  /  SDL2 native graphics");
      if(ImGui::BeginCombo("Signal model",arrModes[iMode])) {
         for(int i=0;i<static_cast<int>(arrModes.size());++i) {
            if(ImGui::Selectable(arrModes[i],i==iMode))iMode=i;
            }
         ImGui::EndCombo();
         }
      if(ImGui::Button(bPlaying?"Pause":"Play"))bPlaying=!bPlaying;
      ImGui::SameLine();
      if(ImGui::Button("Reset"))flTime=0.0;
      ImGui::SameLine();
      ImGui::TextColored({.46F,.91F,.87F,1.0F},"Time %.1f s",flTime);
      ImGui::SliderFloat("Frequency",&flFrequency,.2F,4.5F,"%.2fx");
      ImGui::SliderFloat("Amplitude",&flAmplitude,.1F,2.0F,"%.2fx");
      ImGui::SliderFloat("Speed",&flSpeed,.1F,3.0F,"%.2fx");
      ImGui::Checkbox("Carrier A",&bSeriesA);
      ImGui::SameLine();
      ImGui::Checkbox("Carrier B",&bSeriesB);
      ImGui::SameLine();
      ImGui::Checkbox("Result",&bSeriesC);
      ImGui::Spacing();
      ImVec2 const aArea=ImGui::GetContentRegionAvail();
      ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding,ImVec2{20,19});
      if(ImPlot::BeginPlot("LIVE WAVEFORM / OSCILLOSCOPE",
         {std::max(320.0F,aArea.x),std::max(230.0F,aArea.y-14.0F)})) {
         ImPlot::SetupAxes("Time (s)","Amplitude");
         ImPlot::SetupAxisLimits(ImAxis_X1,0.0,aSignal.arrTime.back(),ImGuiCond_Always);
         ImPlot::SetupAxisLimits(ImAxis_Y1,-3.5,3.5,ImGuiCond_Always);
         if(bSeriesA) ImPlot::PlotLine("Carrier A",aSignal.arrTime.data(),
            aSignal.arrA.data(),static_cast<int>(uSamples));
         if(bSeriesB) ImPlot::PlotLine("Carrier B",aSignal.arrTime.data(),
            aSignal.arrB.data(),static_cast<int>(uSamples));
         if(bSeriesC) ImPlot::PlotLine("Combined response",aSignal.arrTime.data(),
            aSignal.arrC.data(),static_cast<int>(uSamples));
         ImPlot::EndPlot();
         }
      ImPlot::PopStyleVar();
      ImGui::End();
      aApp.EndFrame();
      }
}

}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1&&std::string_view{pszArgv[1]}=="--verify")
         return HeadlessVerify()?0:1;
      Interactive();
      return 0;
      }
   catch(std::exception const& aException){
      std::println(stderr,"SHOWCASE|ERROR|{}",aException.what());
      return 2;
      }
}
