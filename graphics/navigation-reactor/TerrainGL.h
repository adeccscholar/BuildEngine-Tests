#pragma once

#include <stddef.h>
#include <GL/glew.h>
#include <imgui.h>
#include "TerrainView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/**
\brief Dedicated Mesa OpenGL 3.3 core renderer for the Recast input geometry.
\details Draws Recast's original xyz triangles in an offscreen color/depth
         framebuffer and projects the authentic Detour waypoint geometry
         through the identical GPU camera. The result is shown in ImGui.
         No SDL_Renderer or separate generated terrain is involved.
\note OpenGL 3.3 is the minimum runtime contract. Higher Mesa contexts may work.
*/
class TerrainGL final {
   struct Vertex {
      float flX {}, flY {}, flZ {};
      float flNX {}, flNY {}, flNZ {};
      float flArea {};
   };
   GLuint uTerrainProgram {};
   GLuint uRouteProgram {};
   GLuint uTerrainVAO {}, uTerrainVBO {};
   GLuint uRouteVAO {}, uRouteVBO {};
   GLuint uFramebuffer {}, uColor {}, uDepth {};
   int iWidth {}, iHeight {};
   GLsizei iTerrainVertices {};
   GLsizei iRouteVertices {};
   std::vector<std::array<float, 3>> vecRoutePoints;

   static GLuint Compile(GLenum eType, char const* szCode) {
      GLuint const u = glCreateShader(eType);
      glShaderSource(u, 1, &szCode, nullptr);
      glCompileShader(u);
      GLint iPassed {};
      glGetShaderiv(u, GL_COMPILE_STATUS, &iPassed);
      if(!iPassed) {
         GLint iLen {};
         glGetShaderiv(u, GL_INFO_LOG_LENGTH, &iLen);
         std::string strError(static_cast<std::size_t>(std::max(1, iLen)), '\0');
         glGetShaderInfoLog(u, iLen, nullptr, strError.data());
         glDeleteShader(u);
         throw std::runtime_error("Terrain GLSL compilation failed: " + strError);
      }
      return u;
   }

   static GLuint Link(char const* szVertex, char const* szFragment) {
      GLuint const uVS = Compile(GL_VERTEX_SHADER, szVertex);
      GLuint const uFS = Compile(GL_FRAGMENT_SHADER, szFragment);
      GLuint const uProgram = glCreateProgram();
      glAttachShader(uProgram, uVS);
      glAttachShader(uProgram, uFS);
      glLinkProgram(uProgram);
      glDeleteShader(uVS);
      glDeleteShader(uFS);
      GLint iPassed {};
      glGetProgramiv(uProgram, GL_LINK_STATUS, &iPassed);
      if(!iPassed) {
         GLint iLen {};
         glGetProgramiv(uProgram, GL_INFO_LOG_LENGTH, &iLen);
         std::string strError(static_cast<std::size_t>(std::max(1, iLen)), '\0');
         glGetProgramInfoLog(uProgram, iLen, nullptr, strError.data());
         glDeleteProgram(uProgram);
         throw std::runtime_error("Terrain GL program link failed: " + strError);
      }
      return uProgram;
   }

   static char const* VertexShader() {
      return R"(#version 330 core
         layout(location=0) in vec3 aPosition;
         layout(location=1) in vec3 aNormal;
         layout(location=2) in float aArea;
         uniform float uYaw;
         uniform float uPitch;
         uniform float uDistance;
         uniform vec2 uScale;
         out vec3 vWorld;
         out vec3 vNormal;
         out float vArea;
         void main() {
            vec3 p=aPosition-vec3(8.0,0.0,5.0);
            float cy=cos(uYaw), sy=sin(uYaw);
            float ce=cos(uPitch), se=sin(uPitch);
            float right=p.x*cy-p.z*sy;
            float forward=p.x*sy+p.z*cy;
            float vertical=p.y*ce-forward*se;
            float depth=uDistance-forward*ce-p.y*se;
            gl_Position=vec4(right*uScale.x,vertical*uScale.y,depth*1.01-2.0,depth);
            vWorld=aPosition;
            vNormal=aNormal;
            vArea=aArea;
         })";
   }

   static void SetCamera(GLuint const uProgram, TerrainCamera const& theCamera,
                         int const iWidth, int const iHeight) {
      glUseProgram(uProgram);
      glUniform1f(glGetUniformLocation(uProgram, "uYaw"), theCamera.flAzimuth);
      glUniform1f(glGetUniformLocation(uProgram, "uPitch"), theCamera.flElevation);
      glUniform1f(glGetUniformLocation(uProgram, "uDistance"), theCamera.flDistance);
      float const flM = static_cast<float>(std::min(iWidth, iHeight));
      glUniform2f(glGetUniformLocation(uProgram, "uScale"),
         3.2F*flM/static_cast<float>(iWidth),
         3.2F*flM/static_cast<float>(iHeight));
   }

   void EnsureFramebuffer(int const iNewWidth, int const iNewHeight) {
      int const iW = std::clamp(iNewWidth, 1, 4096);
      int const iH = std::clamp(iNewHeight, 1, 4096);
      if(uFramebuffer && iW == iWidth && iH == iHeight) return;
      if(uDepth) glDeleteRenderbuffers(1, &uDepth);
      if(uColor) glDeleteTextures(1, &uColor);
      if(uFramebuffer) glDeleteFramebuffers(1, &uFramebuffer);
      uDepth = uColor = uFramebuffer = 0;
      iWidth = iW;
      iHeight = iH;
      glGenFramebuffers(1, &uFramebuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, uFramebuffer);
      glGenTextures(1, &uColor);
      glBindTexture(GL_TEXTURE_2D, uColor);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iWidth, iHeight, 0,
         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
         GL_TEXTURE_2D, uColor, 0);
      glGenRenderbuffers(1, &uDepth);
      glBindRenderbuffer(GL_RENDERBUFFER, uDepth);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, iWidth, iHeight);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
         GL_RENDERBUFFER, uDepth);
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
         throw std::runtime_error("Terrain framebuffer incomplete");
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }

public:
   TerrainGL() {
      char const* szTerrainFS = R"(#version 330 core
         in vec3 vWorld;
         in vec3 vNormal;
         in float vArea;
         out vec4 FragColor;
         void main() {
            vec3 n=normalize(vNormal);
            vec3 sun=normalize(vec3(-0.42,0.82,0.38));
            float diffuse=max(dot(n,sun),0.0);
            float light=0.24+0.76*diffuse;
            float elevation=clamp((vWorld.y+1.6)/4.0,0.0,1.0);
            vec3 grass=mix(vec3(0.07,0.30,0.28),vec3(0.36,0.66,0.51),elevation);
            vec3 stone=mix(vec3(0.37,0.17,0.24),vec3(0.77,0.40,0.39),elevation);
            vec3 base=mix(stone,grass,step(0.5,vArea));
            float ridge=pow(max(dot(n,sun),0.0),9.0)*0.13;
            FragColor=vec4(base*light+ridge,1.0);
         })";
      char const* szRouteFS = R"(#version 330 core
         in vec3 vWorld;
         in vec3 vNormal;
         in float vArea;
         uniform vec4 uColor;
         out vec4 FragColor;
         void main() { FragColor=uColor; }
         )";
      uTerrainProgram = Link(VertexShader(), szTerrainFS);
      uRouteProgram = Link(VertexShader(), szRouteFS);
      glGenVertexArrays(1, &uTerrainVAO);
      glGenBuffers(1, &uTerrainVBO);
      glBindVertexArray(uTerrainVAO);
      glBindBuffer(GL_ARRAY_BUFFER, uTerrainVBO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flX)));
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flNX)));
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flArea)));
      glGenVertexArrays(1, &uRouteVAO);
      glGenBuffers(1, &uRouteVBO);
      glBindVertexArray(uRouteVAO);
      glBindBuffer(GL_ARRAY_BUFFER, uRouteVBO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flX)));
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flNX)));
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
         reinterpret_cast<void const*>(offsetof(Vertex, flArea)));
      glBindVertexArray(0);
   }

   TerrainGL(TerrainGL const&) = delete;
   TerrainGL& operator=(TerrainGL const&) = delete;

   ~TerrainGL() {
      if(uDepth) glDeleteRenderbuffers(1, &uDepth);
      if(uColor) glDeleteTextures(1, &uColor);
      if(uFramebuffer) glDeleteFramebuffers(1, &uFramebuffer);
      if(uTerrainVBO) glDeleteBuffers(1, &uTerrainVBO);
      if(uRouteVBO) glDeleteBuffers(1, &uRouteVBO);
      if(uTerrainVAO) glDeleteVertexArrays(1, &uTerrainVAO);
      if(uRouteVAO) glDeleteVertexArrays(1, &uRouteVAO);
      if(uTerrainProgram) glDeleteProgram(uTerrainProgram);
      if(uRouteProgram) glDeleteProgram(uRouteProgram);
   }

   /**
   \brief Uploads the single authoritative Recast terrain triangle mesh.
   \details Uploads triangle normals and Recast walkability flags for shading.
   \param vecVertices Recast input xyz coordinates.
   \param vecTriangles Recast input triangle indices.
   \param vecArea Recast-calculated area per triangle.
   \returns Nothing.
   \throw std::runtime_error On malformed vertex indices.
   */
   void SetTerrain(std::vector<float> const& vecVertices,
      std::vector<int> const& vecTriangles,
      std::vector<unsigned char> const& vecArea) {
      std::vector<Vertex> vecGPU;
      vecGPU.reserve(vecTriangles.size());
      for(std::size_t u = 0; u + 2 < vecTriangles.size(); u += 3) {
         std::array<Vertex, 3> arr {};
         for(int j = 0; j < 3; ++j) {
            int const i = vecTriangles[u+static_cast<std::size_t>(j)];
            if(i < 0 || static_cast<std::size_t>(i)*3+2 >= vecVertices.size())
               throw std::runtime_error("Recast terrain index outside vertex array");
            arr[static_cast<std::size_t>(j)].flX = vecVertices[i*3];
            arr[static_cast<std::size_t>(j)].flY = vecVertices[i*3+1];
            arr[static_cast<std::size_t>(j)].flZ = vecVertices[i*3+2];
         }
         float const flUX=arr[1].flX-arr[0].flX, flUY=arr[1].flY-arr[0].flY,
            flUZ=arr[1].flZ-arr[0].flZ;
         float const flVX=arr[2].flX-arr[0].flX, flVY=arr[2].flY-arr[0].flY,
            flVZ=arr[2].flZ-arr[0].flZ;
         // Recast terrain indexing uses a clockwise XZ winding.
         float const flNX=flUY*flVZ-flUZ*flVY;
         float const flNY=flUZ*flVX-flUX*flVZ;
         float const flNZ=flUX*flVY-flUY*flVX;
         float const flLen=std::max(0.00001F,std::sqrt(flNX*flNX+flNY*flNY+flNZ*flNZ));
         for(Vertex& theV : arr) {
            theV.flNX=flNX/flLen;
            theV.flNY=flNY/flLen;
            theV.flNZ=flNZ/flLen;
            theV.flArea=(u/3 < vecArea.size() && vecArea[u/3] != 0) ? 1.0F : 0.0F;
            vecGPU.push_back(theV);
         }
      }
      iTerrainVertices = static_cast<GLsizei>(vecGPU.size());
      glBindBuffer(GL_ARRAY_BUFFER, uTerrainVBO);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vecGPU.size()*sizeof(Vertex)),
         vecGPU.data(), GL_STATIC_DRAW);
   }

   /**
   \brief Uploads the actual Detour straight-path output.
   \details Keeps native xyz elevations; no substitute pathfinding is performed.
   \param vecPoints Ordered waypoints provided by Detour.
   \returns Nothing.
   \throw Nothing intentionally.
   */
   void SetRoute(std::vector<std::array<float,3>> const& vecPoints) {
      std::vector<Vertex> vecGPU;
      for(auto const& arr : vecPoints)
         vecGPU.push_back({arr[0], arr[1]+0.06F, arr[2], 0,1,0,1});
      vecRoutePoints = vecPoints;
      iRouteVertices = static_cast<GLsizei>(vecGPU.size());
      // Reserve one additional vertex for the blue animated agent.
      if(!vecGPU.empty()) vecGPU.push_back(vecGPU.front());
      glBindBuffer(GL_ARRAY_BUFFER, uRouteVBO);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vecGPU.size()*sizeof(Vertex)),
         vecGPU.data(), GL_DYNAMIC_DRAW);
   }

   /**
   \brief Renders an offscreen depth-tested scene.
   \details Terrain and route share the GPU view; GUI draws only its result texture.
   \param iViewWidth Viewport width.
   \param iViewHeight Viewport height.
   \param theCamera Orbit camera.
   \param flPhase Agent progress on the Detour path.
   \param bWireframe Whether to draw the terrain as wireframe.
   \returns Color texture ID for the ImGui viewport.
   \throw std::runtime_error If the framebuffer is incomplete.
   */
   [[nodiscard]] GLuint Render(int const iViewWidth, int const iViewHeight,
      TerrainCamera const& theCamera, float const flPhase, bool const bWireframe) {
      EnsureFramebuffer(iViewWidth, iViewHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, uFramebuffer);
      glViewport(0, 0, iWidth, iHeight);
      glClearColor(0.025F, 0.045F, 0.09F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glDisable(GL_BLEND);
      glDisable(GL_CULL_FACE);
      SetCamera(uTerrainProgram, theCamera, iWidth, iHeight);
      glBindVertexArray(uTerrainVAO);
      if(bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDrawArrays(GL_TRIANGLES, 0, iTerrainVertices);
      if(bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      if(iRouteVertices >= 2) {
         SetCamera(uRouteProgram, theCamera, iWidth, iHeight);
         glUniform4f(glGetUniformLocation(uRouteProgram, "uColor"), 1.0F,0.81F,0.22F,1.0F);
         glBindVertexArray(uRouteVAO);
         glLineWidth(3.0F);
         glDrawArrays(GL_LINE_STRIP, 0, iRouteVertices);
         glPointSize(10.0F);
         glDrawArrays(GL_POINTS, 0, 1);
         glDrawArrays(GL_POINTS, iRouteVertices-1, 1);
         float flTotal = 0.0F;
         for(std::size_t u = 1; u < vecRoutePoints.size(); ++u)
            flTotal += std::hypot(
               vecRoutePoints[u][0] - vecRoutePoints[u-1][0],
               vecRoutePoints[u][2] - vecRoutePoints[u-1][2]);
         float flRemain = flTotal * std::clamp(flPhase, 0.0F, 1.0F);
         std::array<float, 3> arrAgent = vecRoutePoints.front();
         for(std::size_t u = 1; u < vecRoutePoints.size(); ++u) {
            auto const& arrA = vecRoutePoints[u-1];
            auto const& arrB = vecRoutePoints[u];
            float const flLength = std::hypot(arrB[0]-arrA[0], arrB[2]-arrA[2]);
            if(flRemain <= flLength) {
               float const flT = flLength > 0.0F ? flRemain/flLength : 0.0F;
               for(int j = 0; j < 3; ++j)
                  arrAgent[static_cast<std::size_t>(j)] =
                     arrA[static_cast<std::size_t>(j)] +
                     (arrB[static_cast<std::size_t>(j)] - arrA[static_cast<std::size_t>(j)])*flT;
               break;
            }
            flRemain -= flLength;
            arrAgent = arrB;
         }
         Vertex const theAgent {arrAgent[0], arrAgent[1]+0.16F, arrAgent[2],0,1,0,1};
         glBindBuffer(GL_ARRAY_BUFFER, uRouteVBO);
         glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(iRouteVertices)*sizeof(Vertex),
            sizeof(Vertex), &theAgent);
         glUniform4f(glGetUniformLocation(uRouteProgram, "uColor"), 0.24F,0.53F,1.0F,1.0F);
         glPointSize(16.0F);
         glDrawArrays(GL_POINTS, iRouteVertices, 1);
      }
      glBindVertexArray(0);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      return uColor;
   }
};
