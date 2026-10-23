#include "Mandelbrot.h"
#include <omp.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace adecc::mandelbrot {
char const* Name(Schedule s) noexcept {
   switch(s) {
      case Schedule::Serial: return "serial";
      case Schedule::Static: return "static";
      case Schedule::Dynamic: return "dynamic";
      case Schedule::Guided: return "guided";
   }
   return "unknown";
}
Result Compute(Settings const& cfg) {
   if(cfg.width <= 0 || cfg.height <= 0 || cfg.maxIterations <= 0 ||
      cfg.threads <= 0 || cfg.chunk <= 0 || !std::isfinite(cfg.spanX) ||
      cfg.spanX <= 0 || !std::isfinite(cfg.centerX) || !std::isfinite(cfg.centerY))
      throw std::invalid_argument("invalid Mandelbrot parameters");
   Result out;
   out.pixels.resize(static_cast<std::size_t>(cfg.width)*cfg.height);
   double const spanY=cfg.spanX*cfg.height/cfg.width;
   auto const first=std::chrono::steady_clock::now();
   std::uint64_t total=0;
   int observed=1;
   auto row=[&](int y, std::uint64_t& accumulator) {
      for(int x=0;x<cfg.width;++x) {
         double const cx=cfg.centerX+((x+0.5)/cfg.width-0.5)*cfg.spanX;
         double const cy=cfg.centerY+((y+0.5)/cfg.height-0.5)*spanY;
         double zr=0.0,zi=0.0;
         int n=0;
         while(n<cfg.maxIterations && zr*zr+zi*zi<=4.0) {
            double const next=zr*zr-zi*zi+cx;
            zi=2.0*zr*zi+cy;
            zr=next;
            ++n;
         }
         accumulator+=static_cast<std::uint64_t>(n);
         std::uint8_t const k=static_cast<std::uint8_t>(255.0*n/cfg.maxIterations);
         out.pixels[static_cast<std::size_t>(y)*cfg.width+x]=n==cfg.maxIterations
            ? 0xff000000u : (0xff000000u | (static_cast<std::uint32_t>(k)<<16)
            | (static_cast<std::uint32_t>(255-k)<<8) | static_cast<std::uint32_t>((k*7u)%256u));
      }
   };
   if(cfg.schedule==Schedule::Serial) {
      for(int y=0;y<cfg.height;++y) row(y,total);
   } else {
      omp_set_dynamic(0);
      omp_set_schedule(cfg.schedule==Schedule::Static?omp_sched_static:
         cfg.schedule==Schedule::Guided?omp_sched_guided:omp_sched_dynamic,cfg.chunk);
      #pragma omp parallel num_threads(cfg.threads) reduction(+:total) reduction(max:observed)
      {
         observed=std::max(observed,omp_get_num_threads());
         #pragma omp for schedule(runtime)
         for(int y=0;y<cfg.height;++y) row(y,total);
      }
   }
   out.iterationTotal=total;
   out.threadsUsed=observed;
   out.milliseconds=std::chrono::duration<double,std::milli>(
      std::chrono::steady_clock::now()-first).count();
   return out;
}
}
