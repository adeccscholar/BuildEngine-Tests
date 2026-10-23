// SPDX-License-Identifier: MIT
#include "GeoData.h"
#include <GL/glew.h>
#include <SDL.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>
using namespace adecc::geo;
namespace {
struct Vertex {float x,y,z,h;};
GLuint Shader(GLenum type,char const* source) {
   GLuint shader=glCreateShader(type);
   glShaderSource(shader,1,&source,nullptr);
   glCompileShader(shader);
   GLint status=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
   if(!status) {
      char log[2048]{};
      glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
      OutputDebugStringA(log);glDeleteShader(shader);return 0;
   }
   return shader;
}
GLuint CreateProgram() {
   char const* vertex=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in float heightValue;
uniform float yaw;
uniform float pitch;
uniform float distance;
uniform float aspect;
out float heightColor;
out vec3 normalColor;
void main() {
   float cy=cos(yaw),sy=sin(yaw),cp=cos(pitch),sp=sin(pitch);
   // World axes: x=East, y=North, z=address-density height (up).
   vec3 p=vec3(position.x*cy-position.y*sy,position.x*sy+position.y*cy,position.z);
   float screenY=p.y*cp+p.z*sp;
   float viewDepth=p.y*sp-p.z*cp;
   float depth=max(1.0,distance-viewDepth);
   float nearPlane=0.1,farPlane=250.0;
   float clipZ=((farPlane+nearPlane)/(farPlane-nearPlane))*depth-
      (2.0*farPlane*nearPlane)/(farPlane-nearPlane);
   gl_Position=vec4(1.7*p.x/aspect,1.7*screenY,clipZ,depth);
   heightColor=heightValue;
   normalColor=position;
}
)GLSL";
   char const* fragment=R"GLSL(#version 330 core
in float heightColor;
in vec3 normalColor;
out vec4 pixel;
void main() {
   float h=clamp(heightColor,0.0,1.0);
   vec3 low=vec3(0.035,0.15,0.30);
   vec3 mid=vec3(0.12,0.66,0.61);
   vec3 high=vec3(0.97,0.68,0.27);
   vec3 top=vec3(0.99,0.88,0.69);
   vec3 c=h<0.40?mix(low,mid,h/0.4):
      h<0.80?mix(mid,high,(h-0.4)/0.4):mix(high,top,(h-0.8)/0.2);
   pixel=vec4(c,1.0);
}
)GLSL";
   GLuint vs=Shader(GL_VERTEX_SHADER,vertex),fs=Shader(GL_FRAGMENT_SHADER,fragment);
   if(!vs||!fs)return 0;
   GLuint prog=glCreateProgram();
   glAttachShader(prog,vs);glAttachShader(prog,fs);glLinkProgram(prog);
   glDeleteShader(vs);glDeleteShader(fs);
   GLint status=0;glGetProgramiv(prog,GL_LINK_STATUS,&status);
   if(!status){glDeleteProgram(prog);return 0;}
   return prog;
}
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int) {
   int argc=0;
   LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
   std::filesystem::path initial;
   if(argv && argc>1)initial=argv[1];
   if(argv)LocalFree(argv);
   Dataset dataset;
   std::filesystem::path currentPath;
   Bounds bounds{};
   constexpr int gridW=256,gridH=192;
   Terrain terrain;
   std::vector<Vertex> vertices;
   std::vector<std::uint32_t> indices;
   indices.reserve((gridW-1)*(gridH-1)*6);
   for(int y=0;y<gridH-1;++y)for(int x=0;x<gridW-1;++x) {
      auto const a=static_cast<std::uint32_t>(y*gridW+x);
      indices.insert(indices.end(),{a,a+gridW,a+1,a+1,a+gridW,a+gridW+1});
   }
   auto build=[&](std::filesystem::path const& source) {
      bool const directory=std::filesystem::is_directory(source);
      Dataset next=directory?LoadDirectory(source.string(),true):Load(source.string(),true);
      Bounds const nextBounds=ProjectBounds(next.points,true);
      Terrain nextTerrain=BuildTerrain(next.points,nextBounds,gridW,gridH,2.8,
         Method::OpenMP,8);
      std::vector<Vertex> nextVertices;
      nextVertices.reserve(gridW*gridH);
      for(int y=0;y<gridH;++y)for(int x=0;x<gridW;++x) {
         float const normalized=nextTerrain.peak>0?
            nextTerrain.heightField[static_cast<std::size_t>(y)*gridW+x]/nextTerrain.peak:0;
         nextVertices.push_back({(x/(gridW-1.0f)-0.5f)*16.0f,
            (0.5f-y/(gridH-1.0f))*12.0f,
            5.0f*std::sqrt((std::max)(0.0f,normalized)),normalized});
      }
      dataset=std::move(next);
      bounds=nextBounds;
      terrain=std::move(nextTerrain);
      vertices=std::move(nextVertices);
      currentPath=source;
   };
   if(!initial.empty()) {
      try {build(initial);}
      catch(std::exception const& e) {MessageBoxA(nullptr,e.what(),"Loading failed",MB_ICONERROR);}
   }
   SDL_SetMainReady();
   if(SDL_Init(SDL_INIT_VIDEO)!=0)return 4;
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
   SDL_Window* window=SDL_CreateWindow("GeoSpatial Density Terrain",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,850,
      SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
   if(!window){SDL_Quit();return 5;}
   SDL_GLContext context=SDL_GL_CreateContext(window);
   if(!context){SDL_DestroyWindow(window);SDL_Quit();return 6;}
   glewExperimental=GL_TRUE;
   if(glewInit()!=GLEW_OK){SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 7;}
   glGetError();
   GLuint program=CreateProgram();
   if(!program){SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 8;}
   GLuint vao=0,vbo=0,ibo=0;
   glGenVertexArrays(1,&vao);glBindVertexArray(vao);
   glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
   glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_DYNAMIC_DRAW);
   glGenBuffers(1,&ibo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(std::uint32_t),indices.data(),GL_STATIC_DRAW);
   glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
   glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),
      reinterpret_cast<void*>(3*sizeof(float)));
   glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
   glEnable(GL_DEPTH_TEST);
   float yaw=0.22f,pitch=0.58f,distance=24.0f;
   bool running=true,dragging=false;
   std::string renderer=reinterpret_cast<char const*>(glGetString(GL_RENDERER));
   auto upload=[&]() {
      glBindBuffer(GL_ARRAY_BUFFER,vbo);
      glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),
         vertices.data(),GL_DYNAMIC_DRAW);
      char title[512];
      std::snprintf(title,sizeof(title),
         "GeoSpatial Terrain | %zu addresses | %s | %.1f ms | F=flat D=directory L=reload | %s",
         dataset.points.size(),currentPath.empty()?"No data":
         std::filesystem::is_directory(currentPath)?"hierarchy":"flat file",
         terrain.buildMs,renderer.c_str());
      SDL_SetWindowTitle(window,title);
   };
   upload();
   auto selectFile=[&]() -> std::filesystem::path {
      wchar_t filename[32768]{};
      OPENFILENAMEW dialog{};
      dialog.lStructSize=sizeof(dialog);dialog.lpstrFile=filename;
      dialog.nMaxFile=32768;
      dialog.lpstrFilter=L"CompareTest data (*.dat)\0*.dat\0All files\0*.*\0";
      dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
      return GetOpenFileNameW(&dialog)?std::filesystem::path(filename):std::filesystem::path{};
   };
   auto selectDirectory=[&]() -> std::filesystem::path {
      BROWSEINFOW info{};
      info.lpszTitle=L"Select the CompareTest hierarchy ROOT (contains city directories)";
      info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
      PIDLIST_ABSOLUTE selected=SHBrowseForFolderW(&info);
      if(!selected)return {};
      wchar_t name[MAX_PATH]{};
      bool const ok=SHGetPathFromIDListW(selected,name)!=FALSE;
      CoTaskMemFree(selected);
      return ok?std::filesystem::path(name):std::filesystem::path{};
   };
   SDL_GL_SetSwapInterval(1);
   SDL_EventState(SDL_DROPFILE,SDL_ENABLE);
   while(running) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
         if(event.type==SDL_DROPFILE) {
            try {build(std::filesystem::path(event.drop.file));upload();}
            catch(std::exception const& e){MessageBoxA(nullptr,e.what(),"Load failed",MB_ICONERROR);}
            SDL_free(event.drop.file);
         }
         if(event.type==SDL_QUIT)running=false;
         if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT)dragging=true;
         if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT)dragging=false;
         if(event.type==SDL_MOUSEMOTION&&dragging) {
            yaw+=event.motion.xrel*0.008f;
            pitch=(std::clamp)(pitch+event.motion.yrel*0.006f,0.0f,1.48f);
         }
         if(event.type==SDL_MOUSEWHEEL&&event.wheel.y)
            distance=(std::clamp)(distance*std::pow(0.88f,static_cast<float>(event.wheel.y)),9.0f,90.0f);
         if(event.type==SDL_KEYDOWN) {
            if(event.key.keysym.sym==SDLK_ESCAPE)running=false;
            if(event.key.keysym.sym==SDLK_f || event.key.keysym.sym==SDLK_d ||
               event.key.keysym.sym==SDLK_l) {
               try {
                  auto const path=event.key.keysym.sym==SDLK_f?selectFile():
                     event.key.keysym.sym==SDLK_d?selectDirectory():currentPath;
                  if(!path.empty()){build(path);upload();}
               } catch(std::exception const& e) {
                  MessageBoxA(nullptr,e.what(),"Load failed",MB_ICONERROR);
               }
            }
            if(event.key.keysym.sym==SDLK_1){yaw=0;pitch=0.0f;}
            if(event.key.keysym.sym==SDLK_2){yaw=0;pitch=1.42f;}
            if(event.key.keysym.sym==SDLK_3){yaw=0.22f;pitch=0.58f;distance=24;}
         }
      }
      int w=0,h=0;SDL_GL_GetDrawableSize(window,&w,&h);
      glViewport(0,0,w,h);
      glClearColor(0.008f,0.014f,0.036f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glUseProgram(program);
      glUniform1f(glGetUniformLocation(program,"yaw"),yaw);
      glUniform1f(glGetUniformLocation(program,"pitch"),pitch);
      glUniform1f(glGetUniformLocation(program,"distance"),distance);
      glUniform1f(glGetUniformLocation(program,"aspect"),
         static_cast<float>(w)/(std::max)(1,h));
      glBindVertexArray(vao);
      glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(indices.size()),GL_UNSIGNED_INT,nullptr);
      SDL_GL_SwapWindow(window);
   }
   glDeleteBuffers(1,&ibo);glDeleteBuffers(1,&vbo);
   glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
   SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
   return 0;
}
