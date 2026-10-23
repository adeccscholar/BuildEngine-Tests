#include "Galaxy.h"
#include <GL/glew.h>
#include <SDL.h>
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace adecc::galaxy;
extern "C" {
   __declspec(dllexport) unsigned long NvOptimusEnablement=1UL;
   __declspec(dllexport) int AmdPowerXpressRequestHighPerformance=1;
}
namespace {
void SDLCALL GuiLog(void*,int,SDL_LogPriority,const char* text) {
   OutputDebugStringA(text ? text : "");
   OutputDebugStringA("\n");
}
GLuint Shader(GLenum type,char const* source) {
   GLuint shader=glCreateShader(type);
   glShaderSource(shader,1,&source,nullptr);
   glCompileShader(shader);
   GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
   if(!ok) {
      char buffer[2048]{};
      glGetShaderInfoLog(shader,sizeof(buffer),nullptr,buffer);
      OutputDebugStringA(buffer);
      glDeleteShader(shader);
      return 0;
   }
   return shader;
}
GLuint Program() {
   constexpr char vertex[]=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 tint;
uniform float tilt;
uniform float spin;
uniform float distance;
uniform float aspect;
out vec3 color;
void main() {
   float c=cos(spin),s=sin(spin),a=cos(tilt),b=sin(tilt);
   vec3 p=vec3(c*position.x-s*position.y,s*position.x+c*position.y,position.z);
   p=vec3(p.x,a*p.y-b*p.z,b*p.y+a*p.z);
   float z=max(0.4,distance-p.z);
   gl_Position=vec4(p.x/(z*aspect),p.y/z,0.5,1.0);
   gl_PointSize=clamp(18.0/z,1.0,10.0);
   color=tint;
}
)GLSL";
   constexpr char fragment[]=R"GLSL(#version 330 core
in vec3 color;
out vec4 pixel;
void main() {
   vec2 d=gl_PointCoord*2.0-1.0;
   float radius=dot(d,d);
   if(radius>1.0) discard;
   float glow=pow(max(0.0,1.0-radius),1.8);
   pixel=vec4(color*glow,glow);
}
)GLSL";
   GLuint vs=Shader(GL_VERTEX_SHADER,vertex);
   GLuint fs=Shader(GL_FRAGMENT_SHADER,fragment);
   if(!vs||!fs) return 0;
   GLuint program=glCreateProgram();
   glAttachShader(program,vs);glAttachShader(program,fs);
   glLinkProgram(program);
   glDeleteShader(vs);glDeleteShader(fs);
   GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
   if(!ok){glDeleteProgram(program);return 0;}
   return program;
}
struct Vertex {float x,y,z,r,g,b;};
void Fill(Galaxy const& galaxy,std::vector<Vertex>& vertices) {
   vertices.resize(galaxy.stars.size());
   for(std::size_t i=0;i<galaxy.stars.size();++i) {
      auto const& s=galaxy.stars[i];
      vertices[i]={s.x,s.y,s.z,
         ((s.color>>16)&255)/255.0f*s.brightness,
         ((s.color>>8)&255)/255.0f*s.brightness,
         (s.color&255)/255.0f*s.brightness};
   }
}
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int) {
   SDL_LogSetOutputFunction(GuiLog,nullptr);
   SDL_SetMainReady();
   if(SDL_Init(SDL_INIT_VIDEO)!=0)return 1;
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_Window* window=SDL_CreateWindow("adecc Scholar - OpenMP Galaxy",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,800,
      SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
   if(!window){SDL_Quit();return 2;}
   SDL_GLContext context=SDL_GL_CreateContext(window);
   if(!context){SDL_DestroyWindow(window);SDL_Quit();return 3;}
   SDL_GL_MakeCurrent(window,context);
   SDL_GL_SetSwapInterval(1);
   glewExperimental=GL_TRUE;
   if(glewInit()!=GLEW_OK){SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 4;}
   glGetError();
   GLuint program=Program();
   if(!program){SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 5;}
   GLuint vao=0,vbo=0;
   glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER,vbo);
   glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
   glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
      reinterpret_cast<void*>(3*sizeof(float)));
   glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
   glEnable(GL_PROGRAM_POINT_SIZE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE);
   Configuration configuration;
   auto galaxy=Initialize(configuration);
   std::vector<Vertex> vertices;
   Schedule schedule=Schedule::Static;
   int threads=(std::min)(8,(std::max)(1,SDL_GetCPUCount()));
   float tilt=0.57f,spin=0.0f,distance=16.0f;
   bool running=true,paused=false,dragging=false;
   Uint64 last=SDL_GetPerformanceCounter();
   double elapsed=0.0, lastSimulationMs=0;
   while(running) {
      SDL_Event e;
      while(SDL_PollEvent(&e)) {
         if(e.type==SDL_QUIT)running=false;
         if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT)dragging=true;
         if(e.type==SDL_MOUSEBUTTONUP && e.button.button==SDL_BUTTON_LEFT)dragging=false;
         if(e.type==SDL_MOUSEMOTION && dragging) {
            spin+=e.motion.xrel*0.005f;
            tilt=(std::clamp)(tilt+e.motion.yrel*0.005f,-1.4f,1.4f);
         }
         if(e.type==SDL_MOUSEWHEEL)
            distance=(std::clamp)(distance*(e.wheel.y>0?0.9f:1.1f),4.0f,75.0f);
         if(e.type==SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
               case SDLK_ESCAPE:running=false;break;
               case SDLK_SPACE:paused=!paused;break;
               case SDLK_1:schedule=Schedule::Serial;break;
               case SDLK_2:schedule=Schedule::Static;break;
               case SDLK_3:schedule=Schedule::Dynamic;break;
               case SDLK_4:schedule=Schedule::Guided;break;
               case SDLK_PLUS:case SDLK_EQUALS:case SDLK_KP_PLUS:threads=(std::min)(128,threads+1);break;
               case SDLK_MINUS:case SDLK_KP_MINUS:threads=(std::max)(1,threads-1);break;
               case SDLK_r:galaxy=Initialize(configuration);break;
               default:break;
            }
         }
      }
      Uint64 const now=SDL_GetPerformanceCounter();
      elapsed+=static_cast<double>(now-last)/SDL_GetPerformanceFrequency();
      last=now;
      // Fixed-step simulation: cap backlog to preserve GUI responsiveness.
      elapsed=(std::min)(elapsed,0.05);
      if(!paused) {
         int steps=0;
         Uint64 const start=SDL_GetPerformanceCounter();
         while(elapsed>=configuration.dt && steps<6) {
            Step(galaxy,schedule,threads);
            elapsed-=configuration.dt;
            ++steps;
         }
         lastSimulationMs=1000.0*(SDL_GetPerformanceCounter()-start)/
            SDL_GetPerformanceFrequency();
      } else elapsed=0;
      Fill(galaxy,vertices);
      glBindBuffer(GL_ARRAY_BUFFER,vbo);
      glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),
         vertices.data(),GL_STREAM_DRAW);
      int w=0,h=0;
      SDL_GL_GetDrawableSize(window,&w,&h);
      glViewport(0,0,w,h);
      glClearColor(0.003f,0.006f,0.021f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      glUseProgram(program);
      glUniform1f(glGetUniformLocation(program,"tilt"),tilt);
      glUniform1f(glGetUniformLocation(program,"spin"),spin);
      glUniform1f(glGetUniformLocation(program,"distance"),distance);
      glUniform1f(glGetUniformLocation(program,"aspect"),static_cast<float>(w)/(std::max)(h,1));
      glBindVertexArray(vao);
      glDrawArrays(GL_POINTS,0,static_cast<GLsizei>(vertices.size()));
      SDL_GL_SwapWindow(window);
      char title[320];
      std::snprintf(title,sizeof(title),
         "OpenMP Galaxy | %zu stars | %s | %d threads | simulation %.2f ms | %s",
         galaxy.stars.size(),ScheduleName(schedule),threads,lastSimulationMs,
         paused?"PAUSED":"running");
      SDL_SetWindowTitle(window,title);
   }
   glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
   SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
   return 0;
}
