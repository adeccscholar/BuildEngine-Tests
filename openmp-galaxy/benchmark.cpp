// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Galaxy.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
using namespace adecc::galaxy;
int main(int argc,char** argv) {
   try {
      Configuration cfg;
      if(argc>1) cfg.count=static_cast<std::size_t>(std::strtoull(argv[1],nullptr,10));
      int steps=argc>2?std::atoi(argv[2]):100;
      int threads=argc>3?std::atoi(argv[3]):8;
      if(steps<=0 || threads<=0) return 2;
      auto initial=Initialize(cfg);
      Galaxy reference=initial;
      auto const begin=std::chrono::steady_clock::now();
      for(int step=0;step<steps;++step) Step(reference,Schedule::Serial,1);
      double const baseline=std::chrono::duration<double,std::milli>(
         std::chrono::steady_clock::now()-begin).count();
      std::printf("mode,threads,bodies,steps,simulation_ms,speedup,max_position_error,energy\n");
      std::printf("serial,1,%zu,%d,%.3f,1.000,0,%.9f\n",
         cfg.count,steps,baseline,Energy(reference));
      for(Schedule schedule:{Schedule::Static,Schedule::Dynamic,Schedule::Guided}) {
         Galaxy candidate=initial;
         auto const t0=std::chrono::steady_clock::now();
         for(int step=0;step<steps;++step) Step(candidate,schedule,threads);
         double const milliseconds=std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-t0).count();
         double const error=MaxDifference(reference,candidate);
         std::printf("%s,%d,%zu,%d,%.3f,%.3f,%.9g,%.9f\n",
            ScheduleName(schedule),threads,cfg.count,steps,milliseconds,
            baseline/(std::max)(milliseconds,0.000001),error,Energy(candidate));
         if(error>1e-6) return 1;
      }
      return 0;
   } catch(std::exception const& e) {
      std::fprintf(stderr,"Galaxy benchmark error: %s\n",e.what());
      return 2;
   }
}
