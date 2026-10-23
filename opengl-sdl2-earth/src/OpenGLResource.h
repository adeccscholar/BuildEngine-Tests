#pragma once

#include <GL/glew.h>

#include <concepts>
#include <utility>

/**
 \brief Beschreibt die Anforderungen an einen OpenGL Ressourcen Deleter.
 \details Ein Traits Typ muss eine statische noexcept Funktion Delete fuer genau einen GLuint Handle bereitstellen.
 \tparam TTraits Zu pruefender Traits Typ.
 */
template<typename TTraits>
concept GlObjectTraits = requires(GLuint const uHandle) {
   { TTraits::Delete(uHandle) } noexcept -> std::same_as<void>;
   };

/**
 \brief Move-only RAII Wrapper fuer einen OpenGL Handle.
 \details Der Wrapper besitzt genau einen OpenGL Handle und gibt ihn beim Zerstoeren ueber den zugeordneten Traits Typ frei.
 \tparam TTraits Traits Typ mit der passenden OpenGL Loeschfunktion.
 \note Der Wrapper muss zerstoert werden, solange der zugehoerige OpenGL Kontext noch gueltig ist.
 */
template<GlObjectTraits TTraits>
class UniqueGlObject final {
public:
   /**
    \brief Erzeugt einen leeren OpenGL Ressourcen Wrapper.
    \details Der Handle hat den Wert 0 und besitzt noch keine Ressource.
    \returns Nichts.
    \throw Nichts.
    */
   UniqueGlObject() noexcept = default;

   /**
    \brief Uebernimmt den Besitz eines bereits erzeugten OpenGL Handles.
    \details Der uebergebene Handle wird beim Zerstoeren automatisch freigegeben.
    \param uHandle Zu uebernehmender OpenGL Handle.
    \returns Nichts.
    \throw Nichts.
    */
   explicit UniqueGlObject(GLuint const uHandle) noexcept
      : m_uHandle{ uHandle } {
      }

   /**
    \brief Gibt die besessene OpenGL Ressource frei.
    \details Ein leerer Wrapper fuehrt keine OpenGL Operation aus.
    \returns Nichts.
    \throw Nichts.
    */
   ~UniqueGlObject() noexcept {
      Reset();
      }

   UniqueGlObject(UniqueGlObject const&) = delete;
   UniqueGlObject& operator=(UniqueGlObject const&) = delete;

   /**
    \brief Verschiebt den Besitz eines OpenGL Handles.
    \details Das Quellobjekt wird anschliessend leer.
    \param aOther Quellobjekt.
    \returns Nichts.
    \throw Nichts.
    */
   UniqueGlObject(UniqueGlObject&& aOther) noexcept
      : m_uHandle{ std::exchange(aOther.m_uHandle, 0U) } {
      }

   /**
    \brief Uebernimmt den OpenGL Handle eines anderen Wrappers.
    \details Eine zuvor besessene Ressource wird vor der Uebernahme freigegeben.
    \param aOther Quellobjekt.
    \returns Referenz auf dieses Objekt.
    \throw Nichts.
    */
   UniqueGlObject& operator=(UniqueGlObject&& aOther) noexcept {
      if (this != &aOther) {
         Reset();
         m_uHandle = std::exchange(aOther.m_uHandle, 0U);
         }

      return *this;
      }

   /**
    \brief Liefert den gespeicherten OpenGL Handle.
    \details Die Besitzverhaeltnisse werden nicht veraendert.
    \returns Den gespeicherten GLuint Handle.
    \throw Nichts.
    */
   [[nodiscard]] GLuint Get() const noexcept {
      return m_uHandle;
      }

   /**
    \brief Liefert eine Adresse, in die eine glGen Funktion einen neuen Handle schreiben kann.
    \details Eine zuvor besessene Ressource wird vorher freigegeben.
    \returns Adresse des internen Handle Speichers.
    \throw Nichts.
    \warning Put nur unmittelbar als Ausgabeargument einer glGen Funktion verwenden.
    */
   [[nodiscard]] GLuint* Put() noexcept {
      Reset();
      return &m_uHandle;
      }

   /**
    \brief Ersetzt die aktuell besessene Ressource.
    \details Ein vorhandener Handle wird zuerst ueber den Traits Typ freigegeben.
    \param uHandle Neuer Handle oder 0 fuer einen leeren Wrapper.
    \returns Nichts.
    \throw Nichts.
    */
   void Reset(GLuint const uHandle = 0U) noexcept {
      if (m_uHandle != 0U) {
         TTraits::Delete(m_uHandle);
         }

      m_uHandle = uHandle;
      }

   /**
    \brief Prueft, ob der Wrapper einen gueltigen OpenGL Handle besitzt.
    \details Ein Handle ungleich 0 wird als gueltig betrachtet.
    \returns true bei einem besessenen Handle, sonst false.
    \throw Nichts.
    */
   [[nodiscard]] explicit operator bool() const noexcept {
      return m_uHandle != 0U;
      }

private:
   GLuint m_uHandle{};
   };

struct GlShaderTraits final {
   static void Delete(GLuint const uHandle) noexcept {
      glDeleteShader(uHandle);
      }
   };

struct GlProgramTraits final {
   static void Delete(GLuint const uHandle) noexcept {
      glDeleteProgram(uHandle);
      }
   };

struct GlBufferTraits final {
   static void Delete(GLuint const uHandle) noexcept {
      glDeleteBuffers(1, &uHandle);
      }
   };


struct GlTextureTraits final {
   static void Delete(GLuint const uHandle) noexcept {
      glDeleteTextures(1, &uHandle);
      }
   };

struct GlVertexArrayTraits final {
   static void Delete(GLuint const uHandle) noexcept {
      glDeleteVertexArrays(1, &uHandle);
      }
   };

using UniqueGlShader = UniqueGlObject<GlShaderTraits>;
using UniqueGlProgram = UniqueGlObject<GlProgramTraits>;
using UniqueGlBuffer = UniqueGlObject<GlBufferTraits>;
using UniqueGlVertexArray = UniqueGlObject<GlVertexArrayTraits>;
using UniqueGlTexture = UniqueGlObject<GlTextureTraits>;
