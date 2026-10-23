#include "Mandelbrot.h"
#include <SDL.h>
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace adecc::mandelbrot;
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int) {
   SDL_SetMainReady();
   if(SDL_Init(SDL_INIT_VIDEO)!=0) return 1;
   SDL_Window* window=SDL_CreateWindow("OpenMP Mandelbrot",SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);
   if(!window) {SDL_Quit();return 1;}
   SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
   if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
   if(!renderer) {SDL_DestroyWindow(window);SDL_Quit();return 1;}
   Settings cfg;
   cfg.threads=std::max(1,SDL_GetCPUCount());
   SDL_Texture* texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,cfg.width,cfg.height);
   bool running=true,dirty=true;
   Result image;
   while(running) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
         if(event.type==SDL_QUIT) running=false;
         if(event.type==SDL_MOUSEWHEEL) {
            cfg.spanX*=event.wheel.y>0?0.80:1.25;dirty=true;
         }
         if(event.type==SDL_KEYDOWN) {
            auto k=event.key.keysym.sym;
            if(k==SDLK_ESCAPE) running=false;
            if(k==SDLK_1) {cfg.schedule=Schedule::Serial;dirty=true;}
            if(k==SDLK_2) {cfg.schedule=Schedule::Static;dirty=true;}
            if(k==SDLK_3) {cfg.schedule=Schedule::Dynamic;dirty=true;}
            if(k==SDLK_4) {cfg.schedule=Schedule::Guided;dirty=true;}
            if(k==SDLK_LEFT) {cfg.centerX-=cfg.spanX*0.1;dirty=true;}
            if(k==SDLK_RIGHT){cfg.centerX+=cfg.spanX*0.1;dirty=true;}
            if(k==SDLK_UP) {cfg.centerY-=cfg.spanX*0.1*cfg.height/cfg.width;dirty=true;}
            if(k==SDLK_DOWN){cfg.centerY+=cfg.spanX*0.1*cfg.height/cfg.width;dirty=true;}
            if(k==SDLK_PLUS || k==SDLK_EQUALS || k==SDLK_KP_PLUS)
               {cfg.threads=std::min(128,cfg.threads+1);dirty=true;}
            if(k==SDLK_MINUS || k==SDLK_KP_MINUS)
               {cfg.threads=std::max(1,cfg.threads-1);dirty=true;}
         }
      }
      if(dirty) {
         image=Compute(cfg);
         SDL_UpdateTexture(texture,nullptr,image.pixels.data(),cfg.width*4);
         std::string title="OpenMP Mandelbrot | "+std::string(Name(cfg.schedule))+
            " | threads "+std::to_string(image.threadsUsed)+
            " | compute "+std::to_string(static_cast<int>(image.milliseconds))+" ms";
         SDL_SetWindowTitle(window,title.c_str());
         dirty=false;
      }
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer,texture,nullptr,nullptr);
      SDL_RenderPresent(renderer);
      SDL_Delay(16);
   }
   SDL_DestroyTexture(texture);
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   SDL_Quit();
   return 0;
}
