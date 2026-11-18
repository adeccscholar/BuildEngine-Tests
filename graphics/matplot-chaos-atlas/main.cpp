#include "../common/VectorStage.h"
#include <SDL.h>
#include <matplot/matplot.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct Point { double flX; double flY; };
struct Scene {
   std::string strTitle;
   SDL_Color theColor;
   bool bPolyline;
   std::vector<Point> vecPoints;
};

[[nodiscard]] std::vector<Scene> BuildScenes() {
   std::array<std::string,6> const arrNames {
      "LISSAJOUS", "CLIFFORD", "DE JONG", "HARMONIC BLOOM",
      "BUTTERFLY", "ORBITAL RESONANCE"
   };
   std::array<SDL_Color,6> const arrColors {{
      {22,240,223,255}, {247,107,204,255}, {246,186,57,255},
      {126,143,255,255}, {179,250,112,255}, {211,116,254,255}
   }};
   auto const vecSamples = matplot::linspace(0.0,48.0*std::numbers::pi,12000);
   std::vector<Scene> vecScenes;
   for(int iScene=0;iScene<6;++iScene) {
      Scene aScene { arrNames[iScene],arrColors[iScene],iScene!=1&&iScene!=2,{} };
      aScene.vecPoints.reserve(vecSamples.size());
      double flX=.1,flY=.1;
      for(double const flT : vecSamples) {
         double flNewX{},flNewY{};
         switch(iScene) {
            case 0:
               flNewX=std::sin(3.0*flT+.35*std::cos(flT*.07));
               flNewY=std::sin(4.0*flT+.38*std::sin(flT*.09));
               break;
            case 1:
               flNewX=.5*(std::sin(1.7*flY)+.93*std::cos(1.7*flX));
               flNewY=.5*(std::sin(-1.8*flX)+.72*std::cos(-1.8*flY));
               break;
            case 2:
               flNewX=.5*(std::sin(1.4*flY)-std::cos(-2.3*flX));
               flNewY=.5*(std::sin(2.4*flX)-std::cos(-2.1*flY));
               break;
            case 3: {
               double const flRadius=.48+.40*std::sin(11.0*flT)+.08*std::cos(23.0*flT);
               flNewX=flRadius*std::cos(flT);flNewY=flRadius*std::sin(flT);
               break;
            }
            case 4: {
               double const flRadius=std::exp(std::sin(flT))-2.0*std::cos(4.0*flT)+
                  std::pow(std::sin((2.0*flT-std::numbers::pi)/24.0),5.0);
               flNewX=.25*flRadius*std::sin(flT);
               flNewY=.25*flRadius*std::cos(flT);
               break;
            }
            default:
               flNewX=.52*std::cos(5.0*flT)+.29*std::cos(11.0*flT);
               flNewY=.52*std::sin(7.0*flT)-.29*std::sin(13.0*flT);
               break;
         }
         flX=flNewX;flY=flNewY;
         if(!std::isfinite(flX)||!std::isfinite(flY))
            throw std::runtime_error{"Nonfinite numeric sample"};
         aScene.vecPoints.push_back({flX,flY});
      }
      vecScenes.push_back(std::move(aScene));
   }
   return vecScenes;
}

void Draw(SDL_Renderer* const pRenderer, Scene const& aScene,
          SDL_Rect const& aArea,double const flZoom,double const flAngle,
          std::size_t const uVisible) {
   SDL_SetRenderDrawColor(pRenderer,10,21,47,255);
   SDL_RenderFillRect(pRenderer,&aArea);
   SDL_RenderSetClipRect(pRenderer,&aArea);
   SDL_SetRenderDrawBlendMode(pRenderer,SDL_BLENDMODE_BLEND);
   SDL_SetRenderDrawColor(pRenderer,aScene.theColor.r,aScene.theColor.g,
                          aScene.theColor.b,aScene.bPolyline?166:145);
   double const flC=std::cos(flAngle),flS=std::sin(flAngle);
   double const flScale=.43*std::min(aArea.w,aArea.h)*flZoom;
   SDL_Point aPrevious{};bool bPrevious=false;
   for(std::size_t uI=0;uI<std::min(uVisible,aScene.vecPoints.size());++uI) {
      Point const& aPoint=aScene.vecPoints[uI];
      double const flX=aPoint.flX*flC-aPoint.flY*flS;
      double const flY=aPoint.flX*flS+aPoint.flY*flC;
      SDL_Point const aCurrent {
         static_cast<int>(aArea.x+aArea.w*.5+flX*flScale),
         static_cast<int>(aArea.y+aArea.h*.5+flY*flScale)
      };
      if(aScene.bPolyline&&bPrevious) {
         if(std::abs(aCurrent.x-aPrevious.x)<aArea.w/3 &&
            std::abs(aCurrent.y-aPrevious.y)<aArea.h/3)
            SDL_RenderDrawLine(pRenderer,aPrevious.x,aPrevious.y,
                               aCurrent.x,aCurrent.y);
      }
      else SDL_RenderDrawPoint(pRenderer,aCurrent.x,aCurrent.y);
      aPrevious=aCurrent;bPrevious=true;
   }
   SDL_RenderSetClipRect(pRenderer,nullptr);
}

void Export(std::vector<Scene> const& vecScenes) {
   std::filesystem::create_directories("artifacts");
   for(std::size_t uIndex=0;uIndex<vecScenes.size();++uIndex) {
      Scene const& aScene=vecScenes[uIndex];
      VectorStage aStage {
         std::filesystem::path{"artifacts"}/std::format("scene-{}.svg",uIndex+1),
         aScene.strTitle
      };
      for(std::size_t uI=1;uI<aScene.vecPoints.size();++uI) {
         Point const& a=aScene.vecPoints[uI-1];
         Point const& b=aScene.vecPoints[uI];
         double const flX=640+a.flX*235,flY=414+a.flY*235;
         double const flXX=640+b.flX*235,flYY=414+b.flY*235;
         if(aScene.bPolyline) {
            if(std::abs(flXX-flX)<300&&std::abs(flYY-flY)<300)
               aStage.Line(flX,flY,flXX,flYY,"#52e5ec");
         }
         else aStage.Line(flXX,flYY,flXX+.4,flYY+.4,"#eb88fd");
      }
   }
}

void Run(std::vector<Scene> const& vecScenes) {
   if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0)
      throw std::runtime_error{SDL_GetError()};
   struct SDLShutdown { ~SDLShutdown(){SDL_Quit();} } const aShutdown;
   SDL_Window* const pWindow=SDL_CreateWindow("Matplot++ Visual Mathematics Gallery",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1440,900,SDL_WINDOW_RESIZABLE);
   if(!pWindow)throw std::runtime_error{SDL_GetError()};
   struct WindowGuard { SDL_Window* p; ~WindowGuard(){SDL_DestroyWindow(p);} } const aWindow{pWindow};
   SDL_Renderer* const pRenderer=SDL_CreateRenderer(pWindow,-1,
      SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
   if(!pRenderer)throw std::runtime_error{SDL_GetError()};
   struct RendererGuard { SDL_Renderer* p; ~RendererGuard(){SDL_DestroyRenderer(p);} } const aRenderer{pRenderer};
   SDL_RenderSetLogicalSize(pRenderer,1440,900);
   bool bRun=true,bGallery=true,bAnimate=true;
   int iSelected=0;
   double flZoom=1.0,flRotation=0.0;
   std::uint64_t uPrevious=SDL_GetTicks64();
   while(bRun) {
      SDL_Event aEvent{};
      while(SDL_PollEvent(&aEvent)) {
         if(aEvent.type==SDL_QUIT)bRun=false;
         if(aEvent.type==SDL_KEYDOWN) {
            switch(aEvent.key.keysym.sym) {
               case SDLK_ESCAPE: if(bGallery)bRun=false;else bGallery=true;break;
               case SDLK_RETURN: bGallery=false;break;
               case SDLK_g: bGallery=true;break;
               case SDLK_SPACE: bAnimate=!bAnimate;break;
               case SDLK_LEFT: iSelected=(iSelected+5)%6;break;
               case SDLK_RIGHT: iSelected=(iSelected+1)%6;break;
               case SDLK_s: Export(vecScenes);break;
               case SDLK_EQUALS: flZoom=std::min(2.5,flZoom*1.15);break;
               case SDLK_MINUS: flZoom=std::max(.35,flZoom/1.15);break;
               case SDLK_F11: SDL_SetWindowFullscreen(pWindow,
                  (SDL_GetWindowFlags(pWindow)&SDL_WINDOW_FULLSCREEN_DESKTOP)?
                  0:SDL_WINDOW_FULLSCREEN_DESKTOP);break;
               default:break;
            }
         }
         if(aEvent.type==SDL_MOUSEBUTTONDOWN&&aEvent.button.button==SDL_BUTTON_LEFT) {
            float flX{},flY{};
            SDL_RenderWindowToLogical(pRenderer,aEvent.button.x,aEvent.button.y,&flX,&flY);
            if(bGallery) {
               for(int i=0;i<6;++i) {
                  SDL_Rect const aTile{30+(i%3)*470,95+(i/3)*380,440,340};
                  if(flX>=aTile.x&&flX<aTile.x+aTile.w&&
                     flY>=aTile.y&&flY<aTile.y+aTile.h) {
                     iSelected=i;bGallery=false;flZoom=1.0;
                  }
               }
            }
            else bGallery=true;
         }
         if(aEvent.type==SDL_MOUSEWHEEL&&!bGallery)
            flZoom=std::clamp(flZoom*(aEvent.wheel.y>0?1.12:.89),.35,2.5);
      }
      std::uint64_t const uNow=SDL_GetTicks64();
      if(bAnimate&&!bGallery)flRotation+=std::min(double(uNow-uPrevious)*.00004,.1);
      uPrevious=uNow;
      SDL_SetRenderDrawColor(pRenderer,5,9,25,255);
      SDL_RenderClear(pRenderer);
      if(bGallery) {
         SDL_SetWindowTitle(pWindow,"Matplot++ Gallery | Click a tile | S: SVG | F11: Fullscreen");
         for(int i=0;i<6;++i) {
            SDL_Rect const aTile{30+(i%3)*470,95+(i/3)*380,440,340};
            SDL_SetRenderDrawColor(pRenderer,35,58,91,255);
            SDL_RenderFillRect(pRenderer,&aTile);
            SDL_Rect const aPreview{aTile.x+7,aTile.y+7,426,326};
            Draw(pRenderer,vecScenes[i],aPreview,1.0,0.0,12000);
         }
      }
      else {
         SDL_SetWindowTitle(pWindow,std::format("Matplot++ | {} | Left/Right: scene | Space: rotate | Wheel: zoom | Esc: gallery | S: SVG",
            vecScenes[iSelected].strTitle).c_str());
         SDL_Rect const aDetail{50,42,1340,815};
         Draw(pRenderer,vecScenes[iSelected],aDetail,flZoom,flRotation,12000);
      }
      SDL_RenderPresent(pRenderer);
   }
}
}

int main(int const iArgc,char const* const* const pszArgv) {
   try {
      std::vector<Scene> const vecScenes=BuildScenes();
      bool const bPassed=vecScenes.size()==6&&std::ranges::all_of(vecScenes,
         [](Scene const& aScene){return aScene.vecPoints.size()==12000;});
      std::println("SHOWCASE|CHECK|sampling|{}|Six Matplot++ scenes",bPassed?"PASS":"FAIL");
      if(!bPassed)return 1;
      if(iArgc>1&&std::string_view{pszArgv[1]}=="--verify")return 0;
      if(iArgc>1&&std::string_view{pszArgv[1]}=="--export"){Export(vecScenes);return 0;}
      Run(vecScenes);
      return 0;
   }
   catch(std::exception const& aException) {
      std::println(stderr,"SHOWCASE|ERROR|{}",aException.what());
      return 2;
   }
}
