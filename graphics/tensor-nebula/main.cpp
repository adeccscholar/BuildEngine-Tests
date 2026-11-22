#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"
#include <xtensor/containers/xarray.hpp>
#include <xtensor/core/xmath.hpp>
#include <xtl/xoptional.hpp>
#include <xsimd/xsimd.hpp>
#include <algorithm>
#include <cmath>
#include <exception>
#include <print>
#include <string_view>
namespace {
constexpr std::size_t uCols=72, uRows=44;
/**
\brief Computes coherent two-source interference with the installed xtensor library.
\details Uses a native two-dimensional tensor for the real rendered field.
\param flPhase Phase offset.
\param flFrequency Spatial angular frequency.
\returns Numerically bounded intensity array.
\throw std::bad_alloc When allocation fails.
*/
xt::xarray<double> Generate(double const flPhase, double const flFrequency) {
   xt::xarray<double> theField=xt::zeros<double>({uRows,uCols});
   for(std::size_t uRow=0;uRow<uRows;++uRow) {
      for(std::size_t uCol=0;uCol<uCols;++uCol) {
         double const flX=(double(uCol)/(uCols-1)-.5)*6.0;
         double const flY=(double(uRow)/(uRows-1)-.5)*3.7;
         double const flA=std::cos(flFrequency*std::hypot(flX+1.35,flY));
         double const flB=std::cos(flFrequency*std::hypot(flX-1.35,flY)+flPhase);
         theField(uRow,uCol)=.25*(flA+flB)*(flA+flB);
      }
   }
   return theField;
}
bool Verify() {
   xtl::xoptional<int> const theOptional {42};
   xsimd::batch<float> const theVector {2.0F};
   float const flSum=xsimd::reduce_add(theVector);
   auto const theField=Generate(0.0,7.0);
   bool const bXtl=theOptional.has_value()&&theOptional.value()==42;
   bool const bSimd=std::abs(flSum-2.0F*xsimd::batch<float>::size)<.005F;
   bool const bTensor=theField.dimension()==2 && theField.shape()[0]==uRows
      && theField.shape()[1]==uCols && std::all_of(theField.begin(),theField.end(),
         [](double const flValue){return std::isfinite(flValue)&&flValue>=0&&flValue<=1.000001;});
   std::println("SHOWCASE|CHECK|xtl|{}",bXtl?"PASS":"FAIL");
   std::println("SHOWCASE|CHECK|xsimd|{}",bSimd?"PASS":"FAIL");
   std::println("SHOWCASE|CHECK|xtensor|{}",bTensor?"PASS":"FAIL");
   bool const bOK=bXtl&&bSimd&&bTensor;
   std::println("SHOWCASE|RESULT|{}|Tensor Nebula",bOK?"PASS":"FAIL");
   return bOK;
}
void Interactive() {
   NativeShowcase aApp {"BuildEngine | TENSOR NEBULA - Mathematics of Light",1500,920};
   float flPhase=.15F,flFrequency=7.0F;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("NEBULA",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("TENSOR NEBULA / MATHEMATICS OF LIGHT","xtensor + xsimd + xtl / native interference laboratory");
      ImGui::SliderFloat("Phase",&flPhase,-3.14159F,3.14159F);
      ImGui::SliderFloat("Spatial frequency",&flFrequency,2.0F,16.0F);
      auto const theField=Generate(flPhase,flFrequency);
      ImVec2 const aOrigin=ImGui::GetCursorScreenPos(),aAvailable=ImGui::GetContentRegionAvail();
      float const flW=aAvailable.x/float(uCols),flH=aAvailable.y/float(uRows);
      ImDrawList* const pDraw=ImGui::GetWindowDrawList();
      for(std::size_t uRow=0;uRow<uRows;++uRow) {
         for(std::size_t uCol=0;uCol<uCols;++uCol) {
            float const flV=float(theField(uRow,uCol));
            ImU32 const uColor=IM_COL32(int(17+230*flV*flV),int(24+180*std::sqrt(flV)),int(55+190*(1-std::abs(flV-.5F))),255);
            float const flX=aOrigin.x+uCol*flW,flY=aOrigin.y+uRow*flH;
            pDraw->AddRectFilled({flX,flY},{flX+flW+.2F,flY+flH+.2F},uColor);
         }
      }
      ImGui::End();
      aApp.EndFrame();
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
