// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

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
char const* Name(Fractal f) noexcept {
   return f == Fractal::Julia ? "Julia" : "Mandelbrot";
}
Result Compute(Settings const& cfg) {
   if(cfg.width <= 0 || cfg.height <= 0 || cfg.maxIterations <= 0 ||
      cfg.threads <= 0 || cfg.chunk <= 0 || !std::isfinite(cfg.spanX) ||
      cfg.spanX <= 0 || !std::isfinite(cfg.centerX) || !std::isfinite(cfg.centerY) ||
      !std::isfinite(cfg.juliaReal) || !std::isfinite(cfg.juliaImag))
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
         double zr=cfg.fractal==Fractal::Julia?cx:0.0;
         double zi=cfg.fractal==Fractal::Julia?cy:0.0;
         double const cr=cfg.fractal==Fractal::Julia?cfg.juliaReal:cx;
         double const ci=cfg.fractal==Fractal::Julia?cfg.juliaImag:cy;
         int n=0;
         while(n<cfg.maxIterations && zr*zr+zi*zi<=4.0) {
            double const next=zr*zr-zi*zi+cr;
            zi=2.0*zr*zi+ci;
            zr=next;
            ++n;
         }
         accumulator+=static_cast<std::uint64_t>(n);
         // Smooth escape-time coloring, with a vivid Julia-specific palette.
         // Logarithms are evaluated only for escaped points.
         std::uint32_t color=0xff050914u;
         if(n<cfg.maxIterations) {
            double const radius=std::sqrt(zr*zr+zi*zi);
            double const smooth=radius>1.0
               ? static_cast<double>(n)+1.0-std::log2(std::log2(radius))
               : static_cast<double>(n);
            double const phase=(cfg.fractal==Fractal::Julia?0.039:0.026)
               * (std::max)(0.0,smooth);
            auto channel=[](double t) -> std::uint32_t {
               return static_cast<std::uint32_t>((std::clamp)(127.5+127.5*std::sin(t),0.0,255.0));
            };
            std::uint32_t const red=channel(phase+0.6);
            std::uint32_t const green=channel(phase+2.9);
            std::uint32_t const blue=channel(phase+4.4);
            color=0xff000000u | (red<<16) | (green<<8) | blue;
         }
         out.pixels[static_cast<std::size_t>(y)*cfg.width+x]=color;
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
