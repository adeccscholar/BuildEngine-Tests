// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Galaxy.h"
#include <omp.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace adecc::galaxy {
namespace {
std::uint32_t Hash(std::uint32_t& state) noexcept {
   state ^= state << 13;
   state ^= state >> 17;
   state ^= state << 5;
   return state;
}
float Random(std::uint32_t& state) noexcept {
   return static_cast<float>(Hash(state) >> 8) * (1.0f / 16777216.0f);
}
void Integrate(Star& s, Configuration const& cfg) noexcept {
   float const r2=s.x*s.x+s.y*s.y+s.z*s.z+cfg.softening*cfg.softening;
   float const inverse=1.0f / (r2 * std::sqrt(r2));
   float const acceleration=-cfg.gravity*inverse;
   s.vx+=cfg.dt*acceleration*s.x;
   s.vy+=cfg.dt*acceleration*s.y;
   s.vz+=cfg.dt*acceleration*s.z;
   s.x+=cfg.dt*s.vx;
   s.y+=cfg.dt*s.vy;
   s.z+=cfg.dt*s.vz;
}
}
char const* ScheduleName(Schedule schedule) noexcept {
   switch(schedule) {
      case Schedule::Serial: return "serial";
      case Schedule::Static: return "static";
      case Schedule::Dynamic: return "dynamic";
      case Schedule::Guided: return "guided";
   }
   return "unknown";
}
Galaxy Initialize(Configuration configuration) {
   if(configuration.count<1 || configuration.count>2000000 ||
      configuration.dt<=0 || configuration.gravity<=0 || configuration.softening<=0)
      throw std::invalid_argument("invalid galaxy parameters");
   Galaxy galaxy;
   galaxy.config=configuration;
   galaxy.stars.resize(configuration.count);
   std::uint32_t state=configuration.seed ? configuration.seed : 1u;
   constexpr float pi=3.14159265358979323846f;
   for(std::size_t i=0;i<configuration.count;++i) {
      float const u=Random(state), v=Random(state), w=Random(state);
      float const radius=0.35f+8.0f*std::sqrt(u);
      float const arm=static_cast<float>(i%4)*pi*0.5f;
      float const theta=arm+radius*0.82f+(v-0.5f)*0.65f;
      float const height=(w-0.5f)*(0.12f+radius*0.07f);
      float const x=radius*std::cos(theta), y=radius*std::sin(theta);
      float const denom=std::pow(radius*radius+configuration.softening*configuration.softening,1.5f);
      float const orbital=std::sqrt(configuration.gravity*radius*radius/denom);
      float const jitter=0.93f+0.14f*Random(state);
      float const hue=Random(state);
      std::uint32_t const red=static_cast<std::uint32_t>(hue<0.23f?255:170+75*Random(state));
      std::uint32_t const green=static_cast<std::uint32_t>(hue<0.23f?178:190+65*Random(state));
      std::uint32_t const blue=static_cast<std::uint32_t>(hue<0.23f?115:255);
      galaxy.stars[i]={x,y,height,-orbital*y/radius*jitter,
         orbital*x/radius*jitter,0.01f*(Random(state)-0.5f),
         0.3f+0.7f*Random(state),
         (red<<16)|(green<<8)|blue};
   }
   return galaxy;
}
void Step(Galaxy& galaxy, Schedule schedule, int threads) {
   if(threads<=0) throw std::invalid_argument("invalid OpenMP thread count");
   auto& stars=galaxy.stars;
   auto const cfg=galaxy.config;
   if(schedule==Schedule::Serial) {
      for(auto& star:stars) Integrate(star,cfg);
      return;
   }
   omp_set_dynamic(0);
   omp_set_schedule(schedule==Schedule::Static?omp_sched_static:
      schedule==Schedule::Dynamic?omp_sched_dynamic:omp_sched_guided,64);
   // Each worker owns distinct stars; the external potential is immutable.
   #pragma omp parallel for schedule(runtime) num_threads(threads)
   for(std::int64_t i=0;i<static_cast<std::int64_t>(stars.size());++i)
      Integrate(stars[static_cast<std::size_t>(i)],cfg);
}
double Energy(Galaxy const& galaxy) {
   double result=0;
   for(auto const& s:galaxy.stars) {
      double const r2=static_cast<double>(s.x)*s.x+static_cast<double>(s.y)*s.y+
         static_cast<double>(s.z)*s.z;
      double const velocity=static_cast<double>(s.vx)*s.vx+
         static_cast<double>(s.vy)*s.vy+static_cast<double>(s.vz)*s.vz;
      result+=0.5*velocity-galaxy.config.gravity/
         std::sqrt(r2+static_cast<double>(galaxy.config.softening)*galaxy.config.softening);
   }
   return result;
}
double MaxDifference(Galaxy const& a, Galaxy const& b) {
   if(a.stars.size()!=b.stars.size()) throw std::invalid_argument("different galaxy sizes");
   double difference=0;
   for(std::size_t i=0;i<a.stars.size();++i) {
      auto const& x=a.stars[i]; auto const& y=b.stars[i];
      difference=(std::max)(difference,static_cast<double>(std::abs(x.x-y.x)));
      difference=(std::max)(difference,static_cast<double>(std::abs(x.y-y.y)));
      difference=(std::max)(difference,static_cast<double>(std::abs(x.z-y.z)));
      difference=(std::max)(difference,static_cast<double>(std::abs(x.vx-y.vx)));
      difference=(std::max)(difference,static_cast<double>(std::abs(x.vy-y.vy)));
      difference=(std::max)(difference,static_cast<double>(std::abs(x.vz-y.vz)));
   }
   return difference;
}
}
