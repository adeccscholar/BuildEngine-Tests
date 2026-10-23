#pragma once

#include <SDL.h>

#include <memory>

/**
 \brief Deleter fuer ein SDL Fenster.
 \details Erlaubt die Verwaltung von SDL_Window mit std::unique_ptr.
 */
struct SdlWindowDeleter final {
   void operator()(SDL_Window* const pWindow) const noexcept {
      if (pWindow != nullptr) {
         SDL_DestroyWindow(pWindow);
         }
      }
   };

/**
 \brief Deleter fuer einen SDL OpenGL Kontext.
 \details SDL_GLContext ist ein untypisierter Zeiger und wird ueber SDL_GL_DeleteContext freigegeben.
 */
struct SdlGlContextDeleter final {
   void operator()(void* const pContext) const noexcept {
      if (pContext != nullptr) {
         SDL_GL_DeleteContext(pContext);
         }
      }
   };

using UniqueSdlWindow = std::unique_ptr<SDL_Window, SdlWindowDeleter>;
using UniqueSdlGlContext = std::unique_ptr<void, SdlGlContextDeleter>;

/**
 \brief Verwaltet die Lebensdauer der globalen SDL Initialisierung.
 \details Nach erfolgreichem Initialize wird SDL beim Zerstoeren automatisch mit SDL_Quit beendet.
 */
class SdlRuntime final {
public:
   SdlRuntime() noexcept = default;

   ~SdlRuntime() noexcept {
      if (m_fInitialized) {
         SDL_Quit();
         }
      }

   SdlRuntime(SdlRuntime const&) = delete;
   SdlRuntime& operator=(SdlRuntime const&) = delete;
   SdlRuntime(SdlRuntime&&) = delete;
   SdlRuntime& operator=(SdlRuntime&&) = delete;

   /**
    \brief Initialisiert SDL mit den angegebenen Subsystemen.
    \details Ein zweiter erfolgreicher Aufruf ist nicht erforderlich und wird als Erfolg behandelt.
    \param uFlags SDL Initialisierungsflags.
    \returns true bei erfolgreicher Initialisierung, sonst false.
    \throw Nichts.
    */
   [[nodiscard]] bool Initialize(Uint32 const uFlags) noexcept {
      if (m_fInitialized) {
         return true;
         }

      if (SDL_Init(uFlags) != 0) {
         return false;
         }

      m_fInitialized = true;
      return true;
      }

private:
   bool m_fInitialized{};
   };
