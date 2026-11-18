#include "../common/VectorStage.h"
#include <matplot/matplot.h>
#include <cmath>
#include <numbers>
#include <print>
#include <string_view>
#include <utility>
int main(int const iArgc,char const* const* const pszArgv){
   bool const bVerify=iArgc>1&&std::string_view{pszArgv[1]}=="--verify";
   auto const vecAngles=matplot::linspace(0.0,22.0*std::numbers::pi,2400);
   bool const bPassed=vecAngles.size()==2400&&std::abs(vecAngles.front())<1.e-10&&
      std::abs(vecAngles.back()-22.0*std::numbers::pi)<1.e-8;
   if(bPassed&&!bVerify){
      VectorStage aStage{"matplot-chaos-atlas.svg","MATPLOT++ / CHAOS ATLAS"};
      aStage.Text(65,151,17,"2400 mathematical samples / five nested oscillatory structures");
      for(int iLayer=0;iLayer<5;++iLayer){
         double const flScale=1.0-iLayer*.11;
         auto const fnPoint=[flScale,iLayer](double const flT){
            double const flRadius=205.0+75.0*std::sin(flT*.42+iLayer*.31);
            return std::pair{635.0+flScale*flRadius*std::cos(flT*1.73),
               415.0+flScale*flRadius*.79*std::sin(flT*2.23)};
            };
         for(std::size_t uI=1;uI<vecAngles.size();++uI){
            auto const [flX,flY]=fnPoint(vecAngles[uI-1]);
            auto const [flXX,flYY]=fnPoint(vecAngles[uI]);
            aStage.Line(flX,flY,flXX,flYY,iLayer%2?"#de68ff":"#26e9de");
            }
         }
      aStage.Text(65,695,16,"Matplot++ deterministic sampling / vector visualization / no external gnuplot");
      }
   std::println("SHOWCASE|CHECK|sampling|{}|Samples={}",bPassed?"PASS":"FAIL",vecAngles.size());
   return bPassed?0:1;
}
