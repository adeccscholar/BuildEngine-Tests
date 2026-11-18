#include "../common/ImGuiVector.h"
#include <implot.h>
#include <array>
#include <cmath>
#include <print>
#include <string_view>
int main(int const iArgc,char const* const* const pszArgv){
   bool const bVerify=iArgc>1&&std::string_view{pszArgv[1]}=="--verify";
   IMGUI_CHECKVERSION();
   ImGuiContext* const pGui=ImGui::CreateContext();
   if(!pGui)return 2;
   ImPlotContext* const pPlot=ImPlot::CreateContext();
   ImGuiIO& aIo=ImGui::GetIO();aIo.DisplaySize={1280,720};aIo.DeltaTime=1.0F/60.0F;
   if(!aIo.Fonts->Build()){ImPlot::DestroyContext(pPlot);ImGui::DestroyContext(pGui);return 3;}
   std::array<double,768> arrX{},arrA{},arrB{};
   for(std::size_t uI=0;uI<arrX.size();++uI){
      double const flT=uI/60.0;
      arrX[uI]=flT;arrA[uI]=std::sin(flT*3.4)*std::cos(flT*.61);
      arrB[uI]=.65*std::cos(flT*5.1+std::sin(flT*.7));
      }
   ImGui::NewFrame();
   ImGui::SetNextWindowPos({65,162});ImGui::SetNextWindowSize({1150,490});
   ImGui::Begin("SIGNAL THEATER",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
   bool bPlotted{};
   if(ImPlot::BeginPlot("SIGNAL INTERFERENCE",{1090,390})){
      ImPlot::PlotLine("Carrier A",arrX.data(),arrA.data(),static_cast<int>(arrX.size()));
      ImPlot::PlotLine("Carrier B",arrX.data(),arrB.data(),static_cast<int>(arrX.size()));
      ImPlot::EndPlot();bPlotted=true;
      }
   ImGui::End();ImGui::Render();
   ImDrawData const* const pData=ImGui::GetDrawData();
   bool const bPassed=bPlotted&&pData&&pData->TotalVtxCount>1000;
   if(bPassed&&!bVerify){
      VectorStage aStage{"implot-signal-theater.svg","IMPLOT / SIGNAL THEATER"};
      ExportImGui(aStage,pData);
      aStage.Text(65,696,16,"Native ImPlot line series / 768 samples each / genuine draw geometry");
      }
   std::println("SHOWCASE|CHECK|plot|{}|Vertices={}",bPassed?"PASS":"FAIL",pData?pData->TotalVtxCount:0);
   ImPlot::DestroyContext(pPlot);ImGui::DestroyContext(pGui);
   return bPassed?0:1;
}
