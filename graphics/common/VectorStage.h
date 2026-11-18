#pragma once
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string_view>
class VectorStage {
   std::ofstream osFile;
public:
   explicit VectorStage(std::filesystem::path const& aPath,std::string_view const svTitle)
      : osFile{aPath,std::ios::binary|std::ios::trunc} {
      if(!osFile) throw std::runtime_error{"Cannot create SVG"};
      osFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1280 720\">"
         "<defs><linearGradient id=\"bg\" x2=\"1\" y2=\"1\"><stop stop-color=\"#070d26\"/>"
         "<stop offset=\".6\" stop-color=\"#1b124b\"/><stop offset=\"1\" stop-color=\"#022a37\"/>"
         "</linearGradient></defs><rect width=\"1280\" height=\"720\" fill=\"url(#bg)\"/>"
         "<style>text{font-family:Segoe UI,Arial,sans-serif;fill:#e8faff}</style>\n";
      Text(65,91,40,svTitle);Text(66,122,16,"BUILDENGINE / BCC64X / C++23 / SCIENTIFIC SHOWCASE");
      }
   ~VectorStage(){osFile << "</svg>\n";}
   void Raw(std::string_view const svText){osFile << svText;}
   void Text(int const iX,int const iY,int const iSize,std::string_view const svText){
      osFile<<std::format("<text x=\"{}\" y=\"{}\" font-size=\"{}\">{}</text>\n",iX,iY,iSize,svText);
      }
   void Line(double const flX,double const flY,double const flXX,double const flYY,std::string_view const svColor){
      osFile<<std::format("<path d=\"M{:.2f},{:.2f} L{:.2f},{:.2f}\" stroke=\"{}\" stroke-width=\".9\" opacity=\".63\" fill=\"none\"/>\n",flX,flY,flXX,flYY,svColor);
      }
   };
