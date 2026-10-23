#include "LogoScene.h"
#include "OpenGLResource.h"

#include <GL/glew.h>

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {


/**
 \brief Dreidimensionaler Vektor fuer die Kamera, das Licht und die Matrixberechnung.
 \details Die Struktur enthaelt nur die fuer dieses Beispiel benoetigten Vektoroperationen.
 */
struct Vector3 {
   float flX{};
   float flY{};
   float flZ{};
};

/**
 \brief Vierdimensionale Matrix im von OpenGL erwarteten Column Major Layout.
 \details Die Matrix wird direkt mit glUniformMatrix4fv an GLSL uebergeben.
 */
struct Matrix4 {
   std::array<float, 16> arrValues{};
};

/**
 \brief Ein Vertex des geladenen OBJ Meshs.
 \details Fuer die Beleuchtung werden Position und Flaechennormale benoetigt.
 */
struct MeshVertex {
   Vector3 aPosition{};
   Vector3 aNormal{};
};

/**
 \brief CPU Repraesentation eines geladenen Dreiecksmeshs.
 \details Die OBJ Datei wird in einen gemeinsamen Vertex und Index Buffer ueberfuehrt.
 */
struct MeshData {
   std::vector<MeshVertex> vecVertices;
   std::vector<GLuint> vecIndices;
   Vector3 aMinimum{};
   Vector3 aMaximum{};
};

/**
 \brief OpenGL Ressourcen fuer ein Mesh.
 \details Vertex Array, Vertex Buffer und Element Buffer werden gemeinsam verwaltet.
 */
struct MeshBuffer {
   UniqueGlVertexArray aVertexArray;
   UniqueGlBuffer aVertexBuffer;
   UniqueGlBuffer aElementBuffer;
   GLsizei iIndexCount{};
};

/**
 \brief Sphaerische Steuerparameter der Kamera.
 \details Die Kamera bewegt sich auf einer Kugelflaeche um das Zentrum des Meshs.
 */
struct CameraState {
   float flAzimuth{};
   float flElevation{};
   float flRadius{};
};

/**
 \brief Sphaerische Steuerparameter der Lichtquelle.
 \details Die Punktlichtquelle wird auf einer Kugelflaeche um das Mesh verschoben.
 */
struct LightState {
   float flAzimuth{};
   float flElevation{};
   float flRadius{};
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
 \brief Addiert zwei dreidimensionale Vektoren.
 \details Die Addition erfolgt komponentenweise.
 \param aLeft Linker Operand.
 \param aRight Rechter Operand.
 \returns Die Summe beider Vektoren.
 \throw Nichts.
 */
Vector3 Add(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return Vector3{
      aLeft.flX + aRight.flX,
      aLeft.flY + aRight.flY,
      aLeft.flZ + aRight.flZ
   };
}

/**
 \brief Subtrahiert zwei dreidimensionale Vektoren.
 \details Die Subtraktion erfolgt komponentenweise.
 \param aLeft Linker Operand.
 \param aRight Rechter Operand.
 \returns Die Differenz beider Vektoren.
 \throw Nichts.
 */
Vector3 Subtract(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return Vector3{
      aLeft.flX - aRight.flX,
      aLeft.flY - aRight.flY,
      aLeft.flZ - aRight.flZ
   };
}

/**
 \brief Multipliziert einen dreidimensionalen Vektor mit einem Skalar.
 \details Jede Komponente wird mit demselben Faktor multipliziert.
 \param aVector Zu skalierender Vektor.
 \param flFactor Skalierungsfaktor.
 \returns Den skalierten Vektor.
 \throw Nichts.
 */
Vector3 Multiply(Vector3 const& aVector, float const flFactor) noexcept {
   return Vector3{
      aVector.flX * flFactor,
      aVector.flY * flFactor,
      aVector.flZ * flFactor
   };
}

/**
 \brief Berechnet das Skalarprodukt zweier Vektoren.
 \details Das Ergebnis wird fuer Blickmatrix und Beleuchtung verwendet.
 \param aLeft Linker Operand.
 \param aRight Rechter Operand.
 \returns Das Skalarprodukt.
 \throw Nichts.
 */
float Dot(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return aLeft.flX * aRight.flX + aLeft.flY * aRight.flY + aLeft.flZ * aRight.flZ;
}

/**
 \brief Berechnet das Kreuzprodukt zweier Vektoren.
 \details Das Ergebnis steht senkrecht auf beiden Operanden.
 \param aLeft Linker Operand.
 \param aRight Rechter Operand.
 \returns Das Kreuzprodukt.
 \throw Nichts.
 */
Vector3 Cross(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return Vector3{
      aLeft.flY * aRight.flZ - aLeft.flZ * aRight.flY,
      aLeft.flZ * aRight.flX - aLeft.flX * aRight.flZ,
      aLeft.flX * aRight.flY - aLeft.flY * aRight.flX
   };
}

/**
 \brief Normalisiert einen dreidimensionalen Vektor.
 \details Ein Nullvektor wird unveraendert zurueckgegeben.
 \param aVector Zu normalisierender Vektor.
 \returns Einen Vektor der Laenge eins oder den Nullvektor.
 \throw Nichts.
 */
Vector3 Normalize(Vector3 const& aVector) noexcept {
   float const flLength = std::sqrt(Dot(aVector, aVector));

   if (flLength <= std::numeric_limits<float>::epsilon()) {
      return aVector;
   }

   return Multiply(aVector, 1.0F / flLength);
}

/**
 \brief Erzeugt eine Einheitsmatrix.
 \details Die Matrix ist fuer OpenGL im Column Major Layout abgelegt.
 \returns Die vierdimensionale Einheitsmatrix.
 \throw Nichts.
 */
Matrix4 IdentityMatrix() noexcept {
   Matrix4 aMatrix{};

   aMatrix.arrValues[0] = 1.0F;
   aMatrix.arrValues[5] = 1.0F;
   aMatrix.arrValues[10] = 1.0F;
   aMatrix.arrValues[15] = 1.0F;

   return aMatrix;
}

/**
 \brief Multipliziert zwei vierdimensionale Matrizen.
 \details Beide Matrizen und das Ergebnis verwenden das OpenGL Column Major Layout.
 \param aLeft Linke Matrix.
 \param aRight Rechte Matrix.
 \returns Das Matrixprodukt aLeft mal aRight.
 \throw Nichts.
 */
Matrix4 Multiply(Matrix4 const& aLeft, Matrix4 const& aRight) noexcept {
   Matrix4 aResult{};

   for (int iColumn = 0; iColumn < 4; ++iColumn) {
      for (int iRow = 0; iRow < 4; ++iRow) {
         float flValue = 0.0F;

         for (int iElement = 0; iElement < 4; ++iElement) {
            flValue += aLeft.arrValues[iElement * 4 + iRow]
               * aRight.arrValues[iColumn * 4 + iElement];
         }

         aResult.arrValues[iColumn * 4 + iRow] = flValue;
      }
   }

   return aResult;
}

/**
 \brief Erzeugt eine Translationsmatrix.
 \details Die Translation wird in die letzte Spalte der OpenGL Matrix eingetragen.
 \param aTranslation Gewuenschte Verschiebung.
 \returns Die Translationsmatrix.
 \throw Nichts.
 */
Matrix4 TranslationMatrix(Vector3 const& aTranslation) noexcept {
   Matrix4 aMatrix = IdentityMatrix();

   aMatrix.arrValues[12] = aTranslation.flX;
   aMatrix.arrValues[13] = aTranslation.flY;
   aMatrix.arrValues[14] = aTranslation.flZ;

   return aMatrix;
}

/**
 \brief Erzeugt eine perspektivische Projektionsmatrix.
 \details Die Matrix entspricht der OpenGL Perspektivprojektion mit symmetrischem Frustum.
 \param flFieldOfViewY Vertikaler Oeffnungswinkel im Bogenmass.
 \param flAspectRatio Seitenverhaeltnis Breite zu Hoehe.
 \param flNearPlane Abstand der vorderen Clipping Ebene.
 \param flFarPlane Abstand der hinteren Clipping Ebene.
 \returns Die Projektionsmatrix.
 \throw Nichts.
 */
Matrix4 PerspectiveMatrix(float const flFieldOfViewY, float const flAspectRatio,
   float const flNearPlane, float const flFarPlane) noexcept {
   Matrix4 aMatrix{};
   float const flFactor = 1.0F / std::tan(flFieldOfViewY * 0.5F);

   aMatrix.arrValues[0] = flFactor / flAspectRatio;
   aMatrix.arrValues[5] = flFactor;
   aMatrix.arrValues[10] = (flFarPlane + flNearPlane) / (flNearPlane - flFarPlane);
   aMatrix.arrValues[11] = -1.0F;
   aMatrix.arrValues[14] = (2.0F * flFarPlane * flNearPlane) / (flNearPlane - flFarPlane);

   return aMatrix;
}

/**
 \brief Erzeugt eine Look At Blickmatrix.
 \details Die Kamera bleibt auf das Zentrum des Meshs ausgerichtet, waehrend ihre Position um das Mesh rotiert.
 \param aEye Position der Kamera im Weltkoordinatensystem.
 \param aTarget Zielpunkt der Kamera.
 \param aUp Aufwaertsrichtung der Welt.
 \returns Die Blickmatrix.
 \throw Nichts.
 */
Matrix4 LookAtMatrix(Vector3 const& aEye, Vector3 const& aTarget, Vector3 const& aUp) noexcept {
   Vector3 const aForward = Normalize(Subtract(aTarget, aEye));
   Vector3 const aSide = Normalize(Cross(aForward, aUp));
   Vector3 const aCorrectedUp = Cross(aSide, aForward);
   Matrix4 aMatrix = IdentityMatrix();

   aMatrix.arrValues[0] = aSide.flX;
   aMatrix.arrValues[1] = aCorrectedUp.flX;
   aMatrix.arrValues[2] = -aForward.flX;

   aMatrix.arrValues[4] = aSide.flY;
   aMatrix.arrValues[5] = aCorrectedUp.flY;
   aMatrix.arrValues[6] = -aForward.flY;

   aMatrix.arrValues[8] = aSide.flZ;
   aMatrix.arrValues[9] = aCorrectedUp.flZ;
   aMatrix.arrValues[10] = -aForward.flZ;

   aMatrix.arrValues[12] = -Dot(aSide, aEye);
   aMatrix.arrValues[13] = -Dot(aCorrectedUp, aEye);
   aMatrix.arrValues[14] = Dot(aForward, aEye);

   return aMatrix;
}

/**
 \brief Erzeugt eine Projektionsmatrix fuer einen planaren Schatten.
 \details Das Mesh wird aus Sicht einer Punktlichtquelle auf eine horizontale Ebene projiziert.
 \param aLightPosition Position der Punktlichtquelle im Weltkoordinatensystem.
 \param flPlaneY Hoehe der Schattenebene.
 \returns Die Schattenprojektionsmatrix.
 \throw Nichts.
 */
Matrix4 ShadowMatrix(Vector3 const& aLightPosition, float const flPlaneY) noexcept {
   std::array<float, 4> const arrPlane{ 0.0F, 1.0F, 0.0F, -flPlaneY };
   std::array<float, 4> const arrLight{ aLightPosition.flX, aLightPosition.flY, aLightPosition.flZ, 1.0F };
   float const flDot = arrPlane[0] * arrLight[0]
      + arrPlane[1] * arrLight[1]
      + arrPlane[2] * arrLight[2]
      + arrPlane[3] * arrLight[3];
   Matrix4 aMatrix{};

   for (int iColumn = 0; iColumn < 4; ++iColumn) {
      for (int iRow = 0; iRow < 4; ++iRow) {
         float const flIdentity = iRow == iColumn ? flDot : 0.0F;
         aMatrix.arrValues[iColumn * 4 + iRow] = flIdentity - arrLight[iRow] * arrPlane[iColumn];
      }
   }

   return aMatrix;
}

/**
 \brief Berechnet eine kartesische Position aus sphaerischen Orbit Parametern.
 \details Azimut und Elevation drehen eine Position mit konstantem Radius um das Zentrum.
 \param flAzimuth Horizontaler Winkel im Bogenmass.
 \param flElevation Vertikaler Winkel im Bogenmass.
 \param flRadius Abstand zum Zentrum.
 \returns Die kartesische Position relativ zum Zentrum.
 \throw Nichts.
 */
Vector3 OrbitPosition(float const flAzimuth, float const flElevation, float const flRadius) noexcept {
   float const flHorizontalRadius = flRadius * std::cos(flElevation);

   return Vector3{
      flHorizontalRadius * std::sin(flAzimuth),
      flRadius * std::sin(flElevation),
      flHorizontalRadius * std::cos(flAzimuth)
   };
}

/**
 \brief Kompiliert einen OpenGL Shader.
 \details Der uebergebene GLSL Quelltext wird kompiliert und bei einem Fehler wird die Compilerdiagnose im Debugausgabefenster ausgegeben.
 \param uShaderType OpenGL Shadertyp.
 \param szSource Nullterminierter GLSL Quelltext.
 \returns Die OpenGL Shaderkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlShader CompileShader(GLenum const uShaderType, char const* const szSource) noexcept {
   UniqueGlShader aShader{ glCreateShader(uShaderType) };

   if (!aShader) {
      DebugPrint("OpenGL Shader konnte nicht erzeugt werden.");
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
   GLsizei iLength = 0;

   glGetShaderInfoLog(uShader, static_cast<GLsizei>(arrLog.size()), &iLength, arrLog.data());
   DebugPrint(std::format("Shader compilation failed: {}", arrLog.data()));

   return {};
   }

/**
 \brief Verbindet Vertex und Fragment Shader zu einem Shaderprogramm.
 \details Beide Shader werden nach dem Linken freigegeben, das fertige Programm bleibt erhalten.
 \param szVertexShader Quelltext des Vertex Shaders.
 \param szFragmentShader Quelltext des Fragment Shaders.
 \returns Die OpenGL Programmkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlProgram CreateProgram(char const* const szVertexShader, char const* const szFragmentShader) noexcept {
   UniqueGlShader aVertexShader = CompileShader(GL_VERTEX_SHADER, szVertexShader);

   if (!aVertexShader) {
      return {};
      }

   UniqueGlShader aFragmentShader = CompileShader(GL_FRAGMENT_SHADER, szFragmentShader);

   if (!aFragmentShader) {
      return {};
      }

   UniqueGlProgram aProgram{ glCreateProgram() };

   if (!aProgram) {
      DebugPrint("OpenGL Shaderprogramm konnte nicht erzeugt werden.");
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
   GLsizei iLength = 0;

   glGetProgramInfoLog(uProgram, static_cast<GLsizei>(arrLog.size()), &iLength, arrLog.data());
   DebugPrint(std::format("Program linking failed: {}", arrLog.data()));

   return {};
   }

/**
 \brief Erzeugt das Shaderprogramm fuer das beleuchtete Mesh und den Boden.
 \details Der Fragment Shader berechnet fuer das Logo eine metallische Beleuchtung mit farbigem Fresnel Anteil und scharfem Glanzlicht. Die Lichtquelle arbeitet als auf das Mesh gerichteter Scheinwerfer mit weichem Rand. Der Boden kann ueber die Uniforms weiterhin als nichtmetallisches Material gerendert werden.
 \returns Die OpenGL Programmkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlProgram CreateLightingProgram() noexcept {
   constexpr char const* szVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;

void main() {
   vec4 aWorldPosition = uModel * vec4(aPosition, 1.0);
   mat3 aNormalMatrix = transpose(inverse(mat3(uModel)));

   vWorldPosition = aWorldPosition.xyz;
   vNormal = normalize(aNormalMatrix * aNormal);
   gl_Position = uProjection * uView * aWorldPosition;
}
)GLSL";

   constexpr char const* szFragmentShader = R"GLSL(
#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;

uniform vec3 uLightPosition;
uniform vec3 uSpotDirection;
uniform vec3 uCameraPosition;
uniform vec3 uBaseColor;
uniform float uSpecularStrength;
uniform float uShininess;
uniform float uMetallic;
uniform float uAmbientStrength;
uniform float uSpotInnerCutoff;
uniform float uSpotOuterCutoff;

out vec4 aColor;

void main() {
   vec3 aNormal = normalize(vNormal);
   vec3 aLightDirection = normalize(uLightPosition - vWorldPosition);
   vec3 aLightToFragment = normalize(vWorldPosition - uLightPosition);
   vec3 aSpotDirection = normalize(uSpotDirection);
   vec3 aViewDirection = normalize(uCameraPosition - vWorldPosition);
   vec3 aHalfDirection = normalize(aLightDirection + aViewDirection);

   float flSpotCosine = dot(aLightToFragment, aSpotDirection);
   float flSpotFactor = smoothstep(uSpotOuterCutoff, uSpotInnerCutoff, flSpotCosine);
   float flNormalLight = max(dot(aNormal, aLightDirection), 0.0);
   float flDirectLight = flNormalLight * flSpotFactor;
   float flNormalView = max(dot(aNormal, aViewDirection), 0.0);
   float flNormalHalf = max(dot(aNormal, aHalfDirection), 0.0);

   vec3 aFresnelBase = mix(vec3(0.04), uBaseColor, uMetallic);
   float flFresnelFactor = pow(1.0 - flNormalView, 5.0);
   vec3 aFresnel = aFresnelBase + (vec3(1.0) - aFresnelBase) * flFresnelFactor;

   float flSpecular = pow(flNormalHalf, uShininess)
      * uSpecularStrength * flDirectLight;

   float flDiffuseStrength = (1.0 - uMetallic) * flDirectLight * 0.82;
   float flMetalBodyStrength = uMetallic * (0.035 + flDirectLight * 0.58);
   vec3 aAmbientColor = uBaseColor * uAmbientStrength;
   vec3 aDiffuseColor = uBaseColor * flDiffuseStrength;
   vec3 aMetalBodyColor = uBaseColor * flMetalBodyStrength;
   vec3 aHighlightColor = mix(vec3(0.95), uBaseColor * 1.35 + vec3(0.20), 0.45);
   vec3 aSpecularColor = aHighlightColor * aFresnel * flSpecular;

   float flEdgeStrength = pow(1.0 - flNormalView, 3.0) * 0.22 * uMetallic * flSpotFactor;
   vec3 aEdgeReflection = aHighlightColor * aFresnel * flEdgeStrength;
   vec3 aFinalColor = aAmbientColor + aDiffuseColor + aMetalBodyColor + aSpecularColor + aEdgeReflection;

   aColor = vec4(min(aFinalColor, vec3(1.0)), 1.0);
}
)GLSL";

   return CreateProgram(szVertexShader, szFragmentShader);
}

/**
 \brief Erzeugt das Shaderprogramm fuer den planaren Schatten.
 \details Das Schattenmesh wird bereits durch die Modellmatrix auf die Bodenebene projiziert und halbtransparent gezeichnet.
 \returns Die OpenGL Programmkennung oder 0 bei einem Fehler.
 \throw Nichts.
 */
UniqueGlProgram CreateShadowProgram() noexcept {
   constexpr char const* szVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
   gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)GLSL";

   constexpr char const* szFragmentShader = R"GLSL(
#version 330 core

out vec4 aColor;

void main() {
   aColor = vec4(0.0, 0.0, 0.0, 0.34);
}
)GLSL";

   return CreateProgram(szVertexShader, szFragmentShader);
}

/**
 \brief Ermittelt einen Pfad relativ zum Verzeichnis der ausfuehrbaren Datei.
 \details SDL_GetBasePath liefert das Verzeichnis der gestarteten Anwendung. Dadurch ist das Laden von Ressourcen unabhaengig vom aktuellen Arbeitsverzeichnis.
 \param svRelativePath Relativer Pfad unterhalb des Anwendungsverzeichnisses.
 \returns Den vollstaendigen Dateisystempfad.
 \throw std::filesystem::filesystem_error Falls die Ermittlung des Ersatzpfades fehlschlaegt.
 */
std::filesystem::path ExecutablePath(std::string_view const svRelativePath) {
   char* const szBasePath = SDL_GetBasePath();

   if (szBasePath == nullptr) {
      return std::filesystem::current_path() / svRelativePath;
   }

   std::filesystem::path const aBasePath{ szBasePath };

   SDL_free(szBasePath);

   return aBasePath / svRelativePath;
}

/**
 \brief Ermittelt den Pfad einer Ressource in typischen Entwicklungs und Laufzeitverzeichnissen.
 \details Zuerst wird relativ zur ausfuehrbaren Datei gesucht. Danach folgen das aktuelle Arbeitsverzeichnis und das Projektverzeichnis, das aus __FILE__ abgeleitet wird.
 \param svRelativePath Relativer Ressourcenpfad, beispielsweise graphics/adecc_official_logo_3d.obj.
 \returns Den ersten existierenden Pfad oder einen leeren Pfad, wenn die Ressource nicht gefunden wurde.
 \throw Nichts.
 */
std::filesystem::path ResourcePath(std::string_view const svRelativePath) noexcept {
   std::error_code aErrorCode;
   std::array<std::filesystem::path, 3> const arrCandidates{
      ExecutablePath(svRelativePath),
      std::filesystem::current_path(aErrorCode) / svRelativePath,
      std::filesystem::path { __FILE__ }.parent_path().parent_path() / svRelativePath
   };

   for (std::filesystem::path const& aCandidate : arrCandidates) {
      aErrorCode.clear();

      if (std::filesystem::is_regular_file(aCandidate, aErrorCode)) {
         DebugPrint(std::format("Ressource gefunden: {}", aCandidate.string()));
         return aCandidate;
      }

      DebugPrint(std::format("Ressource nicht gefunden: {}", aCandidate.string()));
   }

   return {};
}

/**
 \brief Zeigt einen fatalen Laufzeitfehler sichtbar an.
 \details Die Meldung wird sowohl in das SDL Log als auch in eine SDL MessageBox geschrieben.
 \param svMessage Anzuzeigende Fehlermeldung.
 \returns Nichts.
 \throw Nichts.
 */
void ShowFatalError(std::string_view const svMessage) noexcept {
   DebugPrint(svMessage);
   std::string const strMessage{ svMessage };
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "adecc OpenGL", strMessage.c_str(), nullptr);
}

/**
 \brief Zerlegt einen OBJ Face Eintrag in Positions und Normalenindex.
 \details Unterstuetzt die fuer das erzeugte Mesh verwendete Schreibweise position//normal sowie position/texture/normal.
 \param svToken Einzelner Face Eintrag aus der OBJ Datei.
 \param iPositionIndex Ergebnis fuer den Positionsindex.
 \param iNormalIndex Ergebnis fuer den Normalenindex.
 \returns true wenn beide Indizes gelesen werden konnten.
 \throw Nichts.
 */
bool ParseFaceToken(std::string_view const svToken, int& iPositionIndex, int& iNormalIndex) noexcept {
   std::size_t const uFirstSlash = svToken.find('/');

   if (uFirstSlash == std::string_view::npos) {
      return false;
   }

   std::size_t const uSecondSlash = svToken.find('/', uFirstSlash + 1U);

   if (uSecondSlash == std::string_view::npos) {
      return false;
   }

   std::string const strPosition{ svToken.substr(0U, uFirstSlash) };
   std::string const strNormal{ svToken.substr(uSecondSlash + 1U) };

   if (strPosition.empty() || strNormal.empty()) {
      return false;
   }

   try {
      iPositionIndex = std::stoi(strPosition);
      iNormalIndex = std::stoi(strNormal);
   }
   catch (...) {
      return false;
   }

   return (iPositionIndex > 0) && (iNormalIndex > 0);
}

/**
 \brief Laedt das adecc OBJ Mesh von einem Dateipfad.
 \details Positionen, Normalen und Dreiecksindizes werden eingelesen. Das erzeugte OBJ verwendet identische Positions und Normalenindizes, was beim Laden validiert wird.
 \param aPath Pfad zur OBJ Datei.
 \param aMesh Zielstruktur fuer das geladene Mesh.
 \returns true wenn das Mesh vollstaendig geladen wurde.
 \throw std::ios_base::failure Wird intern abgefangen und als false gemeldet.
 */
bool LoadObjMesh(std::filesystem::path const& aPath, MeshData& aMesh) noexcept {
   std::ifstream aInput(aPath);

   if (!aInput) {
      DebugPrint(std::format("OBJ Datei '{}' konnte nicht geoeffnet werden.", aPath.string()));
      return false;
   }

   std::vector<Vector3> vecPositions;
   std::vector<Vector3> vecNormals;
   std::vector<GLuint> vecIndices;
   std::string strLine;

   try {
      while (std::getline(aInput, strLine)) {
         std::istringstream aLine(strLine);
         std::string strType;

         aLine >> strType;

         if (strType == "v") {
            Vector3 aPosition{};
            aLine >> aPosition.flX >> aPosition.flY >> aPosition.flZ;
            vecPositions.push_back(aPosition);
         }
         else if (strType == "vn") {
            Vector3 aNormal{};
            aLine >> aNormal.flX >> aNormal.flY >> aNormal.flZ;
            vecNormals.push_back(Normalize(aNormal));
         }
         else if (strType == "f") {
            std::array<std::string, 3> arrFaceTokens{};
            aLine >> arrFaceTokens[0] >> arrFaceTokens[1] >> arrFaceTokens[2];

            for (std::string const& strToken : arrFaceTokens) {
               int iPositionIndex = 0;
               int iNormalIndex = 0;

               if (!ParseFaceToken(strToken, iPositionIndex, iNormalIndex)) {
                  DebugPrint(std::format("Ungueltiger OBJ Face Eintrag '{}'.", strToken));
                  return false;
               }

               if (iPositionIndex != iNormalIndex) {
                  DebugPrint("Das OBJ Mesh verwendet unterschiedliche Positions und Normalenindizes.");
                  return false;
               }

               vecIndices.push_back(static_cast<GLuint>(iPositionIndex - 1));
            }
         }
      }
   }
   catch (std::ios_base::failure const&) {
      DebugPrint(std::format("Fehler beim Lesen der OBJ Datei '{}'.", aPath.string()));
      return false;
   }

   if (vecPositions.empty() || vecNormals.empty() || vecIndices.empty()) {
      DebugPrint("Die OBJ Datei enthaelt kein vollstaendiges Dreiecksmesh.");
      return false;
   }

   if (vecPositions.size() != vecNormals.size()) {
      DebugPrint("Anzahl der OBJ Positionen und Normalen stimmt nicht ueberein.");
      return false;
   }

   Vector3 aMinimum{
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()
   };
   Vector3 aMaximum{
      std::numeric_limits<float>::lowest(),
      std::numeric_limits<float>::lowest(),
      std::numeric_limits<float>::lowest()
   };
   std::vector<MeshVertex> vecVertices;

   vecVertices.reserve(vecPositions.size());

   for (std::size_t uIndex = 0U; uIndex < vecPositions.size(); ++uIndex) {
      Vector3 const& aPosition = vecPositions[uIndex];

      aMinimum.flX = (std::min)(aMinimum.flX, aPosition.flX);
      aMinimum.flY = (std::min)(aMinimum.flY, aPosition.flY);
      aMinimum.flZ = (std::min)(aMinimum.flZ, aPosition.flZ);

      aMaximum.flX = (std::max)(aMaximum.flX, aPosition.flX);
      aMaximum.flY = (std::max)(aMaximum.flY, aPosition.flY);
      aMaximum.flZ = (std::max)(aMaximum.flZ, aPosition.flZ);

      vecVertices.push_back(MeshVertex{ aPosition, vecNormals[uIndex] });
   }

   aMesh.vecVertices = std::move(vecVertices);
   aMesh.vecIndices = std::move(vecIndices);
   aMesh.aMinimum = aMinimum;
   aMesh.aMaximum = aMaximum;

   DebugPrint(std::format("OBJ Mesh geladen: {} Vertices, {} Dreiecke.",
      aMesh.vecVertices.size(), aMesh.vecIndices.size() / 3U));

   return true;
}

/**
 \brief Erzeugt OpenGL Buffer fuer ein Dreiecksmesh.
 \details Position und Normale werden als zwei getrennte Vertex Attribute eingerichtet.
 \param aMesh CPU Meshdaten.
 \param aBuffer Zielstruktur fuer die erzeugten OpenGL Objekte.
 \returns true wenn alle Buffer erzeugt wurden.
 \throw Nichts.
 */
bool CreateMeshBuffer(MeshData const& aMesh, MeshBuffer& aBuffer) noexcept {
   glGenVertexArrays(1, aBuffer.aVertexArray.Put());
   glGenBuffers(1, aBuffer.aVertexBuffer.Put());
   glGenBuffers(1, aBuffer.aElementBuffer.Put());

   if (!aBuffer.aVertexArray || !aBuffer.aVertexBuffer || !aBuffer.aElementBuffer) {
      return false;
      }

   glBindVertexArray(aBuffer.aVertexArray.Get());
   glBindBuffer(GL_ARRAY_BUFFER, aBuffer.aVertexBuffer.Get());
   glBufferData(GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(aMesh.vecVertices.size() * sizeof(MeshVertex)),
      aMesh.vecVertices.data(), GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aBuffer.aElementBuffer.Get());
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(aMesh.vecIndices.size() * sizeof(GLuint)),
      aMesh.vecIndices.data(), GL_STATIC_DRAW);

   glVertexAttribPointer(0U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(MeshVertex)), nullptr);
   glEnableVertexAttribArray(0U);

   glVertexAttribPointer(1U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(MeshVertex)),
      reinterpret_cast<void const*>(sizeof(Vector3)));
   glEnableVertexAttribArray(1U);

   glBindVertexArray(0U);

   aBuffer.iIndexCount = static_cast<GLsizei>(aMesh.vecIndices.size());

   return true;
}

/**
 \brief Erzeugt einen rechteckigen Boden unterhalb des Logos.
 \details Die Bodenflaeche dient als Empfaenger fuer den planaren Schatten.
 \param flPlaneY Hoehe der Bodenebene.
 \param aBuffer Zielstruktur fuer die erzeugten OpenGL Objekte.
 \returns true wenn die Buffer erzeugt wurden.
 \throw Nichts.
 */
bool CreateGroundBuffer(float const flPlaneY, MeshBuffer& aBuffer) noexcept {
   constexpr float flHalfWidth = 8.0F;
   constexpr float flHalfDepth = 5.0F;
   Vector3 const aNormal{ 0.0F, 1.0F, 0.0F };
   std::array<MeshVertex, 4> const arrVertices{
      MeshVertex { Vector3 { -flHalfWidth, flPlaneY, -flHalfDepth }, aNormal },
      MeshVertex { Vector3 { -flHalfWidth, flPlaneY,  flHalfDepth }, aNormal },
      MeshVertex { Vector3 {  flHalfWidth, flPlaneY,  flHalfDepth }, aNormal },
      MeshVertex { Vector3 {  flHalfWidth, flPlaneY, -flHalfDepth }, aNormal }
   };
   constexpr std::array<GLuint, 6> arrIndices{
      0U, 1U, 2U,
      2U, 3U, 0U
   };

   glGenVertexArrays(1, aBuffer.aVertexArray.Put());
   glGenBuffers(1, aBuffer.aVertexBuffer.Put());
   glGenBuffers(1, aBuffer.aElementBuffer.Put());

   if (!aBuffer.aVertexArray || !aBuffer.aVertexBuffer || !aBuffer.aElementBuffer) {
      return false;
      }

   glBindVertexArray(aBuffer.aVertexArray.Get());
   glBindBuffer(GL_ARRAY_BUFFER, aBuffer.aVertexBuffer.Get());
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(arrVertices)), arrVertices.data(), GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aBuffer.aElementBuffer.Get());
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(arrIndices)), arrIndices.data(), GL_STATIC_DRAW);

   glVertexAttribPointer(0U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(MeshVertex)), nullptr);
   glEnableVertexAttribArray(0U);

   glVertexAttribPointer(1U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(MeshVertex)),
      reinterpret_cast<void const*>(sizeof(Vector3)));
   glEnableVertexAttribArray(1U);

   glBindVertexArray(0U);

   aBuffer.iIndexCount = static_cast<GLsizei>(arrIndices.size());

   return true;
}

/**
 \brief Verarbeitet die kontinuierliche Tastatursteuerung fuer Kamera und Licht.
 \details Pfeiltasten rotieren die Kamera um das Mesh. Plus und Minus veraendern den Kameraabstand. Home beziehungsweise Pos1 und End rotieren das Licht horizontal, PageUp und PageDown verschieben es vertikal.
 \param flDeltaTime Vergangene Zeit seit dem letzten Frame in Sekunden.
 \param aCamera Zu veraendernder Kamerazustand.
 \param aLight Zu veraendernder Lichtzustand.
 \returns Nichts.
 \throw Nichts.
 \note Home und Pos1 sind unter SDL dieselbe Taste. Fuer die Gegenrichtung wird deshalb End verwendet.
 */
void ProcessKeyboard(float const flDeltaTime, CameraState& aCamera, LightState& aLight) noexcept {
   Uint8 const* const pKeyboard = SDL_GetKeyboardState(nullptr);
   constexpr float flCameraSpeed = 1.15F;
   constexpr float flLightSpeed = 1.15F;
   constexpr float flDistanceSpeed = 3.0F;
   constexpr float flCameraElevationLimit = 1.30F;
   constexpr float flLightMinimumElevation = 0.12F;
   constexpr float flLightMaximumElevation = 1.42F;
   constexpr float flMinimumDistance = 4.0F;
   constexpr float flMaximumDistance = 16.0F;

   if (pKeyboard[SDL_SCANCODE_LEFT] != 0U) {
      aCamera.flAzimuth -= flCameraSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_RIGHT] != 0U) {
      aCamera.flAzimuth += flCameraSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_UP] != 0U) {
      aCamera.flElevation += flCameraSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_DOWN] != 0U) {
      aCamera.flElevation -= flCameraSpeed * flDeltaTime;
   }

   bool const fPlusPressed = (pKeyboard[SDL_SCANCODE_EQUALS] != 0U)
      || (pKeyboard[SDL_SCANCODE_KP_PLUS] != 0U);
   bool const fMinusPressed = (pKeyboard[SDL_SCANCODE_MINUS] != 0U)
      || (pKeyboard[SDL_SCANCODE_KP_MINUS] != 0U);

   if (fPlusPressed) {
      aCamera.flRadius += flDistanceSpeed * flDeltaTime;
   }

   if (fMinusPressed) {
      aCamera.flRadius -= flDistanceSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_HOME] != 0U) {
      aLight.flAzimuth -= flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_END] != 0U) {
      aLight.flAzimuth += flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_PAGEUP] != 0U) {
      aLight.flElevation += flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_PAGEDOWN] != 0U) {
      aLight.flElevation -= flLightSpeed * flDeltaTime;
   }

   aCamera.flElevation = std::clamp(aCamera.flElevation, -flCameraElevationLimit, flCameraElevationLimit);
   aCamera.flRadius = std::clamp(aCamera.flRadius, flMinimumDistance, flMaximumDistance);
   aLight.flElevation = std::clamp(aLight.flElevation, flLightMinimumElevation, flLightMaximumElevation);
}

/**
 \brief Setzt die gemeinsamen Matrix Uniforms eines Shaderprogramms.
 \details Modell, Blick und Projektionsmatrix werden in einem Schritt an GLSL uebergeben.
 \param uProgram Zielprogramm.
 \param aModel Modellmatrix.
 \param aView Blickmatrix.
 \param aProjection Projektionsmatrix.
 \returns Nichts.
 \throw Nichts.
 */
void SetMatrixUniforms(GLuint const uProgram, Matrix4 const& aModel, Matrix4 const& aView,
   Matrix4 const& aProjection) noexcept {
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uModel"), 1, GL_FALSE, aModel.arrValues.data());
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uView"), 1, GL_FALSE, aView.arrValues.data());
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uProjection"), 1, GL_FALSE, aProjection.arrValues.data());
}

/**
 \brief Zeichnet ein bereits gebundenes Mesh ueber seinen Index Buffer.
 \details Die Funktion kapselt die fuer Logo und Boden identische Draw Operation.
 \param aBuffer Zu zeichnender Mesh Buffer.
 \returns Nichts.
 \throw Nichts.
 */
void DrawMesh(MeshBuffer const& aBuffer) noexcept {
   glBindVertexArray(aBuffer.aVertexArray.Get());
   glDrawElements(GL_TRIANGLES, aBuffer.iIndexCount, GL_UNSIGNED_INT, nullptr);
   glBindVertexArray(0U);
}

} // namespace

struct LogoScene::Implementation {
   MeshBuffer aLogoBuffer{};
   MeshBuffer aGroundBuffer{};
   Matrix4 aModel{};
   float flGroundY{};
   UniqueGlProgram aLightingProgram;
   UniqueGlProgram aShadowProgram;
   CameraState aCamera{
      .flAzimuth = 0.0F,
      .flElevation = 0.08F,
      .flRadius = 8.5F
   };
   LightState aLight{
      .flAzimuth = -0.65F,
      .flElevation = 0.72F,
      .flRadius = 7.0F
   };
   bool fInitialized{ false };
};

LogoScene::LogoScene() noexcept
   : pImplementation{ std::make_unique<Implementation>() } {
}

LogoScene::~LogoScene() noexcept {
   Shutdown();
}

void LogoScene::Shutdown() noexcept {
   if (!pImplementation) {
      return;
      }

   pImplementation->aShadowProgram.Reset();
   pImplementation->aLightingProgram.Reset();
   pImplementation->aGroundBuffer = {};
   pImplementation->aLogoBuffer = {};
   pImplementation->fInitialized = false;
   }

LogoScene::LogoScene(LogoScene&&) noexcept = default;
LogoScene& LogoScene::operator=(LogoScene&&) noexcept = default;

bool LogoScene::Initialize() noexcept {
   if (!pImplementation) {
      pImplementation = std::make_unique<Implementation>();
   }

   std::filesystem::path const aMeshPath =
      ResourcePath("graphics/adecc_official_logo_planar_cutout_3d_opengl.obj");
   MeshData aMesh{};

   if (aMeshPath.empty()) {
      ShowFatalError("Die Datei graphics/adecc_official_logo_planar_cutout_3d_opengl.obj wurde weder beim Programm noch im Projektverzeichnis gefunden.");
      return false;
   }

   if (!LoadObjMesh(aMeshPath, aMesh)) {
      ShowFatalError(std::format("Das OBJ Mesh '{}' konnte nicht geladen werden.", aMeshPath.string()));
      return false;
   }

   Vector3 const aMeshCenter{
      (aMesh.aMinimum.flX + aMesh.aMaximum.flX) * 0.5F,
      (aMesh.aMinimum.flY + aMesh.aMaximum.flY) * 0.5F,
      (aMesh.aMinimum.flZ + aMesh.aMaximum.flZ) * 0.5F
   };

   pImplementation->aModel = TranslationMatrix(Multiply(aMeshCenter, -1.0F));
   pImplementation->flGroundY = aMesh.aMinimum.flY - aMeshCenter.flY - 0.25F;

   if (!CreateMeshBuffer(aMesh, pImplementation->aLogoBuffer)
      || !CreateGroundBuffer(pImplementation->flGroundY, pImplementation->aGroundBuffer)) {
      ShowFatalError("Die OpenGL Buffer fuer Logo oder Boden konnten nicht erzeugt werden.");
      pImplementation->aGroundBuffer = {};
      pImplementation->aLogoBuffer = {};
      return false;
   }

   pImplementation->aLightingProgram = CreateLightingProgram();
   pImplementation->aShadowProgram = CreateShadowProgram();

   if (!pImplementation->aLightingProgram
      || !pImplementation->aShadowProgram) {
      ShowFatalError("Das Beleuchtungs oder Schatten Shaderprogramm konnte nicht erzeugt werden. Details stehen im SDL Log.");
      return false;
   }

   pImplementation->fInitialized = true;
   return true;
}

void LogoScene::ProcessInput(float const flDeltaTime) noexcept {
   if (!pImplementation || !pImplementation->fInitialized) {
      return;
   }

   ProcessKeyboard(flDeltaTime, pImplementation->aCamera, pImplementation->aLight);
}

void LogoScene::ProcessEvent(SDL_Event const& aEvent) noexcept {
   if (!pImplementation || !pImplementation->fInitialized || (aEvent.type != SDL_MOUSEWHEEL)) {
      return;
   }

   constexpr float flWheelStep = 0.55F;
   constexpr float flMinimumDistance = 4.0F;
   constexpr float flMaximumDistance = 16.0F;
   float flWheelY = static_cast<float>(aEvent.wheel.y);

#if SDL_VERSION_ATLEAST(2, 0, 4)
   if (aEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
      flWheelY = -flWheelY;
   }
#endif

   pImplementation->aCamera.flRadius -= flWheelY * flWheelStep;
   pImplementation->aCamera.flRadius = std::clamp(pImplementation->aCamera.flRadius,
      flMinimumDistance, flMaximumDistance);
}

void LogoScene::Render(int const iDrawableWidth, int const iDrawableHeight) noexcept {
   if (!pImplementation || !pImplementation->fInitialized) {
      return;
   }

   int const iHeight = (std::max)(iDrawableHeight, 1);
   int const iWidth = (std::max)(iDrawableWidth, 1);
   Vector3 const aTarget{ 0.0F, 0.0F, 0.0F };
   Vector3 const aCameraPosition = OrbitPosition(
      pImplementation->aCamera.flAzimuth,
      pImplementation->aCamera.flElevation,
      pImplementation->aCamera.flRadius);
   Vector3 const aLightPosition = OrbitPosition(
      pImplementation->aLight.flAzimuth,
      pImplementation->aLight.flElevation,
      pImplementation->aLight.flRadius);
   Vector3 const aSpotDirection = Normalize(Subtract(aTarget, aLightPosition));
   float const flSpotInnerCutoff = std::cos(38.0F * std::numbers::pi_v<float> / 180.0F);
   float const flSpotOuterCutoff = std::cos(50.0F * std::numbers::pi_v<float> / 180.0F);
   Matrix4 const aView = LookAtMatrix(aCameraPosition, aTarget, Vector3{ 0.0F, 1.0F, 0.0F });
   float const flAspectRatio = static_cast<float>(iWidth) / static_cast<float>(iHeight);
   Matrix4 const aProjection = PerspectiveMatrix(
      45.0F * std::numbers::pi_v<float> / 180.0F, flAspectRatio, 0.1F, 100.0F);

   glUseProgram(pImplementation->aLightingProgram.Get());
   SetMatrixUniforms(pImplementation->aLightingProgram.Get(), IdentityMatrix(), aView, aProjection);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uLightPosition"),
      aLightPosition.flX, aLightPosition.flY, aLightPosition.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotDirection"),
      aSpotDirection.flX, aSpotDirection.flY, aSpotDirection.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uCameraPosition"),
      aCameraPosition.flX, aCameraPosition.flY, aCameraPosition.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uBaseColor"),
      0.27F, 0.29F, 0.32F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpecularStrength"), 0.16F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uShininess"), 24.0F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uMetallic"), 0.0F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uAmbientStrength"), 0.13F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotInnerCutoff"), flSpotInnerCutoff);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotOuterCutoff"), flSpotOuterCutoff);
   DrawMesh(pImplementation->aGroundBuffer);

   Matrix4 const aShadowProjection =
      ShadowMatrix(aLightPosition, pImplementation->flGroundY + 0.004F);
   Matrix4 const aShadowModel = Multiply(aShadowProjection, pImplementation->aModel);

   glUseProgram(pImplementation->aShadowProgram.Get());
   SetMatrixUniforms(pImplementation->aShadowProgram.Get(), aShadowModel, aView, aProjection);
   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);
   DrawMesh(pImplementation->aLogoBuffer);
   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);

   glUseProgram(pImplementation->aLightingProgram.Get());
   SetMatrixUniforms(pImplementation->aLightingProgram.Get(), pImplementation->aModel, aView, aProjection);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uLightPosition"),
      aLightPosition.flX, aLightPosition.flY, aLightPosition.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotDirection"),
      aSpotDirection.flX, aSpotDirection.flY, aSpotDirection.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uCameraPosition"),
      aCameraPosition.flX, aCameraPosition.flY, aCameraPosition.flZ);
   glUniform3f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uBaseColor"),
      0.0F, 0.321569F, 0.611765F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpecularStrength"), 1.90F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uShininess"), 112.0F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uMetallic"), 0.82F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uAmbientStrength"), 0.075F);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotInnerCutoff"), flSpotInnerCutoff);
   glUniform1f(glGetUniformLocation(pImplementation->aLightingProgram.Get(), "uSpotOuterCutoff"), flSpotOuterCutoff);
   DrawMesh(pImplementation->aLogoBuffer);
}
