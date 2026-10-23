#include "Mandelbrot.h"
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <string_view>
using namespace adecc::mandelbrot;
int main(int argc,char** argv) {
   Settings c;
   if(argc>1) c.width=std::atoi(argv[1]);
   if(argc>2) c.height=std::atoi(argv[2]);
   if(argc>3) c.maxIterations=std::atoi(argv[3]);
   if(argc>4) c.threads=std::atoi(argv[4]);
   if(argc>5 && std::string_view(argv[5])=="julia") {
      c.fractal=Fractal::Julia;
      c.centerX=0.0;
      c.spanX=3.0;
   }
   if(c.width<=0 || c.height<=0 || c.maxIterations<=0 || c.threads<=0) return 2;
   Settings reference=c;
   reference.threads=1;
   reference.schedule=Schedule::Serial;
   auto baseline=Compute(reference);
   std::printf("%s serial %.2f ms checksum=%llu\n",Name(c.fractal),baseline.milliseconds,
      static_cast<unsigned long long>(baseline.iterationTotal));
   for(auto s:{Schedule::Static,Schedule::Dynamic,Schedule::Guided}) {
      c.schedule=s;
      auto r=Compute(c);
      bool const same=r.iterationTotal==baseline.iterationTotal && r.pixels==baseline.pixels;
      std::printf("%s threads=%d %.2f ms speedup=%.2fx identical=%s\n",
         Name(s),r.threadsUsed,r.milliseconds,
         baseline.milliseconds/r.milliseconds,same?"PASS":"FAIL");
      if(!same) return 1;
   }
   return 0;
}
