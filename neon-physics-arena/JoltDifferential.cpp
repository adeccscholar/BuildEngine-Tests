// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Two independently implemented physics solvers, a shared physical scenario,
// and reproducible evidence. The test does NOT claim bitwise agreement.
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <box2d/box2d.h>
#include <box2d/math_functions.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <print>
#include <stdexcept>
#include <string_view>

namespace {
constexpr JPH::ObjectLayer uStaticLayer = 0;
constexpr JPH::ObjectLayer uMovingLayer = 1;
constexpr int iSteps = 720;
constexpr float flStep = 1.0F / 120.0F;

/** \brief Provides deterministic object-to-broadphase mapping for two layers. */
class Broadphase final : public JPH::BroadPhaseLayerInterface {
public:
   JPH::uint GetNumBroadPhaseLayers() const override { return 2; }
   JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer const uLayer) const override {
      return JPH::BroadPhaseLayer { static_cast<JPH::BroadPhaseLayer::Type>(uLayer) };
   }
#ifdef JPH_PROFILE_ENABLED
   char const* GetBroadPhaseLayerName(JPH::BroadPhaseLayer const theLayer) const override {
      return theLayer.GetValue() == 0 ? "Static" : "Moving";
   }
#endif
};

/** \brief Restricts static-static collisions in the physics test. */
class LayerPairs final : public JPH::ObjectLayerPairFilter {
public:
   bool ShouldCollide(JPH::ObjectLayer const uA, JPH::ObjectLayer const uB) const override {
      return uA == uMovingLayer || uB == uMovingLayer;
   }
};

/** \brief Uses the matching broadphase rule for the installed Jolt DLL. */
class LayerVsBroadphase final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
   bool ShouldCollide(JPH::ObjectLayer const uLayer, JPH::BroadPhaseLayer const thePhase) const override {
      return uLayer == uMovingLayer || thePhase.GetValue() == 1;
   }
};

struct Simulation {
   std::array<double, 6> arrSampledHeight {};
   double flFinalHeight {};
   double flMinimumHeight {};
   double flElapsedMs {};
};

/**
\brief Exercises the installed Jolt 5.7 DLL through actual rigid bodies.
\details Allocators, ABI, type registration, broadphase and physics simulation
         all cross the installed DLL boundary. Returns a sampled trajectory.
\returns Deterministic fixed-step Jolt body heights and runtime metric.
\throw std::runtime_error When the installed engine cannot initialize.
*/
Simulation RunJolt() {
   if(!JPH::VerifyJoltVersionID())
      throw std::runtime_error { "Jolt public ABI feature version mismatch" };
   JPH::RegisterDefaultAllocator();
   JPH::Factory::sInstance = new JPH::Factory();
   JPH::RegisterTypes();
   struct Guard {
      ~Guard() {
         JPH::UnregisterTypes();
         delete JPH::Factory::sInstance;
         JPH::Factory::sInstance = nullptr;
      }
   } const theGuard {};
   Broadphase theBroadphase;
   LayerPairs theLayerPairs;
   LayerVsBroadphase theVsBroadphase;
   JPH::PhysicsSystem theSystem;
   theSystem.Init(128, 0, 128, 128, theBroadphase, theVsBroadphase, theLayerPairs);
   JPH::BodyInterface& theBodies = theSystem.GetBodyInterface();
   JPH::BodyCreationSettings const theFloor {
      new JPH::BoxShape(JPH::Vec3 { 12.0F, .5F, 12.0F }),
      JPH::RVec3 { 0.0, -.5, 0.0 }, JPH::Quat::sIdentity(),
      JPH::EMotionType::Static, uStaticLayer
   };
   JPH::BodyCreationSettings const theSphere {
      new JPH::SphereShape(.5F),
      JPH::RVec3 { 0.0, 7.0, 0.0 }, JPH::Quat::sIdentity(),
      JPH::EMotionType::Dynamic, uMovingLayer
   };
   JPH::BodyID const theFloorId = theBodies.CreateAndAddBody(theFloor, JPH::EActivation::DontActivate);
   JPH::BodyID const theSphereId = theBodies.CreateAndAddBody(theSphere, JPH::EActivation::Activate);
   JPH::TempAllocatorMalloc theAllocator;
   JPH::JobSystemSingleThreaded theJobs { JPH::cMaxPhysicsJobs };
   Simulation theResult;
   theResult.flMinimumHeight = 1000.0;
   auto const theStart = std::chrono::steady_clock::now();
   for(int i = 0; i < iSteps; ++i) {
      JPH::EPhysicsUpdateError const theError = theSystem.Update(flStep, 1, &theAllocator, &theJobs);
      if(theError != JPH::EPhysicsUpdateError::None)
         throw std::runtime_error { "Jolt PhysicsSystem::Update failed" };
      double const flHeight = theBodies.GetCenterOfMassPosition(theSphereId).GetY();
      if(!std::isfinite(flHeight)) throw std::runtime_error { "Jolt non-finite trajectory" };
      theResult.flMinimumHeight = std::min(theResult.flMinimumHeight, flHeight);
      if((i + 1) % 120 == 0)
         theResult.arrSampledHeight[static_cast<std::size_t>(i / 120)] = flHeight;
      theResult.flFinalHeight = flHeight;
   }
   theResult.flElapsedMs = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - theStart).count();
   theBodies.RemoveBody(theSphereId);
   theBodies.DestroyBody(theSphereId);
   theBodies.RemoveBody(theFloorId);
   theBodies.DestroyBody(theFloorId);
   return theResult;
}

/**
\brief Runs the equivalent vertical drop using the separately installed Box2D DLL.
\details This independent 2D solver is not expected to reproduce Jolt's exact
         numeric results, since their contact models and dimensions differ.
\returns Six heights sampled at the same fixed timestamps as Jolt.
\throw std::runtime_error When the Box2D world cannot be initialized.
*/
Simulation RunBox2D() {
   b2WorldDef theWorldDef = b2DefaultWorldDef();
   theWorldDef.gravity = b2Vec2 { 0.0F, -9.81F };
   b2WorldId const theWorld = b2CreateWorld(&theWorldDef);
   if(!b2World_IsValid(theWorld)) throw std::runtime_error { "Box2D world creation failed" };
   struct Guard {
      b2WorldId theWorld;
      ~Guard() { b2DestroyWorld(theWorld); }
   } const theGuard { theWorld };
   b2BodyDef theGround = b2DefaultBodyDef();
   b2BodyId const theGroundId = b2CreateBody(theWorld, &theGround);
   b2Polygon const theFloor = b2MakeOffsetBox(12.0F, .5F, b2Vec2 {0.0F, -.5F}, b2Rot_identity);
   b2ShapeDef theGroundShape = b2DefaultShapeDef();
   (void)b2CreatePolygonShape(theGroundId, &theGroundShape, &theFloor);
   b2BodyDef theBall = b2DefaultBodyDef();
   theBall.type = b2_dynamicBody;
   theBall.position = b2Vec2 { 0.0F, 7.0F };
   b2BodyId const theBallId = b2CreateBody(theWorld, &theBall);
   b2Circle const theCircle { b2Vec2_zero, .5F };
   b2ShapeDef theBallShape = b2DefaultShapeDef();
   theBallShape.density = 1.0F;
   (void)b2CreateCircleShape(theBallId, &theBallShape, &theCircle);
   Simulation theResult;
   theResult.flMinimumHeight = 1000.0;
   auto const theStart = std::chrono::steady_clock::now();
   for(int i = 0; i < iSteps; ++i) {
      b2World_Step(theWorld, flStep, 4);
      double const flHeight = b2Body_GetPosition(theBallId).y;
      if(!std::isfinite(flHeight)) throw std::runtime_error { "Box2D non-finite trajectory" };
      theResult.flMinimumHeight = std::min(theResult.flMinimumHeight, flHeight);
      if((i + 1) % 120 == 0)
         theResult.arrSampledHeight[static_cast<std::size_t>(i / 120)] = flHeight;
      theResult.flFinalHeight = flHeight;
   }
   theResult.flElapsedMs = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - theStart).count();
   return theResult;
}

bool Check(std::string_view const svId, bool const bPassed, std::string_view const svDetail) {
   std::println("DUEL|CHECK|{}|{}|{}", svId, bPassed ? "PASS" : "FAIL", svDetail);
   return bPassed;
}
}

int main() {
   try {
      Simulation const theJoltA = RunJolt();
      Simulation const theJoltB = RunJolt();
      Simulation const theBoxA = RunBox2D();
      Simulation const theBoxB = RunBox2D();
      bool bPassed = true;
      bPassed = Check("jolt-repeatability", theJoltA.arrSampledHeight == theJoltB.arrSampledHeight,
                      "Two independent Jolt worlds follow identical scripted trajectories") && bPassed;
      bPassed = Check("box2d-repeatability", theBoxA.arrSampledHeight == theBoxB.arrSampledHeight,
                      "Two independent Box2D worlds follow identical scripted trajectories") && bPassed;
      bPassed = Check("gravity", theJoltA.arrSampledHeight[0] < 7.0 &&
                      theBoxA.arrSampledHeight[0] < 7.0,
                      "Both native solvers move the falling sphere below its initial height") && bPassed;
      bPassed = Check("ground-contact", theJoltA.flFinalHeight >= .35 && theBoxA.flFinalHeight >= .35,
                      "Both solvers keep the sphere above the solid floor after settling") && bPassed;
      bPassed = Check("bounded-motion", theJoltA.flMinimumHeight > -.2 && theBoxA.flMinimumHeight > -.2,
                      "Neither native solver tunnels substantially through the floor") && bPassed;
      for(std::size_t u = 0; u < theJoltA.arrSampledHeight.size(); ++u)
         std::println("DUEL|METRIC|height_t{}|jolt={:.6f};box2d={:.6f}",
                      u + 1, theJoltA.arrSampledHeight[u], theBoxA.arrSampledHeight[u]);
      std::println("DUEL|METRIC|jolt_elapsed_ms|{:.3f}", theJoltA.flElapsedMs);
      std::println("DUEL|METRIC|box2d_elapsed_ms|{:.3f}", theBoxA.flElapsedMs);
      std::println("DUEL|RESULT|{}|Independent fixed-step Jolt 3D and Box2D 2D differential physics test",
                   bPassed ? "PASS" : "FAIL");
      return bPassed ? 0 : 1;
   }
   catch(std::exception const& theError) {
      std::println("DUEL|RESULT|FAIL|{}", theError.what());
      return 2;
   }
}
