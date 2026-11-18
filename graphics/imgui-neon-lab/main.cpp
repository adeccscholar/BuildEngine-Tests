#include "../common/ImGuiVector.h"
#include <cmath>
#include <print>
#include <string_view>
int main(int const iArgc,char const* const* const pszArgv){
   bool const bVerify=iArgc>1&&std::string_view{pszArgv[1]}=="--verify";
   IMGUI_CHECKVERSION();
   ImGuiContext* const pContext=ImGui::CreateContext();
   if(!pContext)return 2;
   ImGuiIO& aIo=ImGui::GetIO();
   aIo.DisplaySize={1280,720};aIo.DeltaTime=1.0F/60.0F;
   if(!aIo.Fonts->Build()){ImGui::DestroyContext(pContext);return 3;}
   ImGui::NewFrame();
   ImDrawList* const pDraw=ImGui::GetBackgroundDrawList();
   pDraw->AddRectFilled({45,158},{1235,654},IM_COL32(7,23,58,225),18);
   for(int i=1;i<420;++i){
      float const flT=i*.029F,flR=24.0F+i*.99F;
      ImVec2 const a{640.0F+flR*std::cos(flT*2.9F),410.0F+flR*.68F*std::sin(flT*3.7F)};
      pDraw->AddCircleFilled(a,1.0F+(i%8)*.6F,IM_COL32(30+i%140,180+i%60,255,185),10);
      if(i>1){
         float const flP=(i-1)*.029F,flQ=24.0F+(i-1)*.99F;
         pDraw->AddLine({640.0F+flQ*std::cos(flP*2.9F),410.0F+flQ*.68F*std::sin(flP*3.7F)},a,IM_COL32(45,240,220,160),1.3F);
         }
      }
   ImGui::EndFrame();ImGui::Render();
   ImDrawData const* const pData=ImGui::GetDrawData();
   bool const bPassed=pData&&pData->TotalVtxCount>1000;
   if(bPassed&&!bVerify){
      VectorStage aStage{"imgui-neon-lab.svg","IMGUI / NEON CONSTELLATION"};
      ExportImGui(aStage,pData);
      aStage.Text(65,694,16,"Native ImGui geometry / 420 nodes / BCC64X DLL consumer");
      }
   std::println("SHOWCASE|CHECK|geometry|{}|Vertices={}",bPassed?"PASS":"FAIL",pData?pData->TotalVtxCount:0);
   ImGui::DestroyContext(pContext);
   return bPassed?0:1;
}
