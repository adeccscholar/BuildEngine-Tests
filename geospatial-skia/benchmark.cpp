// SPDX-License-Identifier: MIT
#include "GeoData.h"
#include <algorithm>
#include <cstdio>
#include <exception>
#include <string>
using namespace adecc::geo;
int main(int argc,char** argv) {
   if(argc<2) {std::fprintf(stderr,"usage: geospatial_benchmark <berlin_infos.dat>\n");return 2;}
   try {
      auto sequential=Load(argv[1],false),parallel=Load(argv[1],true);
      if(sequential.points.size()!=parallel.points.size() ||
         sequential.rejected!=parallel.rejected)return 1;
      for(std::size_t i=0;i<sequential.points.size();++i)
         if(sequential.points[i].latitude!=parallel.points[i].latitude ||
            sequential.points[i].longitude!=parallel.points[i].longitude)return 1;
      auto bounds=ProjectBounds(sequential.points,false);
      auto boundsParallel=ProjectBounds(parallel.points,true);
      if(bounds.minX!=boundsParallel.minX||bounds.maxX!=boundsParallel.maxX||
         bounds.minY!=boundsParallel.minY||bounds.maxY!=boundsParallel.maxY)return 1;
      std::printf("load: serial %.3f ms, oneTBB %.3f ms | addresses %zu | rejected %zu\n",
         sequential.readMs,parallel.readMs,sequential.points.size(),sequential.rejected);
      for(auto mode:{Mode::Points,Mode::Density}) {
         auto serial=Rasterize(sequential.points,bounds,1280,720,mode,Method::Serial,1,12);
         for(auto method:{Method::Tbb,Method::OpenMP}) {
            auto next=Rasterize(sequential.points,bounds,1280,720,mode,method,8,12);
            bool equal=serial.argb==next.argb&&serial.peak==next.peak;
            std::printf("%s %s: %.3f ms vs %.3f ms serial, equal=%s\n",
               mode==Mode::Points?"points":"density",
               method==Method::Tbb?"oneTBB":"OpenMP",
               next.elapsedMs,serial.elapsedMs,equal?"PASS":"FAIL");
            if(!equal)return 1;
         }
      }
      return 0;
   }catch(std::exception const& e) {
      std::fprintf(stderr,"%s\n",e.what());
      return 2;
   }
}
