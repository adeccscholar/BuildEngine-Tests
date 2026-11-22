#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TensorField.h"

namespace tensor_nebula {

/**
\brief Owns the native OpenGL 3.3 field renderer, fullscreen pass and 3D mesh.
\details Consumes ONLY the CPU xtensor intensity as a floating-point R32F texture.
   Two GLSL programs share the same physical source: a cinematic 2D pass and a
   real depth-tested 3D heightfield. The rendered result is an ImGui texture.
\note Must be destroyed before the owning SDL/OpenGL context.
*/
class NebulaGL final {
   GLuint uFieldTexture {};
   GLuint uFrameTexture {};
   GLuint uFramebuffer {};
   GLuint uDepth {};
   GLuint uTriangleVAO {};
   GLuint uSurfaceVAO {};
   GLuint uSurfaceVBO {};
   GLuint uSurfaceEBO {};
   GLuint uFlatProgram {};
   GLuint uSurfaceProgram {};
   int iTextureWidth {};
   int iTextureHeight {};
   int iFrameWidth {};
   int iFrameHeight {};
   GLsizei iSurfaceIndices {};

   static GLuint Compile(GLenum const eType, char const* const szCode) {
      GLuint const uShader = glCreateShader(eType);
      glShaderSource(uShader, 1, &szCode, nullptr);
      glCompileShader(uShader);
      GLint iOK {};
      glGetShaderiv(uShader, GL_COMPILE_STATUS, &iOK);
      if(!iOK) {
         GLint iLength {};
         glGetShaderiv(uShader, GL_INFO_LOG_LENGTH, &iLength);
         std::string strError(static_cast<std::size_t>(std::max(iLength, 1)), '\0');
         glGetShaderInfoLog(uShader, iLength, nullptr, strError.data());
         glDeleteShader(uShader);
         throw std::runtime_error {"Tensor Nebula shader compilation failed: " + strError};
      }
      return uShader;
   }

   static GLuint Link(char const* const szVertex, char const* const szFragment) {
      GLuint const uVertex = Compile(GL_VERTEX_SHADER, szVertex);
      GLuint const uFragment = Compile(GL_FRAGMENT_SHADER, szFragment);
      GLuint const uProgram = glCreateProgram();
      glAttachShader(uProgram, uVertex);
      glAttachShader(uProgram, uFragment);
      glLinkProgram(uProgram);
      glDeleteShader(uVertex);
      glDeleteShader(uFragment);
      GLint iOK {};
      glGetProgramiv(uProgram, GL_LINK_STATUS, &iOK);
      if(!iOK) {
         GLint iLength {};
         glGetProgramiv(uProgram, GL_INFO_LOG_LENGTH, &iLength);
         std::string strError(static_cast<std::size_t>(std::max(iLength, 1)), '\0');
         glGetProgramInfoLog(uProgram, iLength, nullptr, strError.data());
         glDeleteProgram(uProgram);
         throw std::runtime_error {"Tensor Nebula shader link failed: " + strError};
      }
      return uProgram;
   }

   static char const* FlatVertex() {
      return R"(#version 330 core
         out vec2 vUV;
         void main() {
            vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            vUV = p * 0.5;
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
         })";
   }

   static char const* FlatFragment() {
      return R"(#version 330 core
         in vec2 vUV;
         out vec4 FragColor;
         uniform sampler2D uField;
         uniform int uPalette;
         uniform int uMode;
         uniform float uGlow;
         uniform float uContours;
         uniform float uExposure;
         uniform float uRelief;
         uniform float uZoom;
         uniform vec2 uPan;
         uniform vec2 uTexel;

         vec3 palette(float t) {
            t = clamp(t, 0.0, 1.0);
            if(uPalette == 1)
               return mix(mix(vec3(.012,.035,.095),vec3(.08,.74,.79),smoothstep(0.,.55,t)),
                  vec3(.98,.78,.40),smoothstep(.58,1.,t));
            if(uPalette == 2)
               return mix(mix(vec3(.055,.015,.11),vec3(.83,.12,.38),smoothstep(.05,.63,t)),
                  vec3(1.,.88,.45),smoothstep(.62,1.,t));
            if(uPalette == 3) return vec3(t);
            return mix(mix(vec3(.015,.024,.095),vec3(.17,.37,.91),smoothstep(.03,.47,t)),
               mix(vec3(.44,.08,.84),vec3(1.,.66,.34),smoothstep(.45,1.,t)),
               smoothstep(.3,.95,t));
         }
         float field(vec2 uv) {return texture(uField,clamp(uv,vec2(0.),vec2(1.))).r;}
         void main() {
            vec2 uv = (vUV - .5) / max(uZoom, .1) + .5 + uPan;
            if(any(lessThan(uv,vec2(0.)))||any(greaterThan(uv,vec2(1.)))) {
               FragColor = vec4(.012,.025,.065,1.); return;
            }
            float intensity = field(uv);
            float dx = field(uv + vec2(uTexel.x,0.)) -
                       field(uv - vec2(uTexel.x,0.));
            float dy = field(uv + vec2(0.,uTexel.y)) -
                       field(uv - vec2(0.,uTexel.y));
            float mapped = 1.0 - exp(-max(intensity,0.0)*max(uExposure,.1)*1.8);
            vec3 color = palette(mapped);
            vec3 normal = normalize(vec3(-dx*3.5*uRelief,-dy*3.5*uRelief,1.0));
            float lighting = .34 + .66*max(dot(normal,normalize(vec3(-.55,.7,1.))),0.);
            if(uMode == 2) color *= lighting;
            float contour = 0.0;
            if(uContours > 0.0 || uMode == 1) {
               float n = intensity * 18.0;
               float distanceToLine = abs(fract(n)-.5);
               float aa = max(fwidth(n),.02);
               contour = 1.0-smoothstep(0.,aa*1.6,distanceToLine);
               color += vec3(.30,.70,.95)*contour*max(uContours,uMode==1?1.:0.);
            }
            // Screen-space luminous diffusion: samples of the actual tensor texture.
            vec2 blur = uTexel*vec2(7.0);
            float halo = 0.0;
            for(int i=-1;i<=1;i++) {
               for(int j=-1;j<=1;j++)
                  halo += max(0.,field(uv+vec2(float(i),float(j))*blur)-.5);
            }
            halo /= 9.0;
            color += uGlow * vec3(.26,.42,.77)*halo*2.2;
            color += .11*vec3(.34,.57,.92)*pow(intensity,5.);
            color = pow(max(color,vec3(0.)),vec3(.90));
            FragColor = vec4(color,1.);
         })";
   }

   static char const* SurfaceVertex() {
      return R"(#version 330 core
         layout(location=0) in vec2 aUV;
         out vec2 vUV;
         out float vHeight;
         uniform sampler2D uField;
         uniform float uYaw;
         uniform float uPitch;
         uniform float uDistance;
         uniform float uAspect;
         uniform float uElevation;
         void main() {
            vUV = aUV;
            vHeight = textureLod(uField,aUV,0.).r;
            vec3 p = vec3((aUV.x-.5)*5.2, (aUV.y-.5)*3.6, vHeight*uElevation);
            float cy=cos(uYaw),sy=sin(uYaw);
            float cp=cos(uPitch),sp=sin(uPitch);
            float x = cy*p.x-sy*p.y;
            float y = sy*p.x+cy*p.y;
            float up = cp*p.z+sp*y;
            float forward = cp*y-sp*p.z;
            float depth = max(1.0,uDistance-forward);
            gl_Position=vec4(x*1.6/max(uAspect,.2),up*1.6,depth-1.5,depth);
         })";
   }

   static char const* SurfaceFragment() {
      return R"(#version 330 core
         in vec2 vUV;
         in float vHeight;
         out vec4 FragColor;
         uniform int uPalette;
         uniform float uGlow;
         uniform float uContours;
         vec3 palette(float t) {
            t = clamp(t,0.,1.);
            if(uPalette==1) return mix(vec3(.05,.18,.3),vec3(.34,1.,.72),t);
            if(uPalette==2) return mix(vec3(.19,.03,.16),vec3(1.,.59,.22),t);
            if(uPalette==3) return vec3(t);
            return mix(vec3(.075,.14,.42),mix(vec3(.48,.18,.92),vec3(1.,.76,.45),t),t);
         }
         void main() {
            vec3 dx=dFdx(vec3(vUV*vec2(5.2,3.6),vHeight));
            vec3 dy=dFdy(vec3(vUV*vec2(5.2,3.6),vHeight));
            vec3 normal=normalize(cross(dx,dy));
            float diffuse=abs(dot(normal,normalize(vec3(-.5,.62,.72))));
            vec3 color=palette(vHeight)*(.33+.79*diffuse);
            float n=vHeight*19.;
            float contour=1.-smoothstep(0.,max(fwidth(n)*1.3,.04),abs(fract(n)-.5));
            color+=uContours*contour*vec3(.29,.75,1.);
            color+=uGlow*pow(vHeight,8.)*vec3(.55,.33,.65);
            FragColor=vec4(color,1.);
         })";
   }

   void EnsureFramebuffer(int const iWidth,int const iHeight) {
      int const iW=std::clamp(iWidth,1,4096);
      int const iH=std::clamp(iHeight,1,4096);
      if(uFramebuffer && iFrameWidth==iW && iFrameHeight==iH) return;
      if(uDepth)glDeleteRenderbuffers(1,&uDepth);
      if(uFrameTexture)glDeleteTextures(1,&uFrameTexture);
      if(uFramebuffer)glDeleteFramebuffers(1,&uFramebuffer);
      uFramebuffer=0;uDepth=0;uFrameTexture=0;
      iFrameWidth=iW;iFrameHeight=iH;
      glGenFramebuffers(1,&uFramebuffer);
      glBindFramebuffer(GL_FRAMEBUFFER,uFramebuffer);
      glGenTextures(1,&uFrameTexture);
      glBindTexture(GL_TEXTURE_2D,uFrameTexture);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,iW,iH,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,uFrameTexture,0);
      glGenRenderbuffers(1,&uDepth);
      glBindRenderbuffer(GL_RENDERBUFFER,uDepth);
      glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,iW,iH);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,uDepth);
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
         throw std::runtime_error {"Tensor Nebula framebuffer is incomplete"};
      glBindFramebuffer(GL_FRAMEBUFFER,0);
   }

   void BuildSurfaceMesh() {
      constexpr int iNX=192,iNY=112;
      std::vector<float> vecUV;
      std::vector<std::uint32_t> vecIndex;
      vecUV.reserve((iNX+1)*(iNY+1)*2);
      vecIndex.reserve(iNX*iNY*6);
      for(int iY=0;iY<=iNY;++iY)
         for(int iX=0;iX<=iNX;++iX) {
            vecUV.push_back(static_cast<float>(iX)/iNX);
            vecUV.push_back(static_cast<float>(iY)/iNY);
         }
      for(int iY=0;iY<iNY;++iY)
         for(int iX=0;iX<iNX;++iX) {
            std::uint32_t const uA=iY*(iNX+1)+iX;
            std::uint32_t const uB=uA+1;
            std::uint32_t const uC=uA+(iNX+1);
            std::uint32_t const uD=uC+1;
            vecIndex.insert(vecIndex.end(),{uA,uC,uB,uB,uC,uD});
         }
      iSurfaceIndices=static_cast<GLsizei>(vecIndex.size());
      glGenVertexArrays(1,&uSurfaceVAO);
      glGenBuffers(1,&uSurfaceVBO);
      glGenBuffers(1,&uSurfaceEBO);
      glBindVertexArray(uSurfaceVAO);
      glBindBuffer(GL_ARRAY_BUFFER,uSurfaceVBO);
      glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vecUV.size()*sizeof(float)),
         vecUV.data(),GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,uSurfaceEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
         static_cast<GLsizeiptr>(vecIndex.size()*sizeof(std::uint32_t)),
         vecIndex.data(),GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
      glBindVertexArray(0);
   }

public:
   struct VisualSettings {
      int iPalette {};
      int iMode {};
      float flGlow {.9F};
      float flContours {.24F};
      float flExposure {1.0F};
      float flRelief {1.0F};
      float flZoom {1.0F};
      float flPanX {};
      float flPanY {};
      float flYaw {.55F};
      float flPitch {.55F};
      float flDistance {8.0F};
      float flElevation {1.4F};
   };

   /**
   \brief Allocates real floating-point GPU texture and offscreen rendering passes.
   \details The managed OpenGLShowcase must create its active context first.
   \throw std::runtime_error If shaders fail or native GPU objects cannot be created.
   */
   NebulaGL() {
      uFlatProgram=Link(FlatVertex(),FlatFragment());
      uSurfaceProgram=Link(SurfaceVertex(),SurfaceFragment());
      glGenVertexArrays(1,&uTriangleVAO);
      glGenTextures(1,&uFieldTexture);
      glBindTexture(GL_TEXTURE_2D,uFieldTexture);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
      BuildSurfaceMesh();
   }

   NebulaGL(NebulaGL const&)=delete;
   NebulaGL& operator=(NebulaGL const&)=delete;
   ~NebulaGL() {
      if(uSurfaceEBO)glDeleteBuffers(1,&uSurfaceEBO);
      if(uSurfaceVBO)glDeleteBuffers(1,&uSurfaceVBO);
      if(uSurfaceVAO)glDeleteVertexArrays(1,&uSurfaceVAO);
      if(uTriangleVAO)glDeleteVertexArrays(1,&uTriangleVAO);
      if(uDepth)glDeleteRenderbuffers(1,&uDepth);
      if(uFramebuffer)glDeleteFramebuffers(1,&uFramebuffer);
      if(uFrameTexture)glDeleteTextures(1,&uFrameTexture);
      if(uFieldTexture)glDeleteTextures(1,&uFieldTexture);
      if(uFlatProgram)glDeleteProgram(uFlatProgram);
      if(uSurfaceProgram)glDeleteProgram(uSurfaceProgram);
   }

   /**
   \brief Uploads only the explicitly computed tensor field to the GPU.
   \param theField Source of the physical scalar values.
   \returns Nothing.
   \throw std::runtime_error If the requested array is not two-dimensional.
   */
   void Upload(TensorField<float> const& theField) {
      auto const& theArray=theField.Values();
      if(theArray.dimension()!=2)
         throw std::runtime_error {"Tensor Nebula texture requires a rank-two tensor"};
      int const iNewHeight=static_cast<int>(theArray.shape()[0]);
      int const iNewWidth=static_cast<int>(theArray.shape()[1]);
      bool const bResize=iTextureWidth!=iNewWidth || iTextureHeight!=iNewHeight;
      iTextureWidth=iNewWidth;
      iTextureHeight=iNewHeight;
      glBindTexture(GL_TEXTURE_2D,uFieldTexture);
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);
      if(bResize)
         glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,iTextureWidth,iTextureHeight,0,
            GL_RED,GL_FLOAT,theArray.data());
      else
         glTexSubImage2D(GL_TEXTURE_2D,0,0,0,iTextureWidth,iTextureHeight,
            GL_RED,GL_FLOAT,theArray.data());
   }

   /**
   \brief Renders the current field to a native framebuffer for presentation.
   \param aSettings Palette, camera and scientific overlay controls.
   \param iWidth Destination framebuffer width in drawable pixels.
   \param iHeight Destination framebuffer height in drawable pixels.
   \returns OpenGL color texture ID for the official ImGui renderer.
   \throw std::runtime_error On incomplete GPU state.
   */
   [[nodiscard]] GLuint Render(VisualSettings const& aSettings,
                               int const iWidth,int const iHeight) {
      if(iTextureWidth==0 || iTextureHeight==0)
         throw std::runtime_error {"Tensor Nebula field not uploaded"};
      EnsureFramebuffer(iWidth,iHeight);
      glBindFramebuffer(GL_FRAMEBUFFER,uFramebuffer);
      glViewport(0,0,iFrameWidth,iFrameHeight);
      glClearColor(.009F,.015F,.045F,1.0F);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D,uFieldTexture);
      if(aSettings.iMode==3) {
         glEnable(GL_DEPTH_TEST);
         glUseProgram(uSurfaceProgram);
         glUniform1i(glGetUniformLocation(uSurfaceProgram,"uField"),0);
         glUniform1i(glGetUniformLocation(uSurfaceProgram,"uPalette"),aSettings.iPalette);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uGlow"),aSettings.flGlow);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uContours"),aSettings.flContours);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uYaw"),aSettings.flYaw);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uPitch"),aSettings.flPitch);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uDistance"),aSettings.flDistance);
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uAspect"),
            static_cast<float>(iFrameWidth)/static_cast<float>(iFrameHeight));
         glUniform1f(glGetUniformLocation(uSurfaceProgram,"uElevation"),aSettings.flElevation);
         glBindVertexArray(uSurfaceVAO);
         glDrawElements(GL_TRIANGLES,iSurfaceIndices,GL_UNSIGNED_INT,nullptr);
      }
      else {
         glDisable(GL_DEPTH_TEST);
         glUseProgram(uFlatProgram);
         glUniform1i(glGetUniformLocation(uFlatProgram,"uField"),0);
         glUniform1i(glGetUniformLocation(uFlatProgram,"uPalette"),aSettings.iPalette);
         glUniform1i(glGetUniformLocation(uFlatProgram,"uMode"),aSettings.iMode);
         glUniform1f(glGetUniformLocation(uFlatProgram,"uGlow"),aSettings.flGlow);
         glUniform1f(glGetUniformLocation(uFlatProgram,"uContours"),aSettings.flContours);
         glUniform1f(glGetUniformLocation(uFlatProgram,"uExposure"),aSettings.flExposure);
         glUniform1f(glGetUniformLocation(uFlatProgram,"uRelief"),aSettings.flRelief);
         glUniform1f(glGetUniformLocation(uFlatProgram,"uZoom"),aSettings.flZoom);
         glUniform2f(glGetUniformLocation(uFlatProgram,"uPan"),
            aSettings.flPanX,aSettings.flPanY);
         glUniform2f(glGetUniformLocation(uFlatProgram,"uTexel"),
            1.0F/iTextureWidth,1.0F/iTextureHeight);
         glBindVertexArray(uTriangleVAO);
         glDrawArrays(GL_TRIANGLES,0,3);
      }
      glBindVertexArray(0);
      glUseProgram(0);
      glDisable(GL_DEPTH_TEST);
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      return uFrameTexture;
   }

   /**
   \brief Saves an actual rendered GPU framebuffer, without synthesizing evidence.
   \details SDL BMP output is used to avoid an untracked PNG dependency.
   \param thePath Destination BMP file, for example tensor-nebula.bmp.
   \returns True if SDL saved the captured framebuffer.
   \throw std::runtime_error If a framebuffer has not yet been rendered.
   */
   [[nodiscard]] bool SaveScreenshot(std::filesystem::path const& thePath) const {
      if(!uFramebuffer)throw std::runtime_error {"Nothing has been rendered"};
      std::vector<std::uint8_t> vecPixels(
         static_cast<std::size_t>(iFrameWidth)*iFrameHeight*4);
      glBindFramebuffer(GL_READ_FRAMEBUFFER,uFramebuffer);
      glReadPixels(0,0,iFrameWidth,iFrameHeight,GL_RGBA,GL_UNSIGNED_BYTE,vecPixels.data());
      glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
      SDL_Surface* const pSurface=SDL_CreateRGBSurfaceWithFormat(
         0,iFrameWidth,iFrameHeight,32,SDL_PIXELFORMAT_RGBA32);
      if(!pSurface)return false;
      auto* const pDestination=static_cast<std::uint8_t*>(pSurface->pixels);
      std::size_t const uRowBytes=static_cast<std::size_t>(iFrameWidth)*4;
      for(int iRow=0;iRow<iFrameHeight;++iRow) {
         std::copy_n(vecPixels.data()+static_cast<std::size_t>(iRow)*uRowBytes,
            uRowBytes,pDestination+
            static_cast<std::size_t>(iFrameHeight-iRow-1)*pSurface->pitch);
      }
      int const iSaved=SDL_SaveBMP(pSurface,thePath.string().c_str());
      SDL_FreeSurface(pSurface);
      return iSaved==0;
   }
};
}
