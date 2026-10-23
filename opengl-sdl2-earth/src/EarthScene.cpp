#include "EarthScene.h"
#include "OpenGLResource.h"

#include <GL/glew.h>

#if __has_include(<soil2/SOIL2.h>)
#include <soil2/SOIL2.h>
#elif __has_include(<SOIL2.h>)
#include <SOIL2.h>
#else
#error "SOIL2 header not found"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Vector3 {
   float flX{};
   float flY{};
   float flZ{};
};

struct Matrix4 {
   std::array<float, 16> arrValues{};
};

struct EarthVertex {
   Vector3 aPosition{};
   Vector3 aNormal{};
   float flU{};
   float flV{};
};

struct MeshBuffer {
   UniqueGlVertexArray aVertexArray;
   UniqueGlBuffer aVertexBuffer;
   UniqueGlBuffer aElementBuffer;
   GLsizei iIndexCount{};
};

struct LightState {
   float flAzimuth{-0.65F};
   float flElevation{0.45F};
   float flRadius{6.0F};
};

void DebugPrint(std::string_view const svMessage) noexcept {
   std::string const strMessage{ svMessage };
   SDL_Log("%s", strMessage.c_str());
}

Vector3 Subtract(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return Vector3{
      aLeft.flX - aRight.flX,
      aLeft.flY - aRight.flY,
      aLeft.flZ - aRight.flZ
   };
}

Vector3 Multiply(Vector3 const& aVector, float const flFactor) noexcept {
   return Vector3{
      aVector.flX * flFactor,
      aVector.flY * flFactor,
      aVector.flZ * flFactor
   };
}

float Dot(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return aLeft.flX * aRight.flX + aLeft.flY * aRight.flY + aLeft.flZ * aRight.flZ;
}

Vector3 Cross(Vector3 const& aLeft, Vector3 const& aRight) noexcept {
   return Vector3{
      aLeft.flY * aRight.flZ - aLeft.flZ * aRight.flY,
      aLeft.flZ * aRight.flX - aLeft.flX * aRight.flZ,
      aLeft.flX * aRight.flY - aLeft.flY * aRight.flX
   };
}

Vector3 Normalize(Vector3 const& aVector) noexcept {
   float const flLength = std::sqrt(Dot(aVector, aVector));

   if (flLength <= std::numeric_limits<float>::epsilon()) {
      return aVector;
   }

   return Multiply(aVector, 1.0F / flLength);
}

Matrix4 IdentityMatrix() noexcept {
   Matrix4 aMatrix{};

   aMatrix.arrValues[0] = 1.0F;
   aMatrix.arrValues[5] = 1.0F;
   aMatrix.arrValues[10] = 1.0F;
   aMatrix.arrValues[15] = 1.0F;

   return aMatrix;
}

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

Matrix4 RotationYMatrix(float const flAngle) noexcept {
   Matrix4 aMatrix = IdentityMatrix();
   float const flCosine = std::cos(flAngle);
   float const flSine = std::sin(flAngle);

   aMatrix.arrValues[0] = flCosine;
   aMatrix.arrValues[2] = -flSine;
   aMatrix.arrValues[8] = flSine;
   aMatrix.arrValues[10] = flCosine;

   return aMatrix;
}

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

Vector3 OrbitPosition(float const flAzimuth, float const flElevation, float const flRadius) noexcept {
   float const flHorizontalRadius = flRadius * std::cos(flElevation);

   return Vector3{
      flHorizontalRadius * std::sin(flAzimuth),
      flRadius * std::sin(flElevation),
      flHorizontalRadius * std::cos(flAzimuth)
   };
}

std::filesystem::path ExecutablePath(std::string_view const svRelativePath) {
   char* const szBasePath = SDL_GetBasePath();

   if (szBasePath == nullptr) {
      return std::filesystem::current_path() / svRelativePath;
   }

   std::filesystem::path const aBasePath{ szBasePath };
   SDL_free(szBasePath);

   return aBasePath / svRelativePath;
}

std::filesystem::path ResourcePath(std::string_view const svRelativePath) noexcept {
   std::error_code aErrorCode;
   std::array<std::filesystem::path, 3> const arrCandidates{
      ExecutablePath(svRelativePath),
      std::filesystem::current_path(aErrorCode) / svRelativePath,
      std::filesystem::path{ __FILE__ }.parent_path().parent_path() / svRelativePath
   };

   for (std::filesystem::path const& aCandidate : arrCandidates) {
      aErrorCode.clear();

      if (std::filesystem::is_regular_file(aCandidate, aErrorCode)) {
         DebugPrint(std::format("Ressource gefunden: {}", aCandidate.string()));
         return aCandidate;
      }
   }

   DebugPrint(std::format("Earth Ressource nicht gefunden: {}", svRelativePath));
   return {};
}

UniqueGlShader CompileShader(GLenum const uShaderType, char const* const szSource) noexcept {
   UniqueGlShader aShader{ glCreateShader(uShaderType) };

   if (!aShader) {
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
   DebugPrint(std::format("Earth Shader compilation failed: {}", arrLog.data()));

   return {};
}

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
      return {};
   }

   glAttachShader(aProgram.Get(), aVertexShader.Get());
   glAttachShader(aProgram.Get(), aFragmentShader.Get());
   glLinkProgram(aProgram.Get());

   GLint iLinked = GL_FALSE;
   glGetProgramiv(aProgram.Get(), GL_LINK_STATUS, &iLinked);

   if (iLinked == GL_TRUE) {
      return aProgram;
   }

   std::array<char, 4096> arrLog{};
   glGetProgramInfoLog(aProgram.Get(), static_cast<GLsizei>(arrLog.size()), nullptr, arrLog.data());
   DebugPrint(std::format("Earth Program linking failed: {}", arrLog.data()));

   return {};
}

UniqueGlProgram CreateEarthProgram() noexcept {
   constexpr char const* szVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
   vec4 aWorldPosition = uModel * vec4(aPosition, 1.0);
   mat3 aNormalMatrix = transpose(inverse(mat3(uModel)));

   vWorldPosition = aWorldPosition.xyz;
   vNormal = normalize(aNormalMatrix * aNormal);
   vTexCoord = aTexCoord;
   gl_Position = uProjection * uView * aWorldPosition;
}
)GLSL";

   constexpr char const* szFragmentShader = R"GLSL(
#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec3 uLightPosition;
uniform float uNightMode;

out vec4 aColor;

void main() {
   vec3 aNormal = normalize(vNormal);
   vec3 aLightDirection = normalize(uLightPosition - vWorldPosition);
   float flDiffuse = max(dot(aNormal, aLightDirection), 0.0);
   vec3 aSurface = texture(uTexture, vTexCoord).rgb;
   float flDayLight = 0.12 + flDiffuse * 0.88;
   float flNightLight = 0.32 + flDiffuse * 0.68;
   float flLight = mix(flDayLight, flNightLight, uNightMode);

   aColor = vec4(aSurface * flLight, 1.0);
}
)GLSL";

   return CreateProgram(szVertexShader, szFragmentShader);
}

UniqueGlProgram CreateBackgroundProgram() noexcept {
   constexpr char const* szVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;

void main() {
   gl_Position = uProjection * uView * vec4(aPosition, 1.0);
   vTexCoord = aTexCoord;
}
)GLSL";

   constexpr char const* szFragmentShader = R"GLSL(
#version 330 core

in vec2 vTexCoord;
uniform sampler2D uTexture;
out vec4 aColor;

void main() {
   aColor = texture(uTexture, vTexCoord);
}
)GLSL";

   return CreateProgram(szVertexShader, szFragmentShader);
}

bool CreateSphereMesh(float const flRadius, int const iSlices, int const iStacks,
   bool const fFlipHorizontalTexture, bool const fFlipVerticalTexture, MeshBuffer& aBuffer) noexcept {
   if ((iSlices < 3) || (iStacks < 2)) {
      return false;
   }

   std::vector<EarthVertex> vecVertices;
   std::vector<GLuint> vecIndices;
   std::size_t const uColumnCount = static_cast<std::size_t>(iSlices + 1);

   vecVertices.reserve(static_cast<std::size_t>((iSlices + 1) * (iStacks + 1)));
   vecIndices.reserve(static_cast<std::size_t>(iSlices * iStacks * 6));

   for (int iStack = 0; iStack <= iStacks; ++iStack) {
      float const flV = static_cast<float>(iStack) / static_cast<float>(iStacks);
      float const flLatitude = std::numbers::pi_v<float> * flV;
      float const flSinLatitude = std::sin(flLatitude);
      float const flCosLatitude = std::cos(flLatitude);

      for (int iSlice = 0; iSlice <= iSlices; ++iSlice) {
         float const flU = static_cast<float>(iSlice) / static_cast<float>(iSlices);
         float const flLongitude = 2.0F * std::numbers::pi_v<float> * flU;
         Vector3 const aNormal{
            std::cos(flLongitude) * flSinLatitude,
            flCosLatitude,
            std::sin(flLongitude) * flSinLatitude
         };

         vecVertices.push_back(EarthVertex{
            .aPosition = Multiply(aNormal, flRadius),
            .aNormal = aNormal,
            .flU = fFlipHorizontalTexture ? 1.0F - flU : flU,
            .flV = fFlipVerticalTexture ? 1.0F - flV : flV
         });
      }
   }

   for (int iStack = 0; iStack < iStacks; ++iStack) {
      for (int iSlice = 0; iSlice < iSlices; ++iSlice) {
         GLuint const uFirst = static_cast<GLuint>(static_cast<std::size_t>(iStack) * uColumnCount
            + static_cast<std::size_t>(iSlice));
         GLuint const uSecond = static_cast<GLuint>(static_cast<std::size_t>(iStack + 1) * uColumnCount
            + static_cast<std::size_t>(iSlice));

         vecIndices.push_back(uFirst);
         vecIndices.push_back(uFirst + 1U);
         vecIndices.push_back(uSecond);

         vecIndices.push_back(uSecond);
         vecIndices.push_back(uFirst + 1U);
         vecIndices.push_back(uSecond + 1U);
      }
   }

   glGenVertexArrays(1, aBuffer.aVertexArray.Put());
   glGenBuffers(1, aBuffer.aVertexBuffer.Put());
   glGenBuffers(1, aBuffer.aElementBuffer.Put());

   if (!aBuffer.aVertexArray || !aBuffer.aVertexBuffer || !aBuffer.aElementBuffer) {
      return false;
   }

   glBindVertexArray(aBuffer.aVertexArray.Get());
   glBindBuffer(GL_ARRAY_BUFFER, aBuffer.aVertexBuffer.Get());
   glBufferData(GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vecVertices.size() * sizeof(EarthVertex)),
      vecVertices.data(), GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aBuffer.aElementBuffer.Get());
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vecIndices.size() * sizeof(GLuint)),
      vecIndices.data(), GL_STATIC_DRAW);

   glVertexAttribPointer(0U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(EarthVertex)), nullptr);
   glEnableVertexAttribArray(0U);

   glVertexAttribPointer(1U, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(EarthVertex)),
      reinterpret_cast<void const*>(offsetof(EarthVertex, aNormal)));
   glEnableVertexAttribArray(1U);

   glVertexAttribPointer(2U, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(EarthVertex)),
      reinterpret_cast<void const*>(offsetof(EarthVertex, flU)));
   glEnableVertexAttribArray(2U);

   glBindVertexArray(0U);
   aBuffer.iIndexCount = static_cast<GLsizei>(vecIndices.size());

   return true;
}

UniqueGlTexture LoadTexture(std::filesystem::path const& aPath) noexcept {
   GLuint const uTexture = SOIL_load_OGL_texture(aPath.string().c_str(),
      SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

   if (uTexture == 0U) {
      DebugPrint(std::format("SOIL2 konnte '{}' nicht laden: {}", aPath.string(), SOIL_last_result()));
      return {};
   }

   UniqueGlTexture aTexture{ uTexture };
   glBindTexture(GL_TEXTURE_2D, aTexture.Get());
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   GLint iWidth = 0;
   GLint iHeight = 0;
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iWidth);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &iHeight);
   glBindTexture(GL_TEXTURE_2D, 0U);

   DebugPrint(std::format("Textur geladen: {} ({} x {})", aPath.string(), iWidth, iHeight));
   return aTexture;
}

void DrawMesh(MeshBuffer const& aBuffer) noexcept {
   glBindVertexArray(aBuffer.aVertexArray.Get());
   glDrawElements(GL_TRIANGLES, aBuffer.iIndexCount, GL_UNSIGNED_INT, nullptr);
   glBindVertexArray(0U);
}

void SetMatrixUniforms(GLuint const uProgram, Matrix4 const& aModel, Matrix4 const& aView,
   Matrix4 const& aProjection) noexcept {
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uModel"), 1, GL_FALSE, aModel.arrValues.data());
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uView"), 1, GL_FALSE, aView.arrValues.data());
   glUniformMatrix4fv(glGetUniformLocation(uProgram, "uProjection"), 1, GL_FALSE, aProjection.arrValues.data());
}

} // namespace

struct EarthScene::Implementation {
   MeshBuffer aEarthBuffer;
   MeshBuffer aBackgroundBuffer;
   UniqueGlProgram aEarthProgram;
   UniqueGlProgram aBackgroundProgram;
   UniqueGlTexture aDayTexture;
   UniqueGlTexture aNightTexture;
   UniqueGlTexture aBackgroundTexture;
   LightState aLight;
   EarthTextureMode aMode{ EarthTextureMode::Day };
   float flCameraDistance{6.0F};
   float flCameraOffsetX{};
   float flEarthRotation{};
   bool fInitialized{};
};

EarthScene::EarthScene() noexcept
   : pImplementation{ std::make_unique<Implementation>() } {
}

EarthScene::~EarthScene() noexcept = default;
EarthScene::EarthScene(EarthScene&&) noexcept = default;
EarthScene& EarthScene::operator=(EarthScene&&) noexcept = default;

bool EarthScene::Initialize() noexcept {
   Implementation& aImplementation = *pImplementation;

   if (aImplementation.fInitialized) {
      return true;
   }

   std::filesystem::path const aDayPath = ResourcePath("graphics/2k_earth_daymap.jpg");
   std::filesystem::path const aNightPath = ResourcePath("graphics/2k_earth_nightmap.jpg");
   std::filesystem::path const aBackgroundPath = ResourcePath("graphics/2k_stars_milky_way.jpg");

   if (aDayPath.empty() || aNightPath.empty() || aBackgroundPath.empty()) {
      DebugPrint("Mindestens eine Earth Textur wurde nicht gefunden.");
      return false;
   }

   aImplementation.aEarthProgram = CreateEarthProgram();
   aImplementation.aBackgroundProgram = CreateBackgroundProgram();

   if (!aImplementation.aEarthProgram || !aImplementation.aBackgroundProgram) {
      DebugPrint("Earth Shaderprogramme konnten nicht erzeugt werden.");
      Shutdown();
      return false;
   }

   if (!CreateSphereMesh(2.0F, 96, 64, true, false, aImplementation.aEarthBuffer)
      || !CreateSphereMesh(40.0F, 96, 64, false, false, aImplementation.aBackgroundBuffer)) {
      DebugPrint("Earth Kugelmeshs konnten nicht erzeugt werden.");
      Shutdown();
      return false;
   }

   aImplementation.aDayTexture = LoadTexture(aDayPath);
   aImplementation.aNightTexture = LoadTexture(aNightPath);
   aImplementation.aBackgroundTexture = LoadTexture(aBackgroundPath);

   if (!aImplementation.aDayTexture || !aImplementation.aNightTexture || !aImplementation.aBackgroundTexture) {
      DebugPrint("Earth Texturen konnten nicht vollstaendig geladen werden.");
      Shutdown();
      return false;
   }

   aImplementation.fInitialized = true;
   return true;
}

void EarthScene::SetMode(EarthTextureMode const aMode) noexcept {
   pImplementation->aMode = aMode;
}

void EarthScene::ProcessInput(float const flDeltaTime) noexcept {
   Implementation& aImplementation = *pImplementation;
   Uint8 const* const pKeyboard = SDL_GetKeyboardState(nullptr);
   constexpr float flLightSpeed = 1.10F;
   constexpr float flDistanceSpeed = 3.0F;
   constexpr float flCameraMoveSpeed = 2.5F;
   constexpr float flMinimumElevation = -1.35F;
   constexpr float flMaximumElevation = 1.35F;
   constexpr float flMinimumDistance = 3.0F;
   constexpr float flMaximumDistance = 15.0F;
   constexpr float flMinimumCameraOffsetX = -8.0F;
   constexpr float flMaximumCameraOffsetX = 8.0F;

   if (pKeyboard[SDL_SCANCODE_LEFT] != 0U) {
      aImplementation.aLight.flAzimuth -= flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_RIGHT] != 0U) {
      aImplementation.aLight.flAzimuth += flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_UP] != 0U) {
      aImplementation.aLight.flElevation += flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_DOWN] != 0U) {
      aImplementation.aLight.flElevation -= flLightSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_HOME] != 0U) {
      aImplementation.flCameraOffsetX -= flCameraMoveSpeed * flDeltaTime;
   }

   if (pKeyboard[SDL_SCANCODE_END] != 0U) {
      aImplementation.flCameraOffsetX += flCameraMoveSpeed * flDeltaTime;
   }

   bool const fPlusPressed = (pKeyboard[SDL_SCANCODE_EQUALS] != 0U)
      || (pKeyboard[SDL_SCANCODE_KP_PLUS] != 0U);
   bool const fMinusPressed = (pKeyboard[SDL_SCANCODE_MINUS] != 0U)
      || (pKeyboard[SDL_SCANCODE_KP_MINUS] != 0U);

   if (fPlusPressed) {
      aImplementation.flCameraDistance += flDistanceSpeed * flDeltaTime;
   }

   if (fMinusPressed) {
      aImplementation.flCameraDistance -= flDistanceSpeed * flDeltaTime;
   }

   aImplementation.aLight.flElevation = std::clamp(aImplementation.aLight.flElevation,
      flMinimumElevation, flMaximumElevation);
   aImplementation.flCameraDistance = std::clamp(aImplementation.flCameraDistance,
      flMinimumDistance, flMaximumDistance);
   aImplementation.flCameraOffsetX = std::clamp(aImplementation.flCameraOffsetX,
      flMinimumCameraOffsetX, flMaximumCameraOffsetX);
}

void EarthScene::ProcessEvent(SDL_Event const& aEvent) noexcept {
   if (aEvent.type != SDL_MOUSEWHEEL) {
      return;
   }

   Implementation& aImplementation = *pImplementation;
   constexpr float flWheelStep = 0.55F;
   constexpr float flMinimumDistance = 3.0F;
   constexpr float flMaximumDistance = 15.0F;
   float flWheelY = static_cast<float>(aEvent.wheel.y);

#if SDL_VERSION_ATLEAST(2, 0, 4)
   if (aEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
      flWheelY = -flWheelY;
   }
#endif

   aImplementation.flCameraDistance -= flWheelY * flWheelStep;
   aImplementation.flCameraDistance = std::clamp(aImplementation.flCameraDistance,
      flMinimumDistance, flMaximumDistance);
}

void EarthScene::Update(float const flDeltaTime) noexcept {
   Implementation& aImplementation = *pImplementation;
   constexpr float flRotationSpeed = 0.10F;

   aImplementation.flEarthRotation += flRotationSpeed * flDeltaTime;

   if (aImplementation.flEarthRotation > 2.0F * std::numbers::pi_v<float>) {
      aImplementation.flEarthRotation -= 2.0F * std::numbers::pi_v<float>;
   }
}

void EarthScene::Render(int const iDrawableWidth, int const iDrawableHeight) noexcept {
   Implementation& aImplementation = *pImplementation;

   if (!aImplementation.fInitialized || (iDrawableWidth <= 0) || (iDrawableHeight <= 0)) {
      return;
   }

   float const flAspectRatio = static_cast<float>(iDrawableWidth) / static_cast<float>(iDrawableHeight);
   Matrix4 const aProjection = PerspectiveMatrix(45.0F * std::numbers::pi_v<float> / 180.0F,
      flAspectRatio, 0.1F, 100.0F);
   Vector3 const aCameraPosition{
      aImplementation.flCameraOffsetX,
      0.0F,
      aImplementation.flCameraDistance
   };
   Vector3 const aCameraTarget{ aImplementation.flCameraOffsetX, 0.0F, 0.0F };
   Matrix4 const aView = LookAtMatrix(aCameraPosition, aCameraTarget, Vector3{ 0.0F, 1.0F, 0.0F });
   Vector3 const aLightPosition = OrbitPosition(aImplementation.aLight.flAzimuth,
      aImplementation.aLight.flElevation, aImplementation.aLight.flRadius);

   glDepthMask(GL_FALSE);
   glDisable(GL_CULL_FACE);
   glUseProgram(aImplementation.aBackgroundProgram.Get());
   glUniformMatrix4fv(glGetUniformLocation(aImplementation.aBackgroundProgram.Get(), "uView"),
      1, GL_FALSE, aView.arrValues.data());
   glUniformMatrix4fv(glGetUniformLocation(aImplementation.aBackgroundProgram.Get(), "uProjection"),
      1, GL_FALSE, aProjection.arrValues.data());
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, aImplementation.aBackgroundTexture.Get());
   glUniform1i(glGetUniformLocation(aImplementation.aBackgroundProgram.Get(), "uTexture"), 0);
   DrawMesh(aImplementation.aBackgroundBuffer);
   glBindTexture(GL_TEXTURE_2D, 0U);
   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);

   Matrix4 const aModel = RotationYMatrix(aImplementation.flEarthRotation);
   GLuint const uEarthProgram = aImplementation.aEarthProgram.Get();
   UniqueGlTexture const& aEarthTexture = aImplementation.aMode == EarthTextureMode::Day
      ? aImplementation.aDayTexture
      : aImplementation.aNightTexture;

   glUseProgram(uEarthProgram);
   SetMatrixUniforms(uEarthProgram, aModel, aView, aProjection);
   glUniform3f(glGetUniformLocation(uEarthProgram, "uLightPosition"),
      aLightPosition.flX, aLightPosition.flY, aLightPosition.flZ);
   glUniform1f(glGetUniformLocation(uEarthProgram, "uNightMode"),
      aImplementation.aMode == EarthTextureMode::Night ? 1.0F : 0.0F);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, aEarthTexture.Get());
   glUniform1i(glGetUniformLocation(uEarthProgram, "uTexture"), 0);
   DrawMesh(aImplementation.aEarthBuffer);
   glBindTexture(GL_TEXTURE_2D, 0U);
}

void EarthScene::Shutdown() noexcept {
   Implementation& aImplementation = *pImplementation;

   aImplementation.aBackgroundTexture.Reset();
   aImplementation.aNightTexture.Reset();
   aImplementation.aDayTexture.Reset();
   aImplementation.aBackgroundProgram.Reset();
   aImplementation.aEarthProgram.Reset();
   aImplementation.aBackgroundBuffer = {};
   aImplementation.aEarthBuffer = {};
   aImplementation.fInitialized = false;
}
