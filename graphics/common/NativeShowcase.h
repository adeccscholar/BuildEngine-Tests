#pragma once

#include <SDL.h>
#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

/**
\brief Owns the native SDL2 window, ImGui context, and dynamic font textures.
\details Renders real ImGui/ImPlot vertices including the atlas textures required for readable text.
         Does not depend on gnuplot or an external rendering executable.
\note This is a test-side rendering adapter, not a replacement for an installed BuildEngine library.
*/
class NativeShowcase final {
   SDL_Window* pWindow {};
   SDL_Renderer* pRenderer {};
   ImGuiContext* pContext {};
   std::uint64_t uLastTick {};
   bool bRunning { true };

   /** \brief Synchronizes the dynamic font textures introduced in ImGui 1.92. */
   void UpdateTexture(ImTextureData* const pTexture) {
      if(pTexture->Status == ImTextureStatus_WantCreate) {
         SDL_Texture* const pSDLTexture = SDL_CreateTexture(
            pRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
            pTexture->Width, pTexture->Height);
         if(!pSDLTexture)
            throw std::runtime_error { SDL_GetError() };
         if(SDL_UpdateTexture(pSDLTexture, nullptr, pTexture->GetPixels(),
                              pTexture->GetPitch()) != 0) {
            SDL_DestroyTexture(pSDLTexture);
            throw std::runtime_error { SDL_GetError() };
            }
         SDL_SetTextureBlendMode(pSDLTexture, SDL_BLENDMODE_BLEND);
         SDL_SetTextureScaleMode(pSDLTexture, SDL_ScaleModeLinear);
         pTexture->SetTexID((ImTextureID)(intptr_t)pSDLTexture);
         pTexture->SetStatus(ImTextureStatus_OK);
         }
      else if(pTexture->Status == ImTextureStatus_WantUpdates) {
         SDL_Texture* const pSDLTexture = (SDL_Texture*)(intptr_t)pTexture->TexID;
         for(ImTextureRect const& aRect : pTexture->Updates) {
            SDL_Rect const aSDLRect { aRect.x, aRect.y, aRect.w, aRect.h };
            if(SDL_UpdateTexture(pSDLTexture, &aSDLRect,
                                 pTexture->GetPixelsAt(aRect.x, aRect.y),
                                 pTexture->GetPitch()) != 0)
               throw std::runtime_error { SDL_GetError() };
            }
         pTexture->SetStatus(ImTextureStatus_OK);
         }
      else if(pTexture->Status == ImTextureStatus_WantDestroy) {
         SDL_Texture* const pSDLTexture = (SDL_Texture*)(intptr_t)pTexture->TexID;
         if(pSDLTexture) SDL_DestroyTexture(pSDLTexture);
         pTexture->SetTexID(ImTextureID_Invalid);
         pTexture->SetStatus(ImTextureStatus_Destroyed);
         }
      }

   void Render() {
      ImDrawData* const pData = ImGui::GetDrawData();
      if(!pData) return;
      if(pData->Textures) {
         for(ImTextureData* const pTexture : *pData->Textures)
            if(pTexture->Status != ImTextureStatus_OK)
               UpdateTexture(pTexture);
         }

      SDL_Rect const aViewport { 0, 0,
         static_cast<int>(pData->DisplaySize.x),
         static_cast<int>(pData->DisplaySize.y) };
      SDL_RenderSetViewport(pRenderer, &aViewport);
      SDL_RenderSetScale(pRenderer, 1.0F, 1.0F);
      SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
      for(ImDrawList const* const pList : pData->CmdLists) {
         for(ImDrawCmd const& aCommand : pList->CmdBuffer) {
            if(aCommand.UserCallback) {
               if(aCommand.UserCallback != ImDrawCallback_ResetRenderState)
                  aCommand.UserCallback(pList, &aCommand);
               continue;
               }
            ImVec2 const aClipMin {
               (aCommand.ClipRect.x - pData->DisplayPos.x) * pData->FramebufferScale.x,
               (aCommand.ClipRect.y - pData->DisplayPos.y) * pData->FramebufferScale.y
               };
            ImVec2 const aClipMax {
               (aCommand.ClipRect.z - pData->DisplayPos.x) * pData->FramebufferScale.x,
               (aCommand.ClipRect.w - pData->DisplayPos.y) * pData->FramebufferScale.y
               };
            SDL_Rect const aClip {
               static_cast<int>(std::max(0.0F, aClipMin.x)),
               static_cast<int>(std::max(0.0F, aClipMin.y)),
               std::max(0, static_cast<int>(aClipMax.x - aClipMin.x)),
               std::max(0, static_cast<int>(aClipMax.y - aClipMin.y))
               };
            if(aClip.w == 0 || aClip.h == 0) continue;
            SDL_RenderSetClipRect(pRenderer, &aClip);
            ImDrawVert const* const pVertices = pList->VtxBuffer.Data + aCommand.VtxOffset;
            ImDrawIdx const* const pIndices = pList->IdxBuffer.Data + aCommand.IdxOffset;
            SDL_Texture* const pTexture = (SDL_Texture*)(intptr_t)aCommand.GetTexID();
            if(SDL_RenderGeometryRaw(
                  pRenderer, pTexture,
                  reinterpret_cast<float const*>(reinterpret_cast<char const*>(pVertices) + offsetof(ImDrawVert, pos)), sizeof(ImDrawVert),
                  reinterpret_cast<SDL_Color const*>(reinterpret_cast<char const*>(pVertices) + offsetof(ImDrawVert, col)), sizeof(ImDrawVert),
                  reinterpret_cast<float const*>(reinterpret_cast<char const*>(pVertices) + offsetof(ImDrawVert, uv)), sizeof(ImDrawVert),
                  pList->VtxBuffer.Size - static_cast<int>(aCommand.VtxOffset),
                  pIndices, static_cast<int>(aCommand.ElemCount), sizeof(ImDrawIdx)) != 0)
               throw std::runtime_error { SDL_GetError() };
            }
         }
      SDL_RenderSetClipRect(pRenderer, nullptr);
      }

public:
   /**
   \brief Initializes the native window and ImGui 1.93 rendering facilities.
   \details Loads Segoe UI on Windows when present, otherwise falls back to the built-in ImGui font.
   \param svTitle Native application window title.
   \param iWidth Initial window width.
   \param iHeight Initial window height.
   \throw std::runtime_error On SDL, renderer, or ImGui initialization failure.
   */
   explicit NativeShowcase(std::string_view const svTitle,
                           int const iWidth = 1440, int const iHeight = 900) {
      if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
         throw std::runtime_error { SDL_GetError() };
      pWindow = SDL_CreateWindow(
         std::string { svTitle }.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
         iWidth, iHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
      if(!pWindow) throw std::runtime_error { SDL_GetError() };
      pRenderer = SDL_CreateRenderer(
         pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
      if(!pRenderer) throw std::runtime_error { SDL_GetError() };
      IMGUI_CHECKVERSION();
      pContext = ImGui::CreateContext();
      if(!pContext) throw std::runtime_error { "ImGui context creation failed" };
      ImGuiIO& aIo = ImGui::GetIO();
      aIo.BackendFlags |= ImGuiBackendFlags_RendererHasTextures |
                          ImGuiBackendFlags_RendererHasVtxOffset;
      aIo.BackendPlatformName = "BuildEngine SDL2 native showcase";
      aIo.BackendRendererName = "BuildEngine SDL2 geometry renderer";
      aIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
      ImFont* pFont {};
      if(std::filesystem::is_regular_file("C:/Windows/Fonts/segoeui.ttf"))
         pFont = aIo.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 19.0F);
      if(!pFont) aIo.Fonts->AddFontDefault();
      ImGui::StyleColorsDark();
      ImGuiStyle& aStyle = ImGui::GetStyle();
      aStyle.WindowRounding = 14.0F;
      aStyle.ChildRounding = 13.0F;
      aStyle.FrameRounding = 8.0F;
      aStyle.GrabRounding = 8.0F;
      aStyle.WindowPadding = { 18.0F, 17.0F };
      aStyle.FramePadding = { 11.0F, 8.0F };
      aStyle.ItemSpacing = { 12.0F, 11.0F };
      aStyle.Colors[ImGuiCol_WindowBg] = ImVec4 { .044F, .067F, .129F, 1.0F };
      aStyle.Colors[ImGuiCol_ChildBg] = ImVec4 { .075F, .103F, .175F, 1.0F };
      aStyle.Colors[ImGuiCol_Button] = ImVec4 { .085F, .262F, .390F, 1.0F };
      aStyle.Colors[ImGuiCol_ButtonHovered] = ImVec4 { .11F, .38F, .48F, 1.0F };
      aStyle.Colors[ImGuiCol_ButtonActive] = ImVec4 { .10F, .49F, .56F, 1.0F };
      aStyle.Colors[ImGuiCol_Header] = ImVec4 { .13F, .28F, .40F, 1.0F };
      aStyle.Colors[ImGuiCol_Text] = ImVec4 { .91F, .95F, 1.0F, 1.0F };
      aIo.DisplaySize = ImVec2 { static_cast<float>(iWidth), static_cast<float>(iHeight) };
      aIo.DeltaTime = 1.0F / 60.0F;
      uLastTick = SDL_GetTicks64();
      }

   NativeShowcase(NativeShowcase const&) = delete;
   NativeShowcase& operator=(NativeShowcase const&) = delete;

   ~NativeShowcase() {
      if(pContext) {
         ImGui::SetCurrentContext(pContext);
         for(ImTextureData* const pTexture : ImGui::GetPlatformIO().Textures) {
            if(pTexture->Status != ImTextureStatus_Destroyed &&
            pTexture->TexID != ImTextureID_Invalid) {
               pTexture->SetStatus(ImTextureStatus_WantDestroy);
               try { UpdateTexture(pTexture); }
               catch(...) { /* Cleanup must not throw. */ }
               }
            }
         ImGui::DestroyContext(pContext);
         }
      if(pRenderer) SDL_DestroyRenderer(pRenderer);
      if(pWindow) SDL_DestroyWindow(pWindow);
      SDL_Quit();
      }

   [[nodiscard]] bool Running() const noexcept { return bRunning; }
   [[nodiscard]] SDL_Window* Window() const noexcept { return pWindow; }

   /** \brief Collects SDL input and starts one correctly timed interactive ImGui frame. */
   void BeginFrame() {
      ImGuiIO& aIo = ImGui::GetIO();
      SDL_Event aEvent {};
      while(SDL_PollEvent(&aEvent)) {
         if(aEvent.type == SDL_QUIT) bRunning = false;
         if(aEvent.type == SDL_MOUSEMOTION)
            aIo.AddMousePosEvent(static_cast<float>(aEvent.motion.x),
                                 static_cast<float>(aEvent.motion.y));
         if(aEvent.type == SDL_MOUSEBUTTONDOWN || aEvent.type == SDL_MOUSEBUTTONUP) {
            int const iButton =
               aEvent.button.button == SDL_BUTTON_LEFT ? 0 :
               aEvent.button.button == SDL_BUTTON_RIGHT ? 1 :
               aEvent.button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
            if(iButton >= 0) aIo.AddMouseButtonEvent(
               iButton, aEvent.type == SDL_MOUSEBUTTONDOWN);
            }
         if(aEvent.type == SDL_MOUSEWHEEL)
            aIo.AddMouseWheelEvent(static_cast<float>(aEvent.wheel.x),
                                   static_cast<float>(aEvent.wheel.y));
         if(aEvent.type == SDL_KEYDOWN || aEvent.type == SDL_KEYUP) {
            bool const bPressed = aEvent.type == SDL_KEYDOWN;
            ImGuiKey aKey = ImGuiKey_None;
            switch(aEvent.key.keysym.sym) {
               case SDLK_ESCAPE: aKey = ImGuiKey_Escape; break;
               case SDLK_RETURN: aKey = ImGuiKey_Enter; break;
               case SDLK_SPACE: aKey = ImGuiKey_Space; break;
               case SDLK_LEFT: aKey = ImGuiKey_LeftArrow; break;
               case SDLK_RIGHT: aKey = ImGuiKey_RightArrow; break;
               case SDLK_UP: aKey = ImGuiKey_UpArrow; break;
               case SDLK_DOWN: aKey = ImGuiKey_DownArrow; break;
               case SDLK_TAB: aKey = ImGuiKey_Tab; break;
               case SDLK_BACKSPACE: aKey = ImGuiKey_Backspace; break;
               default: break;
               }
            if(aKey != ImGuiKey_None) aIo.AddKeyEvent(aKey, bPressed);
            if(bPressed && aEvent.key.keysym.sym == SDLK_F11)
               SDL_SetWindowFullscreen(pWindow,
                  SDL_GetWindowFlags(pWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP
                     ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
         if(aEvent.type == SDL_TEXTINPUT) aIo.AddInputCharactersUTF8(aEvent.text.text);
         }
      int iWidth {}, iHeight {};
      SDL_GetWindowSize(pWindow, &iWidth, &iHeight);
      aIo.DisplaySize = ImVec2 { static_cast<float>(iWidth), static_cast<float>(iHeight) };
      std::uint64_t const uNow = SDL_GetTicks64();
      aIo.DeltaTime = std::max(1.0F / 1000.0F, (uNow - uLastTick) / 1000.0F);
      uLastTick = uNow;
      ImGui::NewFrame();
      }

   /** \brief Renders a complete ImGui frame with font text, plots, and clipping. */
   void EndFrame() {
      ImGui::Render();
      SDL_SetRenderDrawColor(pRenderer, 7, 13, 28, 255);
      SDL_RenderClear(pRenderer);
      Render();
      SDL_RenderPresent(pRenderer);
      }

   /** \brief Draws a compact, readable show-control header. */
   static void Header(std::string_view const svTitle, std::string_view const svSubtitle) {
      ImGui::TextColored(ImVec4 { .36F, .91F, .90F, 1.0F },
                         "BUILDENGINE  /  VISUAL LABORATORY");
      ImGui::Spacing();
      ImGui::TextUnformatted(svTitle.data(), svTitle.data() + svTitle.size());
      ImGui::TextDisabled("%.*s", static_cast<int>(svSubtitle.size()), svSubtitle.data());
      ImGui::Separator();
      }
   };
