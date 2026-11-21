#pragma once

#include "GraphicsDllImports.h"
#include <GL/glew.h>
#include <SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

/**
\brief SDL2 window and OpenGL context using the official Dear ImGui backends.
\details Builds no custom ImGui draw adapter. The official SDL2 platform and
         OpenGL3 renderer implementations are provided by the managed
         imgui_sdl2_opengl3 DLL, whose dependencies are Mesa, GLEW and SDL2.
\note The existing SDL_Renderer-based NativeShowcase remains unchanged.
*/
class OpenGLShowcase final {
   SDL_Window* pWindow {};
   SDL_GLContext pContext {};
   ImGuiContext* pGui {};
   std::uint64_t uLastTick {};
   bool bRunning {true};
   bool bAboutRequested {};
   bool bSDLBackend {};
   bool bGLBackend {};

public:
   /**
   \brief Creates a native Mesa OpenGL window and official ImGui backends.
   \details The Mesa WGL library is chosen explicitly before context creation,
            then GLEW initializes the native OpenGL functions. The official
            backends are initialized only after ImGui core context creation.
   \param svTitle Window title.
   \param strMesaDLL Full path to managed Mesa opengl32.dll.
   \param iWidth Initial window width.
   \param iHeight Initial window height.
   \throw std::runtime_error If SDL, Mesa, GLEW or ImGui initialization fails.
   */
   explicit OpenGLShowcase(std::string_view const svTitle,
      std::string const& strMesaDLL, int const iWidth = 1540,
      int const iHeight = 900) {
      if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
         throw std::runtime_error(SDL_GetError());
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
      if(SDL_GL_LoadLibrary(strMesaDLL.c_str()) != 0)
         throw std::runtime_error("Managed Mesa WGL load failed: " +
            std::string(SDL_GetError()));
      pWindow = SDL_CreateWindow(std::string(svTitle).c_str(),
         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, iWidth, iHeight,
         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
      if(!pWindow) throw std::runtime_error(SDL_GetError());
      pContext = SDL_GL_CreateContext(pWindow);
      if(!pContext) throw std::runtime_error(SDL_GetError());
      if(SDL_GL_MakeCurrent(pWindow, pContext) != 0)
         throw std::runtime_error(SDL_GetError());
      SDL_GL_SetSwapInterval(1);
      glewExperimental = GL_TRUE;
      GLenum const eGLEW = glewInit();
      if(eGLEW != GLEW_OK)
         throw std::runtime_error("GLEW initialization failed: " +
            std::string(reinterpret_cast<char const*>(glewGetErrorString(eGLEW))));
      if(!GLEW_VERSION_3_3)
         throw std::runtime_error("Managed Mesa does not expose OpenGL 3.3 core");
      // glewInit may report an expected INVALID_ENUM from core profile probing.
      while(glGetError() != GL_NO_ERROR) {}
      IMGUI_CHECKVERSION();
      pGui = ImGui::CreateContext();
      if(!pGui) throw std::runtime_error("ImGui context creation failed");
      ImGuiIO& aIo = ImGui::GetIO();
      aIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
      if(std::filesystem::is_regular_file("C:/Windows/Fonts/segoeui.ttf"))
         aIo.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 19.0F);
      else
         aIo.Fonts->AddFontDefault();
      ImGui::StyleColorsDark();
      ImGuiStyle& aStyle = ImGui::GetStyle();
      aStyle.WindowRounding = 14.0F;
      aStyle.FrameRounding = 8.0F;
      aStyle.WindowPadding = {18.0F, 17.0F};
      aStyle.FramePadding = {11.0F, 8.0F};
      aStyle.ItemSpacing = {12.0F, 11.0F};
      if(!ImGui_ImplSDL2_InitForOpenGL(pWindow, pContext))
         throw std::runtime_error("Official ImGui SDL2 backend initialization failed");
      bSDLBackend = true;
      if(!ImGui_ImplOpenGL3_Init("#version 330"))
         throw std::runtime_error("Official ImGui OpenGL3 backend initialization failed");
      bGLBackend = true;
      aIo.DisplaySize = {static_cast<float>(iWidth), static_cast<float>(iHeight)};
      aIo.DeltaTime = 1.0F / 60.0F;
      uLastTick = SDL_GetTicks64();
   }

   OpenGLShowcase(OpenGLShowcase const&) = delete;
   OpenGLShowcase& operator=(OpenGLShowcase const&) = delete;

   ~OpenGLShowcase() {
      if(pContext && pWindow)
         SDL_GL_MakeCurrent(pWindow, pContext);
      if(pGui) {
         ImGui::SetCurrentContext(pGui);
         if(bGLBackend) ImGui_ImplOpenGL3_Shutdown();
         if(bSDLBackend) ImGui_ImplSDL2_Shutdown();
         ImGui::DestroyContext(pGui);
      }
      if(pContext) SDL_GL_DeleteContext(pContext);
      if(pWindow) SDL_DestroyWindow(pWindow);
      SDL_GL_UnloadLibrary();
      SDL_Quit();
   }

   [[nodiscard]] bool Running() const noexcept { return bRunning; }
   [[nodiscard]] SDL_Window* Window() const noexcept { return pWindow; }

   /**
   \brief Reports the actual runtime OpenGL version.
   \details Reads the current context; never assumes a Mesa driver feature.
   \returns Version string provided by the selected OpenGL implementation.
   \throw Nothing intentionally.
   */
   [[nodiscard]] std::string VersionString() const {
      GLubyte const* const pText = glGetString(GL_VERSION);
      return pText ? std::string(reinterpret_cast<char const*>(pText)) :
         std::string("OpenGL version unavailable");
   }

   /**
   \brief Dispatches SDL2 events through the official platform backend.
   \details Creates one ImGui frame using both official backend NewFrame calls.
   \returns Nothing.
   \throw Nothing intentionally.
   */
   void BeginFrame() {
      SDL_Event aEvent {};
      while(SDL_PollEvent(&aEvent)) {
         ImGui_ImplSDL2_ProcessEvent(&aEvent);
         if(aEvent.type == SDL_QUIT ||
            (aEvent.type == SDL_WINDOWEVENT &&
             aEvent.window.event == SDL_WINDOWEVENT_CLOSE &&
             aEvent.window.windowID == SDL_GetWindowID(pWindow)))
            bRunning = false;
         if(aEvent.type == SDL_KEYDOWN && aEvent.key.repeat == 0) {
            if(aEvent.key.keysym.sym == SDLK_F11)
               SDL_SetWindowFullscreen(pWindow,
                  SDL_GetWindowFlags(pWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP ?
                     0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            else if(aEvent.key.keysym.sym == SDLK_F1)
               bAboutRequested = true;
         }
      }
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();
   }

   /**
   \brief Renders ImGui using the managed upstream OpenGL3 backend.
   \details Restores the default framebuffer after terrain FBO rendering.
   \returns Nothing.
   \throw Nothing intentionally.
   */
   void EndFrame() {
      if(bAboutRequested) {
         ImGui::OpenPopup("About this showcase");
         bAboutRequested = false;
      }
      ImGui::SetNextWindowSize({520.0F, 0.0F}, ImGuiCond_Appearing);
      if(ImGui::BeginPopupModal("About this showcase", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
         ImGui::TextColored({0.36F, 0.91F, 0.90F, 1.0F},
                            "BUILDENGINE / NATIVE C++23 LABORATORY");
         ImGui::Separator();
         ImGui::TextWrapped("%s", SDL_GetWindowTitle(pWindow));
         ImGui::TextDisabled("Renderer: OpenGL3 / Dear ImGui %s", IMGUI_VERSION);
         ImGui::TextWrapped("OpenGL: %s", VersionString().c_str());
         ImGui::Spacing();
         ImGui::TextWrapped("This application is an interactive BuildEngine test "
                            "using native dependencies. Actual package versions, "
                            "licenses, mathematical background and verification "
                            "evidence are documented in its showcase page.");
         ImGui::Separator();
         ImGui::TextUnformatted("F1   About this showcase");
         ImGui::TextUnformatted("F11  Fullscreen / windowed");
         ImGui::TextUnformatted("Esc  Close this dialog");
         if(ImGui::Button("Close", {120.0F, 0.0F}))
            ImGui::CloseCurrentPopup();
         ImGui::EndPopup();
      }
      ImGui::Render();
      int iWidth {}, iHeight {};
      SDL_GL_GetDrawableSize(pWindow, &iWidth, &iHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, iWidth, iHeight);
      glClearColor(0.025F, 0.045F, 0.09F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(pWindow);
   }

   /**
   \brief Draws the common BuildEngine showcase header.
   \details Uses standard ImGui controls through the official backends.
   \param svTitle Showcase heading.
   \param svSubtitle Supporting text.
   \returns Nothing.
   \throw Nothing intentionally.
   */
   static void Header(std::string_view const svTitle,
      std::string_view const svSubtitle) {
      ImGui::TextColored({.36F, .91F, .90F, 1.0F},
         "BUILDENGINE / VISUAL LABORATORY");
      ImGui::Spacing();
      ImGui::TextUnformatted(svTitle.data(), svTitle.data() + svTitle.size());
      ImGui::TextDisabled("%.*s", static_cast<int>(svSubtitle.size()),
         svSubtitle.data());
      ImGui::Separator();
   }
};
