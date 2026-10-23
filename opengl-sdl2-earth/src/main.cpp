#include "EarthScene.h"
#include "LogoScene.h"
#include "OpenGLResource.h"
#include "SdlResource.h"

#include <GL/glew.h>
#include <SDL.h>

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

/**
 \brief Kennzeichnet die aktuell ausgewaehlte Demonstrationsvariante.
 \details Logo, Earth Day und Earth Night waehlen die jeweils aktive Szene beziehungsweise Earth Textur.
 */
enum class DemoMode {
   Logo,
   EarthDay,
   EarthNight
};

/**
 \brief Ein Vertex fuer die zweidimensionale Benutzeroberflaeche.
 \details Positionen werden in logischen SDL Fensterkoordinaten gespeichert. Die Farbe wird pro Vertex an den UI Shader uebergeben.
 */
struct UiVertex {
   float flX{};
   float flY{};
   float flRed{};
   float flGreen{};
   float flBlue{};
   float flAlpha{};
};

/**
 \brief Rechteckiger Bereich eines UI Elements.
 \details Die Koordinaten verwenden den Ursprung links oben und entsprechen den SDL Mauskoordinaten.
 */
struct UiRectangle {
   float flX{};
   float flY{};
   float flWidth{};
   float flHeight{};
};

/**
 \brief OpenGL Ressourcen fuer die zweidimensionale Benutzeroberflaeche.
 \details Das Vertex Array und der dynamische Vertex Buffer werden fuer Panel, Buttons und Text gemeinsam verwendet.
 */
struct UiBuffer {
   UniqueGlVertexArray aVertexArray;
   UniqueGlBuffer aVertexBuffer;
};

/**
 \brief Gibt eine Debugmeldung ueber SDL aus.
 \details SDL_Log funktioniert sowohl unter Windows als auch auf dem Raspberry Pi.
 \param svMessage Auszugebende Meldung.
 \returns Nichts.
 \throw Nichts.
 */
void DebugPrint(std::string_view const svMessage) noexcept {
   std::string const strMessage{ svMessage };
   SDL_Log("%s", strMessage.c_str());
   }

/**
 \brief Kompiliert einen Shader fuer die Benutzeroberflaeche.
 \details Compilerfehler werden im SDL Log ausgegeben.
 \param uShaderType OpenGL Shadertyp.
 \param szSource Nullterminierter GLSL Quelltext.
 \returns Die Shaderkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlShader CompileUiShader(GLenum const uShaderType, char const* const szSource) noexcept {
   UniqueGlShader aShader{ glCreateShader(uShaderType) };

   if (!aShader) {
      DebugPrint("UI Shader konnte nicht erzeugt werden.");
      return {};
      }

   GLuint const uShader = aShader.Get();
   glShaderSource(uShader, 1, &szSource, nullptr);
   glCompileShader(uShader);

   GLint iCompiled = GL_FALSE;
   glGetShaderiv(uShader, GL_COMPILE_STATUS, &iCompiled);

   if (iCompiled == GL_TRUE) {
      return aShader;
      }

   std::array<char, 4096> arrLog{};
   glGetShaderInfoLog(uShader, static_cast<GLsizei>(arrLog.size()), nullptr, arrLog.data());
   DebugPrint(std::format("UI Shader compilation failed: {}", arrLog.data()));

   return {};
   }

/**
 \brief Erzeugt das Shaderprogramm fuer die zweidimensionale Benutzeroberflaeche.
 \details UI Vertices werden direkt aus logischen SDL Fensterkoordinaten in Normalized Device Coordinates umgerechnet.
 \returns Die OpenGL Programmkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlProgram CreateUiProgram() noexcept {
   constexpr char const* szVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aVertexColor;

uniform vec2 uViewportSize;

out vec4 vColor;

void main() {
   vec2 aNormalized = vec2(
      aPosition.x / uViewportSize.x * 2.0 - 1.0,
      1.0 - aPosition.y / uViewportSize.y * 2.0);

   gl_Position = vec4(aNormalized, 0.0, 1.0);
   vColor = aVertexColor;
}
)GLSL";

   constexpr char const* szFragmentShader = R"GLSL(
#version 330 core

in vec4 vColor;
out vec4 aColor;

void main() {
   aColor = vColor;
}
)GLSL";

   UniqueGlShader aVertexShader = CompileUiShader(GL_VERTEX_SHADER, szVertexShader);

   if (!aVertexShader) {
      return {};
      }

   UniqueGlShader aFragmentShader = CompileUiShader(GL_FRAGMENT_SHADER, szFragmentShader);

   if (!aFragmentShader) {
      return {};
      }

   UniqueGlProgram aProgram{ glCreateProgram() };

   if (!aProgram) {
      DebugPrint("UI Shaderprogramm konnte nicht erzeugt werden.");
      return {};
      }

   GLuint const uProgram = aProgram.Get();
   glAttachShader(uProgram, aVertexShader.Get());
   glAttachShader(uProgram, aFragmentShader.Get());
   glLinkProgram(uProgram);

   GLint iLinked = GL_FALSE;
   glGetProgramiv(uProgram, GL_LINK_STATUS, &iLinked);

   if (iLinked == GL_TRUE) {
      return aProgram;
      }

   std::array<char, 4096> arrLog{};
   glGetProgramInfoLog(uProgram, static_cast<GLsizei>(arrLog.size()), nullptr, arrLog.data());
   DebugPrint(std::format("UI Program linking failed: {}", arrLog.data()));

   return {};
   }

/**
 \brief Initialisiert SDL und erzeugt das OpenGL Fenster.
 \details Es wird ein OpenGL 3.3 Core Kontext mit Double Buffering und Tiefenpuffer erzeugt.
 \param pWindow Referenz auf den zu erzeugenden SDL Fensterzeiger.
 \param pContext Referenz auf den zu erzeugenden SDL OpenGL Kontext.
 \returns true bei erfolgreicher Initialisierung.
 \throw Nichts.
 */
bool InitializeWindow(UniqueSdlWindow& pWindow, UniqueSdlGlContext& pContext) noexcept {
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

   pWindow.reset(SDL_CreateWindow("OpenGL Shader Demo",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      1280, 720,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

   if (!pWindow) {
      DebugPrint(std::format("SDL_CreateWindow fehlgeschlagen: {}", SDL_GetError()));
      return false;
      }

   pContext.reset(SDL_GL_CreateContext(pWindow.get()));

   if (!pContext) {
      DebugPrint(std::format("SDL_GL_CreateContext fehlgeschlagen: {}", SDL_GetError()));
      return false;
      }

   SDL_GL_MakeCurrent(pWindow.get(), pContext.get());
   if (SDL_GL_SetSwapInterval(1) != 0)
      DebugPrint(std::format("VSync unavailable: {}", SDL_GetError()));
   DebugPrint(std::format("VSync interval: {}", SDL_GL_GetSwapInterval()));

   return true;
   }

/**
 \brief Initialisiert GLEW und den gemeinsamen OpenGL Zustand.
 \details Tiefentest, Backface Culling und Alpha Blending werden fuer die Demo aktiviert.
 \returns true bei erfolgreicher Initialisierung.
 \throw Nichts.
 */
bool InitializeOpenGL() noexcept {
   glewExperimental = GL_TRUE;
   GLenum const uResult = glewInit();

   if (uResult != GLEW_OK) {
      DebugPrint(std::format("GLEW Initialisierung fehlgeschlagen: {}",
         reinterpret_cast<char const*>(glewGetErrorString(uResult))));
      return false;
   }

   glGetError();
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   DebugPrint(std::format("OpenGL Version: {}", reinterpret_cast<char const*>(glGetString(GL_VERSION))));
   DebugPrint(std::format("Renderer: {}", reinterpret_cast<char const*>(glGetString(GL_RENDERER))));

   return true;
}

/**
 \brief Prueft, ob eine Mausposition innerhalb eines Rechtecks liegt.
 \details Die Berechnung erfolgt in logischen SDL Fensterkoordinaten.
 \param aRectangle Zu pruefendes Rechteck.
 \param iMouseX Horizontale Mausposition.
 \param iMouseY Vertikale Mausposition.
 \returns true, wenn die Mausposition innerhalb des Rechtecks liegt.
 \throw Nichts.
 */
bool IsInside(UiRectangle const& aRectangle, int const iMouseX, int const iMouseY) noexcept {
   float const flMouseX = static_cast<float>(iMouseX);
   float const flMouseY = static_cast<float>(iMouseY);

   return (flMouseX >= aRectangle.flX)
      && (flMouseX <= aRectangle.flX + aRectangle.flWidth)
      && (flMouseY >= aRectangle.flY)
      && (flMouseY <= aRectangle.flY + aRectangle.flHeight);
}

/**
 \brief Fuegt ein gefuelltes Rechteck zur UI Vertexliste hinzu.
 \details Das Rechteck wird aus zwei Dreiecken aufgebaut.
 \param vecVertices Zielcontainer der UI Vertices.
 \param aRectangle Position und Groesse des Rechtecks.
 \param flRed Rotanteil der Farbe.
 \param flGreen Gruenanteil der Farbe.
 \param flBlue Blauanteil der Farbe.
 \param flAlpha Alphawert der Farbe.
 \returns Nichts.
 \throw Nichts.
 */
void AddUiRectangle(std::vector<UiVertex>& vecVertices, UiRectangle const& aRectangle,
   float const flRed, float const flGreen, float const flBlue, float const flAlpha) {
   UiVertex const aTopLeft{ aRectangle.flX, aRectangle.flY, flRed, flGreen, flBlue, flAlpha };
   UiVertex const aTopRight{ aRectangle.flX + aRectangle.flWidth, aRectangle.flY,
      flRed, flGreen, flBlue, flAlpha };
   UiVertex const aBottomLeft{ aRectangle.flX, aRectangle.flY + aRectangle.flHeight,
      flRed, flGreen, flBlue, flAlpha };
   UiVertex const aBottomRight{ aRectangle.flX + aRectangle.flWidth,
      aRectangle.flY + aRectangle.flHeight, flRed, flGreen, flBlue, flAlpha };

   vecVertices.push_back(aTopLeft);
   vecVertices.push_back(aBottomLeft);
   vecVertices.push_back(aTopRight);
   vecVertices.push_back(aTopRight);
   vecVertices.push_back(aBottomLeft);
   vecVertices.push_back(aBottomRight);
}

/**
 \brief Liefert ein 5 mal 7 Bitmuster fuer ein ASCII Zeichen.
 \details Kleinbuchstaben werden als Grossbuchstaben dargestellt.
 \param chCharacter Darzustellendes ASCII Zeichen.
 \returns Sieben Zeilen mit jeweils fuenf gesetzten oder geloeschten Bits.
 \throw Nichts.
 */
std::array<std::uint8_t, 7> GlyphRows(char chCharacter) noexcept {
   if ((chCharacter >= 'a') && (chCharacter <= 'z')) {
      chCharacter = static_cast<char>(chCharacter - 'a' + 'A');
   }

   switch (chCharacter) {
   case 'A': return { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
   case 'B': return { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E };
   case 'C': return { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E };
   case 'D': return { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E };
   case 'E': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
   case 'F': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 };
   case 'G': return { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F };
   case 'H': return { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
   case 'I': return { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F };
   case 'J': return { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C };
   case 'K': return { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 };
   case 'L': return { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F };
   case 'M': return { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 };
   case 'N': return { 0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11 };
   case 'O': return { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
   case 'P': return { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 };
   case 'Q': return { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D };
   case 'R': return { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 };
   case 'S': return { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
   case 'T': return { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
   case 'U': return { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
   case 'V': return { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 };
   case 'W': return { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 };
   case 'X': return { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 };
   case 'Y': return { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 };
   case 'Z': return { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F };
   case '0': return { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E };
   case '1': return { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E };
   case '2': return { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F };
   case '3': return { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E };
   case '4': return { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 };
   case '5': return { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E };
   case '6': return { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E };
   case '7': return { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 };
   case '8': return { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E };
   case '9': return { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E };
   case ':': return { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 };
   case '.': return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C };
   case '[': return { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E };
   case ']': return { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E };
   case '/': return { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 };
   case '-': return { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 };
   case '(': return { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 };
   case ')': return { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 };
   default:  return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
   }
}

/**
 \brief Fuegt Text mit der eingebauten 5 mal 7 Pixelschrift zur UI Vertexliste hinzu.
 \details Jedes gesetzte Pixel eines Zeichens wird als kleines Rechteck gerendert.
 \param vecVertices Zielcontainer der UI Vertices.
 \param svText Darzustellender ASCII Text.
 \param flX Startposition horizontal.
 \param flY Startposition vertikal.
 \param flPixelSize Groesse eines einzelnen Font Pixels.
 \param flRed Rotanteil der Textfarbe.
 \param flGreen Gruenanteil der Textfarbe.
 \param flBlue Blauanteil der Textfarbe.
 \param flAlpha Alphawert der Textfarbe.
 \returns Nichts.
 \throw Nichts.
 */
void AddUiText(std::vector<UiVertex>& vecVertices, std::string_view const svText,
   float flX, float const flY, float const flPixelSize,
   float const flRed, float const flGreen, float const flBlue, float const flAlpha) {
   for (char const chCharacter : svText) {
      std::array<std::uint8_t, 7> const arrRows = GlyphRows(chCharacter);

      for (std::size_t uRow = 0U; uRow < arrRows.size(); ++uRow) {
         for (std::size_t uColumn = 0U; uColumn < 5U; ++uColumn) {
            std::uint8_t const uMask = static_cast<std::uint8_t>(1U << (4U - uColumn));

            if ((arrRows[uRow] & uMask) == 0U) {
               continue;
            }

            UiRectangle const aPixel{
               flX + static_cast<float>(uColumn) * flPixelSize,
               flY + static_cast<float>(uRow) * flPixelSize,
               flPixelSize,
               flPixelSize
            };

            AddUiRectangle(vecVertices, aPixel, flRed, flGreen, flBlue, flAlpha);
         }
      }

      flX += flPixelSize * 6.0F;
   }
}

/**
 \brief Erzeugt den dynamischen OpenGL Buffer fuer die Benutzeroberflaeche.
 \details Position und Farbe werden aus einem gemeinsamen UiVertex Buffer gelesen.
 \param aBuffer Zielstruktur fuer die OpenGL Ressourcen.
 \returns true bei erfolgreicher Erzeugung.
 \throw Nichts.
 */
bool CreateUiBuffer(UiBuffer& aBuffer) noexcept {
   glGenVertexArrays(1, aBuffer.aVertexArray.Put());
   glGenBuffers(1, aBuffer.aVertexBuffer.Put());

   if (!aBuffer.aVertexArray || !aBuffer.aVertexBuffer) {
      return false;
      }

   glBindVertexArray(aBuffer.aVertexArray.Get());
   glBindBuffer(GL_ARRAY_BUFFER, aBuffer.aVertexBuffer.Get());
   glVertexAttribPointer(0U, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(UiVertex)), nullptr);
   glEnableVertexAttribArray(0U);
   glVertexAttribPointer(1U, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(UiVertex)),
      reinterpret_cast<void const*>(sizeof(float) * 2U));
   glEnableVertexAttribArray(1U);
   glBindVertexArray(0U);

   return true;
}

/**
 \brief Fuegt einen Button mit aktivem oder inaktivem Erscheinungsbild zur UI hinzu.
 \details Aktive Buttons verwenden das adecc Blau, inaktive Buttons ein neutrales Dunkelgrau.
 \param vecVertices Zielcontainer der UI Vertices.
 \param aRectangle Buttonrechteck.
 \param svText Beschriftung.
 \param fActive Kennzeichnet den aktuell ausgewaehlten Modus.
 \returns Nichts.
 \throw Nichts.
 */
void AddModeButton(std::vector<UiVertex>& vecVertices, UiRectangle const& aRectangle,
   std::string_view const svText, bool const fActive) {
   if (fActive) {
      AddUiRectangle(vecVertices, aRectangle, 0.0F, 0.321569F, 0.611765F, 0.96F);
   }
   else {
      AddUiRectangle(vecVertices, aRectangle, 0.22F, 0.24F, 0.27F, 0.90F);
   }

   float const flTextX = aRectangle.flX + 12.0F;
   float const flTextY = aRectangle.flY + 8.0F;
   AddUiText(vecVertices, svText, flTextX, flTextY, 1.6F,
      fActive ? 1.0F : 0.78F,
      fActive ? 1.0F : 0.81F,
      fActive ? 1.0F : 0.86F,
      1.0F);
}

/**
 \brief Uebertraegt UI Vertices in den dynamischen OpenGL Vertex Buffer.
 \details std::span entkoppelt die Upload Funktion vom konkreten Container.
 \param aBuffer Zielbuffer.
 \param spVertices Zu uebertragende UI Vertices.
 \returns Nichts.
 \throw Nichts.
 */
void UploadUiVertices(UiBuffer const& aBuffer, std::span<UiVertex const> const spVertices) noexcept {
   glBindVertexArray(aBuffer.aVertexArray.Get());
   glBindBuffer(GL_ARRAY_BUFFER, aBuffer.aVertexBuffer.Get());
   glBufferData(GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(spVertices.size_bytes()),
      spVertices.data(), GL_DYNAMIC_DRAW);
   }

/**
 \brief Zeichnet die gemeinsame Benutzeroberflaeche der Demo.
 \details Die Oberflaeche zeigt Titel, Steuerhinweise und drei Varianten. Beim Logo wird der KI Hinweis, bei Earth die Texturquelle und Lizenz am unteren Fensterrand eingeblendet.
 \param uProgram UI Shaderprogramm.
 \param aBuffer Dynamischer UI Buffer.
 \param iWindowWidth Logische SDL Fensterbreite.
 \param iWindowHeight Logische SDL Fensterhoehe.
 \param aMode Aktuell ausgewaehlte Demonstrationsvariante.
 \returns Nichts.
 \throw Nichts.
 */
void DrawUi(GLuint const uProgram, UiBuffer const& aBuffer,
   int const iWindowWidth, int const iWindowHeight, DemoMode const aMode) {
   if ((iWindowWidth <= 0) || (iWindowHeight <= 0)) {
      return;
      }

   UiRectangle const aPanel{ 18.0F, 18.0F, 650.0F, 146.0F };
   UiRectangle const aLogoButton{ 34.0F, 116.0F, 92.0F, 30.0F };
   UiRectangle const aEarthDayButton{ 138.0F, 116.0F, 150.0F, 30.0F };
   UiRectangle const aEarthNightButton{ 300.0F, 116.0F, 162.0F, 30.0F };
   std::vector<UiVertex> vecVertices;

   vecVertices.reserve(9000U);

   AddUiRectangle(vecVertices, aPanel, 0.035F, 0.045F, 0.060F, 0.82F);
   AddModeButton(vecVertices, aLogoButton, "Logo", aMode == DemoMode::Logo);
   AddModeButton(vecVertices, aEarthDayButton, "Earth (Day)", aMode == DemoMode::EarthDay);
   AddModeButton(vecVertices, aEarthNightButton, "Earth (Night)", aMode == DemoMode::EarthNight);

   AddUiText(vecVertices, "Demoprogramm OpenGL mit Shader und Licht",
      34.0F, 32.0F, 1.65F, 0.94F, 0.96F, 1.0F, 1.0F);

   if (aMode == DemoMode::Logo) {
      AddUiText(vecVertices, "Logo Kamera: LEFT/RIGHT drehen  UP/DOWN neigen",
         34.0F, 56.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);
      AddUiText(vecVertices, "Logo Licht: HOME/END drehen  PAGEUP/PAGEDOWN Hoehe",
         34.0F, 73.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);
      AddUiText(vecVertices, "Logo Abstand: + / - oder Mausrad",
         34.0F, 90.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);

      float const flCreditY = static_cast<float>((std::max)(iWindowHeight - 28, 4));
      AddUiText(vecVertices, "Mesh Modell des adecc Logos erzeugt von OpenAI ChatGPT KI",
         22.0F, flCreditY, 1.35F, 0.86F, 0.89F, 0.94F, 0.58F);
      }
   else {
      AddUiText(vecVertices, "Earth Licht: LEFT/RIGHT drehen  UP/DOWN neigen",
         34.0F, 56.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);
      AddUiText(vecVertices, "Earth Kamera: POS1/END links/rechts verschieben",
         34.0F, 73.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);
      AddUiText(vecVertices, "Earth Abstand: + / - oder Mausrad",
         34.0F, 90.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);

      float const flSourceY = static_cast<float>((std::max)(iWindowHeight - 43, 4));
      float const flLicenseY = static_cast<float>((std::max)(iWindowHeight - 24, 4));
      AddUiText(vecVertices, "Texturen: Solar System Scope - [https://www.solarsystemscope.com/textures/]",
         22.0F, flSourceY, 1.10F, 0.86F, 0.89F, 0.94F, 0.62F);
      AddUiText(vecVertices, "CC BY 4.0 - Attribution 4.0 International",
         22.0F, flLicenseY, 1.10F, 0.86F, 0.89F, 0.94F, 0.62F);
      }

   AddUiText(vecVertices, "ESC: Ende",
      500.0F, 90.0F, 1.25F, 0.82F, 0.86F, 0.92F, 1.0F);

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glUseProgram(uProgram);
   glUniform2f(glGetUniformLocation(uProgram, "uViewportSize"),
      static_cast<float>(iWindowWidth), static_cast<float>(iWindowHeight));

   UploadUiVertices(aBuffer, std::span<UiVertex const>{ vecVertices });
   glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vecVertices.size()));
   glBindVertexArray(0U);

   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   }

} // namespace

/**
 \brief Fuehrt die SDL2/OpenGL-Anwendung aus.
 \details Verwaltet Fenster, gemeinsame UI und die Auswahl der Demonstrationsvarianten. Logo und Earth Darstellung sowie deren Steuerung liegen in getrennten Szenenklassen.
 \returns EXIT_SUCCESS bei regulaerer Beendigung, sonst EXIT_FAILURE.
 \throw Nichts.
 */
// GUI logging: no output to the inherited console/stderr.
void SDLCALL GuiLogOutput([[maybe_unused]] void* context,
   [[maybe_unused]] int category, SDL_LogPriority priority, const char* message) {
   std::string const line = std::format("SDL [{}] {}\n",
      static_cast<int>(priority), message ? message : "");
   OutputDebugStringA(line.c_str());
}

int RunApplication() {
   SDL_LogSetOutputFunction(GuiLogOutput, nullptr);
   SDL_SetMainReady();

   SdlRuntime aSdlRuntime;

   if (!aSdlRuntime.Initialize(SDL_INIT_VIDEO)) {
      DebugPrint(std::format("SDL_Init fehlgeschlagen: {}", SDL_GetError()));
      return EXIT_FAILURE;
      }

   UniqueSdlWindow pWindow;
   UniqueSdlGlContext pContext;

   if (!InitializeWindow(pWindow, pContext)) {
      return EXIT_FAILURE;
      }

   if (!InitializeOpenGL()) {
      return EXIT_FAILURE;
      }

   LogoScene aLogoScene;

   if (!aLogoScene.Initialize()) {
      return EXIT_FAILURE;
      }

   EarthScene aEarthScene;
   bool fEarthInitialized = false;

   UniqueGlProgram aUiProgram = CreateUiProgram();
   UiBuffer aUiBuffer{};

   if (!aUiProgram || !CreateUiBuffer(aUiBuffer)) {
      DebugPrint("Die OpenGL Ressourcen fuer die Benutzeroberflaeche konnten nicht erzeugt werden.");
      return EXIT_FAILURE;
      }

   UiRectangle const aLogoButton{ 34.0F, 116.0F, 92.0F, 30.0F };
   UiRectangle const aEarthDayButton{ 138.0F, 116.0F, 150.0F, 30.0F };
   UiRectangle const aEarthNightButton{ 300.0F, 116.0F, 162.0F, 30.0F };
   DemoMode aMode = DemoMode::Logo;
   Uint64 uPreviousCounter = SDL_GetPerformanceCounter();
   double const flCounterFrequency = static_cast<double>(SDL_GetPerformanceFrequency());
   bool fRunning = true;
   Uint64 uStatsStart = uPreviousCounter;
   Uint64 uStatsFrames = 0;
   double flStatsFrameMs = 0.0;
   double flStatsSwapMs = 0.0;

   while (fRunning) {
      SDL_Event aEvent{};

      while (SDL_PollEvent(&aEvent) != 0) {
         if (aEvent.type == SDL_QUIT) {
            fRunning = false;
            }
         else if ((aEvent.type == SDL_KEYDOWN) && (aEvent.key.keysym.sym == SDLK_F9) && aEvent.key.repeat == 0) {
            int const iNext = SDL_GL_GetSwapInterval() == 0 ? 1 : 0;
            if (SDL_GL_SetSwapInterval(iNext) != 0)
               DebugPrint(std::format("Cannot change VSync: {}", SDL_GetError()));
            DebugPrint(std::format("VSync interval: {}", SDL_GL_GetSwapInterval()));
            }
         else if ((aEvent.type == SDL_KEYDOWN) && (aEvent.key.keysym.sym == SDLK_ESCAPE)) {
            fRunning = false;
            }
         else if ((aEvent.type == SDL_MOUSEBUTTONDOWN)
            && (aEvent.button.button == SDL_BUTTON_LEFT)) {
            if (IsInside(aLogoButton, aEvent.button.x, aEvent.button.y)) {
               aMode = DemoMode::Logo;
               DebugPrint("Logo ausgewaehlt.");
               }
            else if (IsInside(aEarthDayButton, aEvent.button.x, aEvent.button.y)) {
               if (fEarthInitialized || aEarthScene.Initialize()) {
                  fEarthInitialized = true;
                  aEarthScene.SetMode(EarthTextureMode::Day);
                  aMode = DemoMode::EarthDay;
                  DebugPrint("Earth Day ausgewaehlt.");
                  }
               else {
                  DebugPrint("Earth Szene konnte nicht initialisiert werden.");
                  }
               }
            else if (IsInside(aEarthNightButton, aEvent.button.x, aEvent.button.y)) {
               if (fEarthInitialized || aEarthScene.Initialize()) {
                  fEarthInitialized = true;
                  aEarthScene.SetMode(EarthTextureMode::Night);
                  aMode = DemoMode::EarthNight;
                  DebugPrint("Earth Night ausgewaehlt.");
                  }
               else {
                  DebugPrint("Earth Szene konnte nicht initialisiert werden.");
                  }
               }
            }

         if (aMode == DemoMode::Logo) {
            aLogoScene.ProcessEvent(aEvent);
            }
         else {
            aEarthScene.ProcessEvent(aEvent);
            }
         }

      Uint64 const uCurrentCounter = SDL_GetPerformanceCounter();
      float const flDeltaTime = static_cast<float>(
         static_cast<double>(uCurrentCounter - uPreviousCounter) / flCounterFrequency);
      uPreviousCounter = uCurrentCounter;

      float const flFrameDelta = (std::min)(flDeltaTime, 0.05F);

      if (aMode == DemoMode::Logo) {
         aLogoScene.ProcessInput(flFrameDelta);
         }
      else {
         aEarthScene.ProcessInput(flFrameDelta);
         aEarthScene.Update(flFrameDelta);
         }

      int iDrawableWidth = 0;
      int iDrawableHeight = 0;
      int iWindowWidth = 0;
      int iWindowHeight = 0;

      SDL_GL_GetDrawableSize(pWindow.get(), &iDrawableWidth, &iDrawableHeight);
      SDL_GetWindowSize(pWindow.get(), &iWindowWidth, &iWindowHeight);

      iDrawableWidth = (std::max)(iDrawableWidth, 1);
      iDrawableHeight = (std::max)(iDrawableHeight, 1);

      glViewport(0, 0, iDrawableWidth, iDrawableHeight);
      glClearColor(0.13F, 0.15F, 0.18F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (aMode == DemoMode::Logo) {
         aLogoScene.Render(iDrawableWidth, iDrawableHeight);
         }
      else {
         aEarthScene.Render(iDrawableWidth, iDrawableHeight);
         }

      DrawUi(aUiProgram.Get(), aUiBuffer, iWindowWidth, iWindowHeight, aMode);
      Uint64 const uSwapBegin = SDL_GetPerformanceCounter();
      SDL_GL_SwapWindow(pWindow.get());
      Uint64 const uSwapEnd = SDL_GetPerformanceCounter();
      ++uStatsFrames;
      flStatsFrameMs += 1000.0 * static_cast<double>(uSwapEnd - uCurrentCounter) / flCounterFrequency;
      flStatsSwapMs += 1000.0 * static_cast<double>(uSwapEnd - uSwapBegin) / flCounterFrequency;
      double const flPeriod = static_cast<double>(uSwapEnd - uStatsStart) / flCounterFrequency;
      if (flPeriod >= 3.0) {
         DebugPrint(std::format(
            "PERF scene={} fps={:.1f} frame_ms={:.2f} swap_ms={:.2f} vsync={} viewport={}x{}",
            aMode == DemoMode::Logo ? "Logo" : "Earth",
            static_cast<double>(uStatsFrames) / flPeriod,
            flStatsFrameMs / static_cast<double>(uStatsFrames),
            flStatsSwapMs / static_cast<double>(uStatsFrames),
            SDL_GL_GetSwapInterval(), iDrawableWidth, iDrawableHeight));
         uStatsStart = uSwapEnd;
         uStatsFrames = 0;
         flStatsFrameMs = 0.0;
         flStatsSwapMs = 0.0;
         }
      }

   /*
    Die Zerstoerungsreihenfolge ist absichtlich RAII-basiert:
    UI, EarthScene und LogoScene werden vor dem OpenGL Kontext zerstoert,
    danach Fenster und zuletzt die SDL Runtime.
   */
   return EXIT_SUCCESS;
   }

/**
 \brief Windows GUI Einstiegspunkt ohne separates Konsolenfenster.
 \details SDL_MAIN_HANDLED ist fuer dieses Target gesetzt; SDL_SetMainReady wird in RunApplication aufgerufen.
 \returns Rueckgabecode der Anwendung.
 \throw Nichts.
 */
// NVIDIA Optimus and AMD switchable-graphics hints. Windows and the GPU
// driver retain final authority over the selected adapter.
extern "C" {
   __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001UL;
   __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain([[maybe_unused]] HINSTANCE const hInstance,
   [[maybe_unused]] HINSTANCE const hPrevInstance,
   [[maybe_unused]] LPSTR const pszCommandLine,
   [[maybe_unused]] int const iShowCommand) {
   return RunApplication();
   }
