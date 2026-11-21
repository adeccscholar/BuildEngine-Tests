// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

#include "PhysicsArena.h"

#include <box2d/math_functions.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using adecc::tests::Actor;
using adecc::tests::EActorKind;
using adecc::tests::Impact;
using adecc::tests::PhysicsArena;

constexpr int iViewportWidth = 1600;
constexpr int iViewportHeight = 900;
constexpr float flPixelsPerMetre = 40.0f;
constexpr float flPi = std::numbers::pi_v<float>;

constexpr std::array<SDL_FColor, 6> arrNeon {
   SDL_FColor { 0.08f, 0.81f, 0.94f, 1.0f },
   SDL_FColor { 0.94f, 0.20f, 0.65f, 1.0f },
   SDL_FColor { 1.00f, 0.71f, 0.24f, 1.0f },
   SDL_FColor { 0.45f, 0.95f, 0.55f, 1.0f },
   SDL_FColor { 0.63f, 0.50f, 1.00f, 1.0f },
   SDL_FColor { 0.18f, 0.75f, 1.00f, 1.0f }
   };

struct Particle {
   SDL_FPoint thePosition {};
   SDL_FPoint theVelocity {};
   SDL_FColor theColor {};
   float flLife {};
   };

struct RuntimeView {
   SDL_Window* pWindow {};
   SDL_Renderer* pRenderer {};
   PhysicsArena* pPhysics {};
   std::vector<Particle> vecParticles;
   SDL_FPoint theMouse { 800.0f, 460.0f };
   std::uint32_t uRandomState { 0x9e3779b9U };
   float flTime {};
   bool bPaused {};
   bool bScreenshotRequested {};
   };

/**
\brief Converts native Box2D metres to SDL3 logical pixels.
\details World origin is screen centre and world vertical increases upward.
\param theWorld World position in metres.
\returns Logical pixel coordinates.
\throw Nothing.
*/
SDL_FPoint WorldToScreen(b2Vec2 const theWorld) noexcept {
   return SDL_FPoint {
      iViewportWidth * 0.5f + flPixelsPerMetre * theWorld.x,
      iViewportHeight * 0.5f - flPixelsPerMetre * theWorld.y
      };
   }

/**
\brief Converts SDL3 logical pixels to Box2D world metres.
\details The SDL3 logical presentation mode handles window resizes.
\param theScreen Point in logical pixels.
\returns World-space position.
\throw Nothing.
*/
b2Vec2 ScreenToWorld(SDL_FPoint const theScreen) noexcept {
   return b2Vec2 {
      (theScreen.x - iViewportWidth * 0.5f) / flPixelsPerMetre,
      (iViewportHeight * 0.5f - theScreen.y) / flPixelsPerMetre
      };
   }

/**
\brief Changes the SDL3 renderer draw color.
\details Normalized RGBA colors are used throughout the neon scene.
\param pRenderer Native SDL3 renderer.
\param theColor Desired normalized RGBA color.
\throw Nothing intentionally.
*/
void SetColor(SDL_Renderer* const pRenderer, SDL_FColor const theColor) {
   (void)SDL_SetRenderDrawColorFloat(pRenderer, theColor.r, theColor.g,
                                    theColor.b, theColor.a);
   }

/**
\brief Draws a filled triangle-fan polygon using SDL3 geometry.
\details All polygons are submitted through the native SDL3 renderer.
\param pRenderer Native SDL3 renderer.
\param vecPoints Polygon vertices in logical pixels.
\param theColor Fill color.
\throw Nothing intentionally.
*/
void FillPolygon(SDL_Renderer* const pRenderer,
                 std::vector<SDL_FPoint> const& vecPoints, SDL_FColor const theColor) {
   if(vecPoints.size() < 3) {
      return;
      }
   std::vector<SDL_Vertex> vecVertices;
   std::vector<int> vecIndices;
   vecVertices.reserve(vecPoints.size());
   vecIndices.reserve((vecPoints.size() - 2) * 3);
   for(SDL_FPoint const& thePoint : vecPoints) {
      vecVertices.push_back(SDL_Vertex { .position = thePoint, .color = theColor });
      }
   for(std::size_t uIndex = 1; uIndex + 1 < vecPoints.size(); ++uIndex) {
      vecIndices.push_back(0);
      vecIndices.push_back(static_cast<int>(uIndex));
      vecIndices.push_back(static_cast<int>(uIndex + 1));
      }
   (void)SDL_RenderGeometry(pRenderer, nullptr, vecVertices.data(),
                            static_cast<int>(vecVertices.size()),
                            vecIndices.data(), static_cast<int>(vecIndices.size()));
   }

/**
\brief Draws a filled and outlined circle.
\details A 32-segment polygon approximates a neon-glowing disc.
\param pRenderer Native SDL3 renderer.
\param theCenter Circle centre in logical pixels.
\param flRadius Radius in pixels.
\param theColor Base normalized color.
\param bGlow Whether to draw translucent halo layers.
\throw Nothing intentionally.
*/
void DrawCircle(SDL_Renderer* const pRenderer, SDL_FPoint const theCenter,
                float const flRadius, SDL_FColor const theColor, bool const bGlow) {
   if(bGlow) {
      for(int iHalo = 4; iHalo >= 1; --iHalo) {
         SDL_FColor theHalo = theColor;
         theHalo.a = 0.018f + 0.012f * static_cast<float>(iHalo);
         std::vector<SDL_FPoint> vecHalo;
         vecHalo.reserve(32);
         for(int i = 0; i < 32; ++i) {
            float const flAngle = 2.0f * flPi * static_cast<float>(i) / 32.0f;
            float const flR = flRadius + 2.7f * static_cast<float>(iHalo);
            vecHalo.push_back(SDL_FPoint {
               theCenter.x + std::cos(flAngle) * flR,
               theCenter.y + std::sin(flAngle) * flR
               });
            }
         FillPolygon(pRenderer, vecHalo, theHalo);
         }
      }
   std::vector<SDL_FPoint> vecShape;
   vecShape.reserve(32);
   for(int i = 0; i < 32; ++i) {
      float const flAngle = 2.0f * flPi * static_cast<float>(i) / 32.0f;
      vecShape.push_back(SDL_FPoint {
         theCenter.x + flRadius * std::cos(flAngle),
         theCenter.y + flRadius * std::sin(flAngle)
         });
      }
   SDL_FColor theFill = theColor;
   theFill.a = 0.18f;
   FillPolygon(pRenderer, vecShape, theFill);
   SetColor(pRenderer, theColor);
   for(std::size_t uIndex = 0; uIndex < vecShape.size(); ++uIndex) {
      SDL_FPoint const& theA = vecShape[uIndex];
      SDL_FPoint const& theB = vecShape[(uIndex + 1) % vecShape.size()];
      (void)SDL_RenderLine(pRenderer, theA.x, theA.y, theB.x, theB.y);
      }
   }

/**
\brief Draws an actual Box2D rigid body with its current rotation.
\details Body poses come directly from the Box2D DLL, never from visual interpolation.
\param pRenderer Native SDL3 renderer.
\param theActor Renderable physics entity.
\throw Nothing intentionally.
*/
void DrawActor(SDL_Renderer* const pRenderer, Actor const& theActor) {
   if(!b2Body_IsValid(theActor.theBody)) {
      return;
      }
   b2Vec2 const theCenter = b2ToVec2(b2Body_GetPosition(theActor.theBody));
   SDL_FColor const theColor = arrNeon[static_cast<std::size_t>(
      std::clamp(theActor.iColor, 0, static_cast<int>(arrNeon.size() - 1)))];
   if(theActor.eKind == EActorKind::orb || theActor.eKind == EActorKind::projectile) {
      DrawCircle(pRenderer, WorldToScreen(theCenter), theActor.flRadius * flPixelsPerMetre,
                 theColor, true);
      return;
      }

   b2Rot const theRot = b2Body_GetRotation(theActor.theBody);
   std::array<b2Vec2, 4> const arrCorners {
      b2Vec2 { -theActor.flHalfWidth, -theActor.flHalfHeight },
      b2Vec2 { theActor.flHalfWidth, -theActor.flHalfHeight },
      b2Vec2 { theActor.flHalfWidth, theActor.flHalfHeight },
      b2Vec2 { -theActor.flHalfWidth, theActor.flHalfHeight }
      };
   std::vector<SDL_FPoint> vecPoints;
   vecPoints.reserve(arrCorners.size());
   for(b2Vec2 const& theCorner : arrCorners) {
      vecPoints.push_back(WorldToScreen(
         b2Add(theCenter, b2RotateVector(theRot, theCorner))));
      }
   SDL_FColor theFill = theColor;
   theFill.a = theActor.eKind == EActorKind::platform ? 0.24f : 0.33f;
   FillPolygon(pRenderer, vecPoints, theFill);
   SetColor(pRenderer, theColor);
   for(std::size_t uIndex = 0; uIndex < vecPoints.size(); ++uIndex) {
      SDL_FPoint const& theA = vecPoints[uIndex];
      SDL_FPoint const& theB = vecPoints[(uIndex + 1) % vecPoints.size()];
      (void)SDL_RenderLine(pRenderer, theA.x, theA.y, theB.x, theB.y);
      }
   if(theActor.eKind == EActorKind::crate) {
      SDL_FPoint const theA = vecPoints[0];
      SDL_FPoint const theC = vecPoints[2];
      (void)SDL_RenderLine(pRenderer, theA.x, theA.y, theC.x, theC.y);
      }
   }

/**
\brief Generates a deterministic pseudorandom unit value.
\details A small LCG drives visual particles without changing Box2D physics.
\param theView Application state.
\returns Pseudorandom value in [0, 1].
\throw Nothing.
*/
float Random(RuntimeView& theView) noexcept {
   theView.uRandomState = theView.uRandomState * 1664525U + 1013904223U;
   return static_cast<float>(theView.uRandomState >> 8) / 16777216.0f;
   }

/**
\brief Adds temporary particle sparks at native Box2D collision points.
\details Particle effects do not affect Box2D physics.
\param theView Application state.
\param theImpact Native contact hit record.
\throw Nothing intentionally.
*/
void SpawnImpact(RuntimeView& theView, Impact const& theImpact) {
   int const iCount = std::clamp(static_cast<int>(theImpact.flSpeed * 1.2f), 4, 24);
   SDL_FPoint const thePosition = WorldToScreen(theImpact.thePosition);
   for(int i = 0; i < iCount; ++i) {
      float const flAngle = Random(theView) * 2.0f * flPi;
      float const flSpeed = 35.0f + 180.0f * Random(theView);
      theView.vecParticles.push_back(Particle {
         .thePosition = thePosition,
         .theVelocity = { std::cos(flAngle) * flSpeed, std::sin(flAngle) * flSpeed },
         .theColor = arrNeon[static_cast<std::size_t>(i % 6)],
         .flLife = 0.4f + Random(theView) * 0.55f
         });
      }
   }

/**
\brief Updates the bounded visual particle system.
\details Frames do not allocate physics bodies for decorative effects.
\param theView Application state.
\param flDeltaSeconds Visual frame delta.
\throw Nothing intentionally.
*/
void UpdateParticles(RuntimeView& theView, float const flDeltaSeconds) {
   for(Particle& theParticle : theView.vecParticles) {
      theParticle.thePosition.x += theParticle.theVelocity.x * flDeltaSeconds;
      theParticle.thePosition.y += theParticle.theVelocity.y * flDeltaSeconds;
      theParticle.theVelocity.y += 55.0f * flDeltaSeconds;
      theParticle.flLife -= flDeltaSeconds;
      }
   std::erase_if(theView.vecParticles, [](Particle const& theParticle) -> bool {
      return theParticle.flLife <= 0.0f;
      });
   if(theView.vecParticles.size() > 3000) {
      theView.vecParticles.erase(
         theView.vecParticles.begin(),
         theView.vecParticles.begin() +
            static_cast<std::ptrdiff_t>(theView.vecParticles.size() - 3000));
      }
   }

/**
\brief Renders the animated science-fiction arena.
\details Uses SDL3's accelerated 2D renderer, debug font and real rigid-body poses.
\param theView SDL and Box2D application state.
\throw Nothing intentionally.
*/
void Render(RuntimeView& theView) {
   SDL_Renderer* const pRenderer = theView.pRenderer;
   SetColor(pRenderer, SDL_FColor { 0.012f, 0.018f, 0.058f, 1.0f });
   (void)SDL_RenderClear(pRenderer);
   (void)SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);

   // A gradual dark-blue sky with luminous horizon and a fine engineering grid.
   for(int i = 0; i < 80; ++i) {
      float const flY = static_cast<float>(i) * iViewportHeight / 80.0f;
      SDL_FColor const theBand { 0.014f + i * 0.00015f, 0.018f + i * 0.00022f,
                                  0.055f + i * 0.00065f, 1.0f };
      SetColor(pRenderer, theBand);
      SDL_FRect const theRectangle { 0.0f, flY,
                                     static_cast<float>(iViewportWidth),
                                     iViewportHeight / 80.0f + 1.0f };
      (void)SDL_RenderFillRect(pRenderer, &theRectangle);
      }
   SetColor(pRenderer, SDL_FColor { 0.13f, 0.25f, 0.40f, 0.24f });
   for(int i = 0; i <= 40; ++i) {
      float const flX = static_cast<float>(i) * 40.0f;
      (void)SDL_RenderLine(pRenderer, flX, 0.0f, flX,
                           static_cast<float>(iViewportHeight));
      }
   for(int i = 0; i <= 23; ++i) {
      float const flY = static_cast<float>(i) * 40.0f;
      (void)SDL_RenderLine(pRenderer, 0.0f, flY,
                           static_cast<float>(iViewportWidth), flY);
      }
   SetColor(pRenderer, SDL_FColor { 0.20f, 0.60f, 0.83f, 0.38f });
   (void)SDL_RenderLine(pRenderer, 0.0f, 450.0f, 1600.0f, 450.0f);

   for(Actor const& theActor : theView.pPhysics->Actors()) {
      DrawActor(pRenderer, theActor);
      }
   SDL_FPoint const theCannon = WorldToScreen(b2Vec2 { -17.5f, -7.6f });
   DrawCircle(pRenderer, theCannon, 24.0f, arrNeon[0], true);
   SetColor(pRenderer, arrNeon[0]);
   (void)SDL_RenderLine(pRenderer, theCannon.x, theCannon.y,
                        theView.theMouse.x, theView.theMouse.y);
   DrawCircle(pRenderer, theView.theMouse, 9.0f, arrNeon[2], false);
   for(Particle const& theParticle : theView.vecParticles) {
      SDL_FColor theColor = theParticle.theColor;
      theColor.a = std::clamp(theParticle.flLife * 1.3f, 0.0f, 1.0f);
      SetColor(pRenderer, theColor);
      SDL_FRect const theRect {
         theParticle.thePosition.x - 2.0f, theParticle.thePosition.y - 2.0f,
         4.0f, 4.0f
         };
      (void)SDL_RenderFillRect(pRenderer, &theRect);
      }
   SDL_FRect const thePanel { 28.0f, 26.0f, 640.0f, 125.0f };
   SetColor(pRenderer, SDL_FColor { 0.02f, 0.05f, 0.11f, 0.88f });
   (void)SDL_RenderFillRect(pRenderer, &thePanel);
   SetColor(pRenderer, arrNeon[0]);
   (void)SDL_RenderRect(pRenderer, &thePanel);
   (void)SDL_RenderDebugText(pRenderer, 48.0f, 43.0f,
                              "ADECC SCHOLAR // NEON PHYSICS ARENA");
   SetColor(pRenderer, SDL_FColor { 0.95f, 0.95f, 1.0f, 1.0f });
   std::string const strScore = std::format(
      "SCORE {:07}   HITS {:05}   SHOTS {:03}   BODIES {}",
      theView.pPhysics->Score(), theView.pPhysics->Hits(),
      theView.pPhysics->Shots(), theView.pPhysics->Actors().size());
   (void)SDL_RenderDebugText(pRenderer, 48.0f, 74.0f, strScore.c_str());
   std::string const strStatus = std::format(
      "120 HZ BOX2D  |  SDL3 RENDER  |  {}{}",
      theView.pPhysics->GravityInverted() ? "GRAVITY UP" : "GRAVITY DOWN",
      theView.bPaused ? "  |  PAUSED" : "");
   (void)SDL_RenderDebugText(pRenderer, 48.0f, 102.0f, strStatus.c_str());
   SetColor(pRenderer, SDL_FColor { 0.9f, 0.9f, 1.0f, 1.0f });
   (void)SDL_RenderDebugText(pRenderer, 33.0f, 868.0f,
      "LEFT CLICK FIRE   RIGHT CLICK EXPLOSION   G GRAVITY   SPACE PAUSE   R RESET   P BMP   ESC EXIT");

   if(theView.bScreenshotRequested) {
      // Save the REAL SDL framebuffer before Present. No synthetic image.
      SDL_Surface* const pSurface = SDL_RenderReadPixels(pRenderer, nullptr);
      if(pSurface != nullptr) {
         bool const bSaved = SDL_SaveBMP(pSurface, "neon-physics-arena.bmp");
         SDL_DestroySurface(pSurface);
         if(!bSaved) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Screenshot failed: %s", SDL_GetError());
            }
         }
      theView.bScreenshotRequested = false;
      }
   (void)SDL_RenderPresent(pRenderer);
   }

/**
\brief Handles the native SDL3 input events.
\details Mouse events are transformed into logical render coordinates before
         creating impulses. The physics world is never accessed concurrently.
\param theView Application state.
\param theEvent SDL3 event.
\returns False when the user requests termination.
\throw Nothing intentionally.
*/
bool HandleEvent(RuntimeView& theView, SDL_Event& theEvent) {
   if(theEvent.type == SDL_EVENT_QUIT) {
      return false;
      }
   if(theEvent.type == SDL_EVENT_MOUSE_MOTION ||
      theEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      (void)SDL_ConvertEventToRenderCoordinates(theView.pRenderer, &theEvent);
      if(theEvent.type == SDL_EVENT_MOUSE_MOTION) {
         theView.theMouse = SDL_FPoint { theEvent.motion.x, theEvent.motion.y };
         }
      else {
         theView.theMouse = SDL_FPoint { theEvent.button.x, theEvent.button.y };
         b2Vec2 const theTarget = ScreenToWorld(theView.theMouse);
         if(theEvent.button.button == SDL_BUTTON_LEFT) {
            (void)theView.pPhysics->Launch(theTarget);
            }
         else if(theEvent.button.button == SDL_BUTTON_RIGHT) {
            theView.pPhysics->Pulse(theTarget);
            }
         }
      }
   if(theEvent.type == SDL_EVENT_KEY_DOWN && !theEvent.key.repeat) {
      switch(theEvent.key.scancode) {
         case SDL_SCANCODE_F11:
            (void)SDL_SetWindowFullscreen(theView.pWindow,
               !(SDL_GetWindowFlags(theView.pWindow)&SDL_WINDOW_FULLSCREEN));
            break;
         case SDL_SCANCODE_F1:
            (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
               "About Neon Physics Arena",
               "adecc Scholar | Neon Physics Arena\n"
               "Native C++ / SDL3 / Box2D / Jolt differential tests\n"
               "Physics and collision simulation\n"
               "F1: About    F11: Fullscreen / windowed",theView.pWindow);
            break;
         case SDL_SCANCODE_ESCAPE:
            return false;
         case SDL_SCANCODE_R:
            theView.pPhysics->Reset();
            theView.vecParticles.clear();
            break;
         case SDL_SCANCODE_G:
            theView.pPhysics->ToggleGravity();
            break;
         case SDL_SCANCODE_SPACE:
            theView.bPaused = !theView.bPaused;
            break;
         case SDL_SCANCODE_P:
            theView.bScreenshotRequested = true;
            break;
         default:
            break;
         }
      }
   return true;
   }

/**
\brief Runs the interactive SDL3/Box2D integration showcase.
\details Fixed 120 Hz Box2D stepping is independent of renderer frame rate.
         The native SDL3 renderer draws polygons, particles, input and HUD.
\returns Process exit code.
\throw std::runtime_error If a required SDL3 window or renderer cannot initialize.
*/
int RunApplication() {
   SDL_SetMainReady();
   if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
      throw std::runtime_error(std::format("SDL3 initialization failed: {}", SDL_GetError()));
      }
   struct SdlQuit {
      ~SdlQuit() noexcept { SDL_Quit(); }
      };
   SdlQuit const theSdlOwner;
   SDL_Window* pRawWindow = nullptr;
   SDL_Renderer* pRawRenderer = nullptr;
   if(!SDL_CreateWindowAndRenderer("adecc Scholar // NEON PHYSICS ARENA",
                                  iViewportWidth, iViewportHeight,
                                  SDL_WINDOW_RESIZABLE, &pRawWindow, &pRawRenderer)) {
      throw std::runtime_error(std::format("SDL3 renderer failed: {}", SDL_GetError()));
      }
   std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> theWindow {
      pRawWindow, &SDL_DestroyWindow
      };
   std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> theRenderer {
      pRawRenderer, &SDL_DestroyRenderer
      };
   if(!SDL_SetRenderLogicalPresentation(theRenderer.get(),
                                        iViewportWidth, iViewportHeight,
                                        SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
      throw std::runtime_error("SDL3 logical presentation unavailable");
      }
   (void)SDL_SetRenderVSync(theRenderer.get(), 1);
   PhysicsArena thePhysics;
   RuntimeView theView {
      .pWindow = theWindow.get(), .pRenderer = theRenderer.get(),
      .pPhysics = &thePhysics
      };
   std::uint64_t uLastCounter = SDL_GetPerformanceCounter();
   std::uint64_t const uFrequency = SDL_GetPerformanceFrequency();
   if(uFrequency == 0) {
      throw std::runtime_error("SDL3 high-resolution clock unavailable");
      }
   double flAccumulator = 0.0;
   bool bRunning = true;
   while(bRunning) {
      SDL_Event theEvent {};
      while(SDL_PollEvent(&theEvent)) {
         if(!HandleEvent(theView, theEvent)) {
            bRunning = false;
            }
         }
      std::uint64_t const uNow = SDL_GetPerformanceCounter();
      double const flElapsed = std::clamp(
         static_cast<double>(uNow - uLastCounter) / static_cast<double>(uFrequency),
         0.0, 0.05);
      uLastCounter = uNow;
      if(!theView.bPaused) {
         flAccumulator += flElapsed;
         int iSteps = 0;
         while(flAccumulator >= 1.0 / 120.0 && iSteps < 6) {
            thePhysics.Step(1.0f / 120.0f);
            for(Impact const& theImpact : thePhysics.Impacts()) {
               SpawnImpact(theView, theImpact);
               }
            flAccumulator -= 1.0 / 120.0;
            ++iSteps;
            }
         if(iSteps == 6) {
            flAccumulator = 0.0;
            }
         }
      else {
         flAccumulator = 0.0;
         }
      UpdateParticles(theView, static_cast<float>(flElapsed));
      Render(theView);
      SDL_Delay(1);
      }
   return 0;
   }

} // namespace

/**
\brief Windows subsystem entry point for the SDL3 interactive application.
\details Reports fatal initialization errors to the user via a message box.
\param theInstance Current Windows instance.
\param thePreviousInstance Unused previous instance.
\param szCommandLine Native Windows command line, unused.
\param iShowCommand Window show mode, unused.
\returns Zero on normal termination or an error status.
\throw Nothing beyond the process.
*/
int WINAPI WinMain(HINSTANCE const theInstance, HINSTANCE const thePreviousInstance,
                   LPSTR const szCommandLine, int const iShowCommand) {
   (void)theInstance;
   (void)thePreviousInstance;
   (void)szCommandLine;
   (void)iShowCommand;
   try {
      return RunApplication();
      }
   catch(std::exception const& theException) {
      MessageBoxA(nullptr, theException.what(), "Neon Physics Arena: startup failure",
                  MB_OK | MB_ICONERROR);
      return 2;
      }
   }
