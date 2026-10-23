// SPDX-License-Identifier: MIT
#include "GeoData.h"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_reduce.h>
#include <omp.h>
#include <algorithm>
#include <charconv>
#include <fast_float/fast_float.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>
namespace adecc::geo {
namespace {
using Clock=std::chrono::steady_clock;
double Millis(Clock::time_point a) {
   return std::chrono::duration<double,std::milli>(Clock::now()-a).count();
}
bool ParseCoord(std::string_view s,double& d) {
   auto r=fast_float::from_chars(s.data(),s.data()+s.size(),d);
   return r.ec==std::errc{} && r.ptr==s.data()+s.size() && std::isfinite(d);
}
bool Parse(std::string_view line,Point& p) {
   // CompareTest/Test_Read.cpp: City;Street;Number;ZIP;Urban;OldUrban;District;Latitude;Longitude.
   std::size_t start=0;
   std::string_view lat,lon;
   for(int col=0;col<9;++col) {
      auto const pos=line.find(';',start);
      if(col<8 && pos==std::string_view::npos)return false;
      auto const value=line.substr(start,pos==std::string_view::npos?pos:pos-start);
      if(col==7)lat=value;
      if(col==8)lon=value;
      start=pos==std::string_view::npos?line.size():pos+1;
   }
   if(!ParseCoord(lat,p.latitude)||!ParseCoord(lon,p.longitude))return false;
   return p.latitude>=-90 && p.latitude<=90 && p.longitude>=-180 && p.longitude<=180;
}
double X(Point p, double centerLat) {
   constexpr double rad=0.01745329251994329577;
   return p.longitude * std::cos(centerLat*rad);
}
struct Pixel {int x{},y{};};
Pixel Transform(Point p,Bounds b,int width,int height) {
   double const centerLat=(b.minY+b.maxY)*0.5;
   double const xx=X(p,centerLat);
   double const spanX=(std::max)(1e-12,b.maxX-b.minX);
   double const spanY=(std::max)(1e-12,b.maxY-b.minY);
   double const availableX=width*0.94, availableY=height*0.94;
   double const scale=(std::min)(availableX/spanX,availableY/spanY);
   double const px=width*0.5+(xx-(b.minX+b.maxX)*0.5)*scale;
   double const py=height*0.5-(p.latitude-(b.minY+b.maxY)*0.5)*scale;
   return {static_cast<int>((std::clamp)(px,-1000000.0,1000000.0)),
      static_cast<int>((std::clamp)(py,-1000000.0,1000000.0))};
}
std::uint32_t Color(double t) {
   t=(std::clamp)(t,0.0,1.0);
   // Deep blue -> cyan -> green -> amber -> red (population is NOT inferred).
   struct RGB {double r,g,b;};
   constexpr RGB stops[]={{15,35,69},{19,135,186},{57,190,139},
      {250,200,75},{232,84,58}};
   auto const scaled=t*4.0;
   int const i=(std::min)(3,static_cast<int>(scaled));
   double const f=scaled-i;
   auto mix=[&](double a,double b){return static_cast<std::uint32_t>(a+(b-a)*f+0.5);};
   return 0xff000000u | (mix(stops[i].r,stops[i+1].r)<<16)
      | (mix(stops[i].g,stops[i+1].g)<<8) | mix(stops[i].b,stops[i+1].b);
}
}
Dataset Load(std::string const& path,bool parallel) {
   auto const begin=Clock::now();
   std::ifstream in(path,std::ios::binary);
   if(!in)throw std::runtime_error("cannot open input address file: "+path);
   std::string bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
   // Full-line offsets give deterministic output ordering without split-line races.
   std::vector<std::pair<std::size_t,std::size_t>> lines;
   std::size_t start=0;
   while(start<bytes.size()) {
      auto const pos=bytes.find('\n',start);
      std::size_t const end=pos==std::string::npos?bytes.size():pos;
      if(end>start)lines.emplace_back(start,end);
      if(pos==std::string::npos)break;
      start=pos+1;
   }
   std::vector<Point> parsed(lines.size());
   std::vector<std::uint8_t> valid(lines.size(),0);
   auto work=[&](std::size_t i) {
      auto const [a,b]=lines[i];
      std::string_view sv(bytes.data()+a,b-a);
      if(!sv.empty() && sv.back()=='\r')sv.remove_suffix(1);
      valid[i]=Parse(sv,parsed[i])?1:0;
   };
   if(parallel)oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0,lines.size(),4096),
      [&](auto const& range){for(auto i=range.begin();i!=range.end();++i)work(i);});
   else for(std::size_t i=0;i<lines.size();++i)work(i);
   Dataset out;
   out.points.reserve(lines.size());
   for(std::size_t i=0;i<lines.size();++i)
      if(valid[i])out.points.push_back(parsed[i]);
      else ++out.rejected;
   out.readMs=Millis(begin);
   if(out.points.empty())throw std::runtime_error("no valid GPS coordinates in input");
   return out;
}

Dataset LoadDirectory(std::string const& directory,bool parallel) {
   namespace fs=std::filesystem;
   auto const begin=Clock::now();
   std::vector<fs::path> files;
   for(auto const& city:fs::directory_iterator(fs::path(directory))) {
      if(!city.is_directory())continue;
      for(auto const& unit:fs::directory_iterator(city.path())) {
         if(!unit.is_directory())continue;
         for(auto const& district:fs::directory_iterator(unit.path())) {
            if(!district.is_directory())continue;
            for(auto const& street:fs::directory_iterator(district.path()))
               if(street.is_regular_file() && street.path().extension()==".csv")
                  files.push_back(street.path());
         }
      }
   }
   std::sort(files.begin(),files.end());
   if(files.empty())throw std::runtime_error("no CompareTest hierarchical CSV files found");
   std::vector<std::vector<Point>> chunks(files.size());
   std::vector<std::size_t> rejected(files.size(),0);
   auto parseFile=[&](std::size_t i) {
      std::ifstream in(files[i],std::ios::binary);
      if(!in)throw std::runtime_error("cannot open hierarchy CSV");
      std::string line;
      while(std::getline(in,line)) {
         if(!line.empty() && line.back()=='\r')line.pop_back();
         // CompareTest directory row: number;zip;old-urban;lat;lon;distance;bearing
         std::string_view view(line);
         std::size_t start=0;
         std::string_view lat,lon;
         bool format=true;
         for(int col=0;col<5;++col) {
            auto const end=view.find(';',start);
            if(end==std::string_view::npos && col<4){format=false;break;}
            auto const field=view.substr(start,end==std::string_view::npos?end:end-start);
            if(col==3)lat=field;
            if(col==4)lon=field;
            start=end==std::string_view::npos?view.size():end+1;
         }
         Point point;
         if(format && ParseCoord(lat,point.latitude) && ParseCoord(lon,point.longitude)
            && point.latitude>=-90 && point.latitude<=90
            && point.longitude>=-180 && point.longitude<=180)
            chunks[i].push_back(point);
         else ++rejected[i];
      }
   };
   if(parallel)oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0,files.size(),1),
      [&](auto const& range){for(auto i=range.begin();i!=range.end();++i)parseFile(i);});
   else for(std::size_t i=0;i<files.size();++i)parseFile(i);
   Dataset output;
   for(std::size_t i=0;i<files.size();++i) {
      output.points.insert(output.points.end(),chunks[i].begin(),chunks[i].end());
      output.rejected+=rejected[i];
   }
   if(output.points.empty())throw std::runtime_error("no GPS points in hierarchy");
   output.readMs=Millis(begin);
   return output;
}
Terrain BuildTerrain(std::vector<Point> const& points,Bounds bounds,
   int width,int height,double sigma,Method method,int threads) {
   if(width<2||height<2||sigma<=0||threads<1)throw std::invalid_argument("invalid terrain size");
   auto const begin=Clock::now();
   // Build a count histogram directly: visualization color is not a metric.
   std::vector<double> density(static_cast<std::size_t>(width)*height,0.0);
   for(auto const& p:points) {
      auto const xy=Transform(p,bounds,width,height);
      if(xy.x<0||xy.x>=width||xy.y<0||xy.y>=height)continue;
      density[static_cast<std::size_t>(xy.y)*width+xy.x]+=1.0;
   }
   int const radius=static_cast<int>(std::ceil(sigma*3.0));
   std::vector<double> kernel(static_cast<std::size_t>(radius)*2+1);
   double sum=0;
   for(int k=-radius;k<=radius;++k) {
      double const v=std::exp(-0.5*k*k/(sigma*sigma));
      kernel[static_cast<std::size_t>(k+radius)]=v;sum+=v;
   }
   for(auto& v:kernel)v/=sum;
   std::vector<double> intermediate(density.size(),0.0);
   auto horizontal=[&](int y){
      for(int x=0;x<width;++x) {
         double v=0;
         for(int k=-radius;k<=radius;++k) {
            int const xx=(std::clamp)(x+k,0,width-1);
            v+=density[static_cast<std::size_t>(y)*width+xx]*kernel[static_cast<std::size_t>(k+radius)];
         }
         intermediate[static_cast<std::size_t>(y)*width+x]=v;
      }
   };
   auto vertical=[&](int y,Terrain& out){
      for(int x=0;x<width;++x) {
         double v=0;
         for(int k=-radius;k<=radius;++k) {
            int const yy=(std::clamp)(y+k,0,height-1);
            v+=intermediate[static_cast<std::size_t>(yy)*width+x]*kernel[static_cast<std::size_t>(k+radius)];
         }
         out.heightField[static_cast<std::size_t>(y)*width+x]=static_cast<float>(v);
      }
   };
   auto rows=[&](auto&& fn) {
      if(method==Method::OpenMP) {
         #pragma omp parallel for num_threads(threads) schedule(static)
         for(int y=0;y<height;++y)fn(y);
      } else if(method==Method::Tbb) {
         oneapi::tbb::parallel_for(0,height,1,[&](int y){fn(y);});
      } else for(int y=0;y<height;++y)fn(y);
   };
   rows(horizontal);
   Terrain output;
   output.width=width;output.height=height;
   output.heightField.resize(density.size());
   rows([&](int y){vertical(y,output);});
   output.peak=*std::max_element(output.heightField.begin(),output.heightField.end());
   output.buildMs=Millis(begin);
   return output;
}

Bounds ProjectBounds(std::vector<Point> const& points,bool parallel) {
   if(points.empty())throw std::invalid_argument("empty point set");
   // The same reference latitude is required for all threads.
   double minLat=90,maxLat=-90;
   for(auto const& p:points){minLat=(std::min)(minLat,p.latitude);maxLat=(std::max)(maxLat,p.latitude);}
   double const centerLat=(minLat+maxLat)*0.5;
   auto identity=[]{return Bounds{std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};};
   auto merge=[](Bounds a,Bounds b){return Bounds{
      (std::min)(a.minX,b.minX),(std::min)(a.minY,b.minY),
      (std::max)(a.maxX,b.maxX),(std::max)(a.maxY,b.maxY)};};
   auto scan=[&](std::size_t i,Bounds& b){
      auto const p=points[i];double const x=X(p,centerLat);
      b.minX=(std::min)(b.minX,x); b.maxX=(std::max)(b.maxX,x);
      b.minY=(std::min)(b.minY,p.latitude);b.maxY=(std::max)(b.maxY,p.latitude);
   };
   if(parallel)return oneapi::tbb::parallel_reduce(
      oneapi::tbb::blocked_range<std::size_t>(0,points.size(),4096),identity(),
      [&](auto const& range,Bounds local){
         for(auto i=range.begin();i!=range.end();++i)scan(i,local);
         return local;
      },merge);
   Bounds b=identity();
   for(std::size_t i=0;i<points.size();++i)scan(i,b);
   return b;
}
Image Rasterize(std::vector<Point> const& points,Bounds const& bounds,
   int width,int height,Mode mode,Method method,int threads,int cellSize) {
   if(width<=0||height<=0||threads<=0||cellSize<=0)throw std::invalid_argument("invalid rendering parameters");
   auto const begin=Clock::now();
   std::vector<Pixel> locations(points.size());
   auto project=[&](std::size_t i){locations[i]=Transform(points[i],bounds,width,height);};
   if(method==Method::Tbb)oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0,points.size(),4096),
      [&](auto const& r){for(auto i=r.begin();i!=r.end();++i)project(i);});
   else if(method==Method::OpenMP) {
      #pragma omp parallel for num_threads(threads) schedule(static)
      for(std::int64_t i=0;i<static_cast<std::int64_t>(points.size());++i)
         project(static_cast<std::size_t>(i));
   } else for(std::size_t i=0;i<points.size();++i)project(i);
   int const cell=mode==Mode::Points?1:cellSize;
   int const cols=(width+cell-1)/cell, rows=(height+cell-1)/cell;
   std::size_t const count=static_cast<std::size_t>(cols)*rows;
   std::vector<std::uint32_t> histogram(count,0);
   // A deterministic collision-free reduction: per-worker histograms when affordable,
   // otherwise serial accumulation; no atomic races on shared pixels.
   bool const localHistogram=method==Method::OpenMP &&
      count*static_cast<std::size_t>(threads)<=4000000;
   if(localHistogram) {
      std::vector<std::vector<std::uint32_t>> locals(
         static_cast<std::size_t>(threads),std::vector<std::uint32_t>(count,0));
      #pragma omp parallel num_threads(threads)
      {
         auto& h=locals[static_cast<std::size_t>(omp_get_thread_num())];
         #pragma omp for schedule(static)
         for(std::int64_t i=0;i<static_cast<std::int64_t>(locations.size());++i) {
            auto const p=locations[static_cast<std::size_t>(i)];
            if(p.x>=0&&p.x<width&&p.y>=0&&p.y<height)
               ++h[static_cast<std::size_t>(p.y/cell)*cols+p.x/cell];
         }
      }
      for(auto const& h:locals)
         for(std::size_t i=0;i<count;++i)histogram[i]+=h[i];
   } else {
      for(auto const p:locations)
         if(p.x>=0&&p.x<width&&p.y>=0&&p.y<height)
            ++histogram[static_cast<std::size_t>(p.y/cell)*cols+p.x/cell];
   }
   std::uint32_t const peak=*(std::max_element)(histogram.begin(),histogram.end());
   Image out;
   out.width=width;out.height=height;out.peak=peak;
   out.totalPoints=0;
   for(auto const p:locations)
      if(p.x>=0&&p.x<width&&p.y>=0&&p.y<height)++out.totalPoints;
   out.argb.resize(static_cast<std::size_t>(width)*height,0xff071525u);
   for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
      std::uint32_t const n=histogram[static_cast<std::size_t>(y/cell)*cols+x/cell];
      if(n) {
         double const strength=mode==Mode::Points?1.0:
            std::log1p(static_cast<double>(n))/std::log1p(static_cast<double>(peak));
         out.argb[static_cast<std::size_t>(y)*width+x]=
            mode==Mode::Points?0xff52d6ffu:Color(strength);
      }
   }
   out.elapsedMs=Millis(begin);
   return out;
}
}
