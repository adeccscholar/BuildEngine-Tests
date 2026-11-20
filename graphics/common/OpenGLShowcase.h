#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/**
\brief Owns SDL2/WGL, Mesa OpenGL and a dedicated ImGui GPU renderer.
\details This adapter is independent of NativeShowcase's SDL_Renderer.
         It renders dynamic ImGui atlas pages and all indexed ImDrawList geometry
         using one OpenGL core-profile shader and VBO/IBO/VAO.
\note Use an explicitly selected Mesa OpenGL DLL rather than an incidental host SDK.
*/
class OpenGLShowcase final {
   SDL_Window* pWindow {};
   SDL_GLContext pContext {};
   ImGuiContext* pGui {};
   GLuint uProgram {}, uVAO {}, uVBO {}, uEBO {};
   GLint iProjection {}, iTexture {};
   std::uint64_t uLastTick {};
   bool bRunning {true};
   std::vector<GLuint> vecTextures;

   static GLuint CompileShader(GLenum const eType, char const* const szSource) {
      GLuint const uShader = glCreateShader(eType);
      glShaderSource(uShader, 1, &szSource, nullptr);
      glCompileShader(uShader);
      GLint iSuccess {};
      glGetShaderiv(uShader, GL_COMPILE_STATUS, &iSuccess);
      if(!iSuccess) {
         GLint iLength {};
         glGetShaderiv(uShader, GL_INFO_LOG_LENGTH, &iLength);
         std::string strLog(static_cast<std::size_t>(std::max(iLength, 1)), '\0');
         glGetShaderInfoLog(uShader, iLength, nullptr, strLog.data());
         glDeleteShader(uShader);
         throw std::runtime_error("GLSL compile: " + strLog);
      }
      return uShader;
   }

   void InitializeRenderer() {
      char const* const szVertex = R"(#version 330 core
         layout(location=0) in vec2 Position;
         layout(location=1) in vec2 UV;
         layout(location=2) in vec4 Color;
         uniform mat4 Projection;
         out vec2 TexCoord;
         out vec4 VertColor;
         void main() {
            TexCoord=UV;
            VertColor=Color;
            gl_Position=Projection*vec4(Position,0.0,1.0);
         })";
      char const* const szFragment = R"(#version 330 core
         in vec2 TexCoord;
         in vec4 VertColor;
         uniform sampler2D Texture;
         out vec4 FragColor;
         void main() { FragColor=VertColor*texture(Texture,TexCoord); }
         )";
      GLuint const uVS = CompileShader(GL_VERTEX_SHADER, szVertex);
      GLuint const uFS = CompileShader(GL_FRAGMENT_SHADER, szFragment);
      uProgram = glCreateProgram();
      glAttachShader(uProgram, uVS);
      glAttachShader(uProgram, uFS);
      glLinkProgram(uProgram);
      glDeleteShader(uVS);
      glDeleteShader(uFS);
      GLint iLinked {};
      glGetProgramiv(uProgram, GL_LINK_STATUS, &iLinked);
      if(!iLinked) throw std::runtime_error("ImGui OpenGL shader link failed");
      iProjection = glGetUniformLocation(uProgram, "Projection");
      iTexture = glGetUniformLocation(uProgram, "Texture");
      glGenVertexArrays(1, &uVAO);
      glGenBuffers(1, &uVBO);
      glGenBuffers(1, &uEBO);
      glBindVertexArray(uVAO);
      glBindBuffer(GL_ARRAY_BUFFER, uVBO);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uEBO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
         reinterpret_cast<void const*>(offsetof(ImDrawVert, pos)));
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
         reinterpret_cast<void const*>(offsetof(ImDrawVert, uv)));
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
         reinterpret_cast<void const*>(offsetof(ImDrawVert, col)));
      glBindVertexArray(0);
   }

   void UpdateTexture(ImTextureData* const pTexture) {
      GLuint uTexture = static_cast<GLuint>(reinterpret_cast<std::uintptr_t>(pTexture->TexID));
      if(pTexture->Status == ImTextureStatus_WantCreate) {
         glGenTextures(1, &uTexture);
         glBindTexture(GL_TEXTURE_2D, uTexture);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pTexture->Width,
            pTexture->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pTexture->GetPixels());
         pTexture->SetTexID(static_cast<ImTextureID>(uTexture));
         vecTextures.push_back(uTexture);
         pTexture->SetStatus(ImTextureStatus_OK);
      }
      else if(pTexture->Status == ImTextureStatus_WantUpdates) {
         glBindTexture(GL_TEXTURE_2D, uTexture);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         // Source atlas stride can exceed the partial rectangle width.
         glPixelStorei(GL_UNPACK_ROW_LENGTH, pTexture->Width);
         for(ImTextureRect const& aRect : pTexture->Updates)
            glTexSubImage2D(GL_TEXTURE_2D, 0, aRect.x, aRect.y, aRect.w,
               aRect.h, GL_RGBA, GL_UNSIGNED_BYTE,
               pTexture->GetPixelsAt(aRect.x, aRect.y));
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
         pTexture->SetStatus(ImTextureStatus_OK);
      }
      else if(pTexture->Status == ImTextureStatus_WantDestroy) {
         glDeleteTextures(1, &uTexture);
         std::erase(vecTextures, uTexture);
         pTexture->SetTexID(ImTextureID_Invalid);
         pTexture->SetStatus(ImTextureStatus_Destroyed);
      }
   }

   void RenderGUI() {
      ImDrawData* const pData = ImGui::GetDrawData();
      if(!pData) return;
      if(pData->Textures) {
         for(ImTextureData* const pTexture : *pData->Textures)
            if(pTexture->Status != ImTextureStatus_OK) UpdateTexture(pTexture);
      }
      int iWidth {}, iHeight {};
      SDL_GL_GetDrawableSize(pWindow, &iWidth, &iHeight);
      float const flWidth = pData->DisplaySize.x;
      float const flHeight = pData->DisplaySize.y;
      float const arrProjection[4][4] = {
         {2.0F/flWidth, 0, 0, 0},
         {0, -2.0F/flHeight, 0, 0},
         {0, 0, -1, 0},
         {-1.0F, 1.0F, 0, 1.0F}
      };
      glViewport(0, 0, iWidth, iHeight);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glEnable(GL_BLEND);
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_SCISSOR_TEST);
      glUseProgram(uProgram);
      glUniform1i(iTexture, 0);
      glUniformMatrix4fv(iProjection, 1, GL_FALSE, &arrProjection[0][0]);
      glBindVertexArray(uVAO);
      for(ImDrawList const* const pList : pData->CmdLists) {
         glBindBuffer(GL_ARRAY_BUFFER, uVBO);
         glBufferData(GL_ARRAY_BUFFER, pList->VtxBuffer.Size * sizeof(ImDrawVert),
            pList->VtxBuffer.Data, GL_STREAM_DRAW);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uEBO);
         glBufferData(GL_ELEMENT_ARRAY_BUFFER, pList->IdxBuffer.Size * sizeof(ImDrawIdx),
            pList->IdxBuffer.Data, GL_STREAM_DRAW);
         for(ImDrawCmd const& aCmd : pList->CmdBuffer) {
            if(aCmd.UserCallback) {
               if(aCmd.UserCallback == ImDrawCallback_ResetRenderState) continue;
               aCmd.UserCallback(pList, &aCmd);
               continue;
            }
            ImVec4 const aClip = aCmd.ClipRect;
            int const iLeft = static_cast<int>((aClip.x - pData->DisplayPos.x) * pData->FramebufferScale.x);
            int const iTop = static_cast<int>((aClip.y - pData->DisplayPos.y) * pData->FramebufferScale.y);
            int const iRight = static_cast<int>((aClip.z - pData->DisplayPos.x) * pData->FramebufferScale.x);
            int const iBottom = static_cast<int>((aClip.w - pData->DisplayPos.y) * pData->FramebufferScale.y);
            if(iRight <= iLeft || iBottom <= iTop) continue;
            glScissor(iLeft, iHeight - iBottom, iRight - iLeft, iBottom - iTop);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<std::uintptr_t>(aCmd.GetTexID())));
            glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(aCmd.ElemCount),
               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
               reinterpret_cast<void const*>(static_cast<std::uintptr_t>(aCmd.IdxOffset * sizeof(ImDrawIdx))),
               static_cast<GLint>(aCmd.VtxOffset));
         }
      }
      glBindVertexArray(0);
      glDisable(GL_SCISSOR_TEST);
   }

public:
   /**
   \brief Opens an SDL2 window with Mesa OpenGL context and ImGui.
   \details Mesa must be selected before SDL creates its WGL context.
   \param svTitle Native window title.
   \param strMesaDLL Absolute installed Mesa opengl32.dll path.
   \param iWidth Initial window width.
   \param iHeight Initial window height.
   \throw std::runtime_error On SDL, OpenGL or ImGui initialization failures.
   */
   explicit OpenGLShowcase(std::string_view const svTitle,
      std::string const& strMesaDLL, int const iWidth = 1540, int const iHeight = 900) {
      if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
         throw std::runtime_error(SDL_GetError());
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
      if(SDL_GL_LoadLibrary(strMesaDLL.c_str()) != 0)
         throw std::runtime_error("Managed Mesa WGL load failed: " + std::string(SDL_GetError()));
      pWindow = SDL_CreateWindow(std::string(svTitle).c_str(),
         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, iWidth, iHeight,
         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
      if(!pWindow) throw std::runtime_error(SDL_GetError());
      pContext = SDL_GL_CreateContext(pWindow);
      if(!pContext) throw std::runtime_error(SDL_GetError());
      SDL_GL_MakeCurrent(pWindow, pContext);
      SDL_GL_SetSwapInterval(1);
      glewExperimental = GL_TRUE;
      GLenum const eGLEW = glewInit();
      if(eGLEW != GLEW_OK)
         throw std::runtime_error("GLEW initialization failed: " +
            std::string(reinterpret_cast<char const*>(glewGetErrorString(eGLEW))));
      if(!GLEW_VERSION_3_3)
         throw std::runtime_error("Managed Mesa does not expose OpenGL 3.3 core");
      pGui = ImGui::CreateContext();
      if(!pGui) throw std::runtime_error("ImGui initialization failed");
      ImGuiIO& aIo = ImGui::GetIO();
      aIo.BackendPlatformName = "BuildEngine SDL2 Mesa";
      aIo.BackendRendererName = "BuildEngine native OpenGL3";
      aIo.BackendFlags |= ImGuiBackendFlags_RendererHasTextures |
         ImGuiBackendFlags_RendererHasVtxOffset;
      aIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
      if(std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf"))
         aIo.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 19.0F);
      else aIo.Fonts->AddFontDefault();
      ImGui::StyleColorsDark();
      aIo.DisplaySize = {static_cast<float>(iWidth), static_cast<float>(iHeight)};
      aIo.DeltaTime = 1.0F / 60.0F;
      uLastTick = SDL_GetTicks64();
      InitializeRenderer();
   }

   OpenGLShowcase(OpenGLShowcase const&) = delete;
   OpenGLShowcase& operator=(OpenGLShowcase const&) = delete;

   ~OpenGLShowcase() {
      if(pContext) SDL_GL_MakeCurrent(pWindow, pContext);
      if(pGui) {
         ImGui::SetCurrentContext(pGui);
         for(ImTextureData* const pTexture : ImGui::GetPlatformIO().Textures)
            if(pTexture->Status != ImTextureStatus_Destroyed &&
               pTexture->TexID != ImTextureID_Invalid) {
               GLuint const uId = static_cast<GLuint>(reinterpret_cast<std::uintptr_t>(pTexture->TexID));
               glDeleteTextures(1, &uId);
               pTexture->SetTexID(ImTextureID_Invalid);
               pTexture->SetStatus(ImTextureStatus_Destroyed);
            }
         ImGui::DestroyContext(pGui);
      }
      if(uEBO) glDeleteBuffers(1, &uEBO);
      if(uVBO) glDeleteBuffers(1, &uVBO);
      if(uVAO) glDeleteVertexArrays(1, &uVAO);
      if(uProgram) glDeleteProgram(uProgram);
      if(pContext) SDL_GL_DeleteContext(pContext);
      if(pWindow) SDL_DestroyWindow(pWindow);
      SDL_GL_UnloadLibrary();
      SDL_Quit();
   }

   [[nodiscard]] bool Running() const noexcept { return bRunning; }
   [[nodiscard]] SDL_Window* Window() const noexcept { return pWindow; }

   /** \brief Advances SDL input and begins a new ImGui frame. */
   void BeginFrame() {
      ImGuiIO& aIo = ImGui::GetIO();
      SDL_Event aEvent {};
      while(SDL_PollEvent(&aEvent)) {
         if(aEvent.type == SDL_QUIT) bRunning = false;
         if(aEvent.type == SDL_MOUSEMOTION)
            aIo.AddMousePosEvent(static_cast<float>(aEvent.motion.x),
               static_cast<float>(aEvent.motion.y));
         if(aEvent.type == SDL_MOUSEBUTTONDOWN || aEvent.type == SDL_MOUSEBUTTONUP) {
            int const iButton = aEvent.button.button == SDL_BUTTON_LEFT ? 0 :
               aEvent.button.button == SDL_BUTTON_RIGHT ? 1 :
               aEvent.button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
            if(iButton >= 0) aIo.AddMouseButtonEvent(iButton,
               aEvent.type == SDL_MOUSEBUTTONDOWN);
         }
         if(aEvent.type == SDL_MOUSEWHEEL)
            aIo.AddMouseWheelEvent(static_cast<float>(aEvent.wheel.x),
               static_cast<float>(aEvent.wheel.y));
         if(aEvent.type == SDL_KEYDOWN || aEvent.type == SDL_KEYUP) {
            bool const bPressed = aEvent.type == SDL_KEYDOWN;
            ImGuiKey eKey = ImGuiKey_None;
            switch(aEvent.key.keysym.sym) {
               case SDLK_ESCAPE: eKey = ImGuiKey_Escape; break;
               case SDLK_RETURN: eKey = ImGuiKey_Enter; break;
               case SDLK_SPACE: eKey = ImGuiKey_Space; break;
               case SDLK_LEFT: eKey = ImGuiKey_LeftArrow; break;
               case SDLK_RIGHT: eKey = ImGuiKey_RightArrow; break;
               case SDLK_UP: eKey = ImGuiKey_UpArrow; break;
               case SDLK_DOWN: eKey = ImGuiKey_DownArrow; break;
               case SDLK_TAB: eKey = ImGuiKey_Tab; break;
               case SDLK_BACKSPACE: eKey = ImGuiKey_Backspace; break;
               default: break;
            }
            if(eKey != ImGuiKey_None) aIo.AddKeyEvent(eKey, bPressed);
            if(bPressed && aEvent.key.keysym.sym == SDLK_F11)
               SDL_SetWindowFullscreen(pWindow,
                  SDL_GetWindowFlags(pWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP ?
                     0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
         }
         if(aEvent.type == SDL_TEXTINPUT)
            aIo.AddInputCharactersUTF8(aEvent.text.text);
      }
      int iWidth {}, iHeight {};
      SDL_GetWindowSize(pWindow, &iWidth, &iHeight);
      aIo.DisplaySize = {static_cast<float>(iWidth), static_cast<float>(iHeight)};
      std::uint64_t const uNow = SDL_GetTicks64();
      aIo.DeltaTime = std::max(0.001F, (uNow-uLastTick)/1000.0F);
      uLastTick = uNow;
      ImGui::NewFrame();
   }

   /** \brief Ends GUI frame and swaps the Mesa OpenGL framebuffer. */
   void EndFrame() {
      ImGui::Render();
      RenderGUI();
      SDL_GL_SwapWindow(pWindow);
   }

   /** \brief Draws the standard BuildEngine showcase header. */
   static void Header(std::string_view const svTitle, std::string_view const svSubtitle) {
      ImGui::TextColored({.36F, .91F, .90F, 1.0F}, "BUILDENGINE / VISUAL LABORATORY");
      ImGui::Spacing();
      ImGui::TextUnformatted(svTitle.data(), svTitle.data() + svTitle.size());
      ImGui::TextDisabled("%.*s", static_cast<int>(svSubtitle.size()), svSubtitle.data());
      ImGui::Separator();
   }
};
