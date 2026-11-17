// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

#include "PhysicsArena.h"

#include <box2d/math_functions.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <print>
#include <stdexcept>
#include <string_view>

namespace {

/**
\brief Reports one machine-readable regression assertion.
\details CTest records both the check outcome and its description.
\param svName Stable machine-readable assertion name.
\param bPass Whether the assertion passed.
\param svDescription Descriptive detail.
\returns The assertion result.
\throw Nothing intentionally.
*/
bool Check(std::string_view const svName, bool const bPass,
           std::string_view const svDescription) {
   std::println("ARENA|CHECK|{}|{}|{}", svName, bPass ? "PASS" : "FAIL", svDescription);
   return bPass;
   }

struct Result {
   std::int64_t iPositionSignature {};
   int iHits {};
   int iScore {};
   int iShots {};
   std::size_t uActors {};
   double flElapsedMilliseconds {};
   };

/**
\brief Runs a real native Box2D scene with fixed scripted inputs.
\details Deterministic Box2D solver steps and repeatable controller impulses
         establish a strong regression fingerprint independent of rendering.
\param iSteps Number of 120 Hz solver iterations.
\returns A signature, collision metrics and duration.
\throw std::runtime_error On world creation failure.
*/
Result Simulate(int const iSteps) {
   adecc::tests::PhysicsArena theArena;
   std::chrono::steady_clock::time_point const theStart =
      std::chrono::steady_clock::now();
   for(int i = 0; i < iSteps; ++i) {
      if(i == 12) {
         (void)theArena.Launch(b2Vec2 { 9.0f, -4.0f });
         }
      if(i == 90) {
         (void)theArena.Launch(b2Vec2 { 12.0f, 1.0f });
         }
      if(i == 160) {
         theArena.Pulse(b2Vec2 { 6.0f, -4.0f });
         }
      if(i == 280) {
         (void)theArena.Launch(b2Vec2 { 15.0f, -2.0f });
         }
      theArena.Step(1.0f / 120.0f);
      }
   std::chrono::steady_clock::time_point const theEnd =
      std::chrono::steady_clock::now();
   Result theResult;
   theResult.iHits = theArena.Hits();
   theResult.iScore = theArena.Score();
   theResult.iShots = theArena.Shots();
   theResult.uActors = theArena.Actors().size();
   theResult.flElapsedMilliseconds =
      std::chrono::duration<double, std::milli>(theEnd - theStart).count();

   // Signature contains quantized body positions and velocities. Relative
   // comparisons of identical solver runs do not rely on host timing.
   for(adecc::tests::Actor const& theActor : theArena.Actors()) {
      if(!theActor.bDynamic || !b2Body_IsValid(theActor.theBody)) {
         continue;
         }
      b2Vec2 const thePosition = b2ToVec2(b2Body_GetPosition(theActor.theBody));
      b2Vec2 const theVelocity = b2Body_GetLinearVelocity(theActor.theBody);
      double const flValue = 1000.0 * (
         static_cast<double>(thePosition.x) * 7.0 +
         static_cast<double>(thePosition.y) * 11.0 +
         static_cast<double>(theVelocity.x) * 3.0 +
         static_cast<double>(theVelocity.y) * 5.0);
      if(!std::isfinite(flValue) || std::abs(flValue) > 2.0e9) {
         throw std::runtime_error("non-finite Box2D body position/velocity");
         }
      theResult.iPositionSignature += static_cast<std::int64_t>(std::llround(flValue));
      }
   return theResult;
   }

} // namespace

/**
\brief Executes a headless integration and repeatability suite.
\details Checks SDL3 runtime, native Box2D world dynamics, contact production,
         deterministic repeated simulation, zero-desktop execution and load.
\param iArgc Number of command-line arguments.
\param aszArgv Arguments; optionally --stress.
\returns Zero when every assertion passes.
\throw Nothing outside this process: exceptions are caught.
*/
int main(int const iArgc, char const* const* const aszArgv) {
   try {
      bool bStress = false;
      for(int i = 1; i < iArgc; ++i) {
         std::string_view const svArgument { aszArgv[i] };
         if(svArgument == "--stress") {
            bStress = true;
            }
         else {
            throw std::invalid_argument(std::format("unsupported option: {}", svArgument));
            }
         }
      bool bPass = true;
      bPass = Check("sdl-version", SDL_VERSIONNUM_MAJOR(SDL_GetVersion()) == 3,
                    "SDL3 managed DLL exposes major version 3") && bPass;
      bool const bSdlInit = SDL_Init(0);
      bPass = Check("sdl-init", bSdlInit,
                    "SDL3 runtime initializes without video or desktop") && bPass;
      if(bSdlInit) {
         SDL_Quit();
         }
      b2Version const theVersion = b2GetVersion();
      bPass = Check("box2d-version", theVersion.major == 3,
                    "Box2D managed DLL exposes major version 3") && bPass;

      // Exercise native Box2D world gravity, impulse and reset semantics
      // independently of the repeated long-running physics scenario.
      adecc::tests::PhysicsArena theControlArena;
      bPass = Check("gravity-default", !theControlArena.GravityInverted(),
                    "native world starts with downward gravity") && bPass;
      theControlArena.ToggleGravity();
      bPass = Check("gravity-invert", theControlArena.GravityInverted(),
                    "native world gravity can be reversed") && bPass;
      theControlArena.ToggleGravity();
      bPass = Check("gravity-restore", !theControlArena.GravityInverted(),
                    "native world gravity is restored") && bPass;
      b2BodyId const theProjectile = theControlArena.Launch(b2Vec2 { 12.0f, -1.0f });
      b2Vec2 const theInitialVelocity = b2Body_GetLinearVelocity(theProjectile);
      bPass = Check("impulse", b2LengthSquared(theInitialVelocity) > 10.0f,
                    "native Box2D projectile receives a nonzero physical impulse") && bPass;
      theControlArena.Reset();
      bPass = Check("reset", theControlArena.Shots() == 0 &&
                    !theControlArena.GravityInverted(),
                    "world ownership and counters reset cleanly") && bPass;

      int const iSteps = bStress ? 2400 : 480;
      Result const theFirst = Simulate(iSteps);
      Result const theSecond = Simulate(iSteps);
      bPass = Check("fixed-step", theFirst.iPositionSignature == theSecond.iPositionSignature,
                    "repeatable 120 Hz solver state fingerprint") && bPass;
      bPass = Check("native-contact-events",
                    theFirst.iHits > 0 && theFirst.iHits == theSecond.iHits,
                    "real Box2D hit events observed consistently") && bPass;
      bPass = Check("projectile-count",
                    theFirst.iShots == 3 && theSecond.iShots == 3,
                    "scripted physical projectiles fired") && bPass;
      bPass = Check("score", theFirst.iScore >= 0 && theFirst.iScore == theSecond.iScore,
                    "native collision score is repeatable") && bPass;
      bPass = Check("scene", theFirst.uActors >= 40 &&
                    theFirst.uActors == theSecond.uActors,
                    "native Box2D scene contains many installed-world bodies") && bPass;
      std::println("ARENA|METRIC|steps|{}", iSteps);
      std::println("ARENA|METRIC|actors|{}", theFirst.uActors);
      std::println("ARENA|METRIC|hits|{}", theFirst.iHits);
      std::println("ARENA|METRIC|score|{}", theFirst.iScore);
      std::println("ARENA|METRIC|signature|{}", theFirst.iPositionSignature);
      std::println("ARENA|METRIC|elapsed_ms|{:.3f}", theFirst.flElapsedMilliseconds);
      std::println("ARENA|RESULT|{}|SDL3 and Box2D integrated native test",
                   bPass ? "PASS" : "FAIL");
      return bPass ? 0 : 1;
      }
   catch(std::exception const& theException) {
      std::println("ARENA|RESULT|FAIL|{}", theException.what());
      return 2;
      }
   }
