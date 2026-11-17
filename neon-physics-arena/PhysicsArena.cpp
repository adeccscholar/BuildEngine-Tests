// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

#include "PhysicsArena.h"

#include <box2d/math_functions.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace adecc::tests {

namespace {
   constexpr b2Vec2 theCannon { -17.5f, -7.6f };
   }

/**
\brief Constructs the deterministic Box2D scene.
\details Delegates all physical object creation to the native Box2D DLL.
\throw std::runtime_error On invalid world allocation.
*/
PhysicsArena::PhysicsArena() {
   Reset();
   }

/**
\brief Destroys the world and all its children.
\details An invalid or uninitialized world is ignored.
\throw Nothing.
*/
PhysicsArena::~PhysicsArena() noexcept {
   if(b2World_IsValid(theWorld)) {
      b2DestroyWorld(theWorld);
      }
   }

/**
\brief Creates a Box2D rigid box.
\details Material friction, restitution and collision events use Box2D's original API.
\param flX World x coordinate.
\param flY World y coordinate.
\param flHalfWidth Half-width in metres.
\param flHalfHeight Half-height in metres.
\param flAngle Initial angle in radians.
\param bDynamic Whether the rigid body responds to forces.
\param eKind Visual descriptor of the body.
\param iColor Palette index.
\returns Valid native Box2D body handle.
\throw std::runtime_error If body creation fails.
*/
b2BodyId PhysicsArena::CreateBox(float const flX, float const flY,
                                 float const flHalfWidth, float const flHalfHeight,
                                 float const flAngle, bool const bDynamic,
                                 EActorKind const eKind, int const iColor) {
   b2BodyDef theBodyDef = b2DefaultBodyDef();
   theBodyDef.type = bDynamic ? b2_dynamicBody : b2_staticBody;
   theBodyDef.position = b2ToPos(b2Vec2 { flX, flY });
   theBodyDef.rotation = b2MakeRot(flAngle);
   theBodyDef.angularDamping = 0.09f;
   b2BodyId const theBody = b2CreateBody(theWorld, &theBodyDef);
   if(!b2Body_IsValid(theBody)) {
      throw std::runtime_error("Box2D box body creation failed");
      }

   b2ShapeDef theShapeDef = b2DefaultShapeDef();
   theShapeDef.density = 1.15f;
   theShapeDef.material.friction = 0.65f;
   theShapeDef.material.restitution = eKind == EActorKind::platform ? 0.15f : 0.26f;
   theShapeDef.enableHitEvents = bDynamic;
   b2Polygon const theShape = b2MakeBox(flHalfWidth, flHalfHeight);
   b2CreatePolygonShape(theBody, &theShapeDef, &theShape);
   vecActors.push_back(Actor {
      .theBody = theBody, .eKind = eKind,
      .flHalfWidth = flHalfWidth, .flHalfHeight = flHalfHeight,
      .flRadius = 0.0f, .iColor = iColor, .bDynamic = bDynamic
      });
   return theBody;
   }

/**
\brief Creates a circular dynamic body.
\details Continuous collision is enabled for small fast projectiles.
\param flX World x coordinate.
\param flY World y coordinate.
\param flRadius Circle radius in metres.
\param eKind Rendering and simulation actor category.
\param iColor Palette index.
\returns The Box2D body identifier.
\throw std::runtime_error If allocation fails.
*/
b2BodyId PhysicsArena::CreateOrb(float const flX, float const flY,
                                 float const flRadius, EActorKind const eKind, int const iColor) {
   b2BodyDef theBodyDef = b2DefaultBodyDef();
   theBodyDef.type = b2_dynamicBody;
   theBodyDef.position = b2ToPos(b2Vec2 { flX, flY });
   theBodyDef.isBullet = eKind == EActorKind::projectile;
   theBodyDef.angularDamping = 0.04f;
   b2BodyId const theBody = b2CreateBody(theWorld, &theBodyDef);
   if(!b2Body_IsValid(theBody)) {
      throw std::runtime_error("Box2D orb body creation failed");
      }
   b2ShapeDef theShapeDef = b2DefaultShapeDef();
   theShapeDef.density = eKind == EActorKind::projectile ? 5.0f : 1.0f;
   theShapeDef.material.friction = 0.3f;
   theShapeDef.material.restitution = eKind == EActorKind::projectile ? 0.75f : 0.55f;
   theShapeDef.enableHitEvents = true;
   b2Circle const theCircle { .center = { 0.0f, 0.0f }, .radius = flRadius };
   b2CreateCircleShape(theBody, &theShapeDef, &theCircle);
   vecActors.push_back(Actor {
      .theBody = theBody, .eKind = eKind,
      .flRadius = flRadius, .iColor = iColor, .bDynamic = true
      });
   return theBody;
   }

/**
\brief Populates a reproducible arena with towers and ramps.
\details Physics bodies are entirely owned by the Box2D world.
\throw std::runtime_error If any body allocation fails.
*/
void PhysicsArena::BuildScene() {
   (void)CreateBox(0.0f, -10.4f, 19.3f, 0.5f, 0.0f, false, EActorKind::platform, 0);
   (void)CreateBox(-19.1f, 0.0f, 0.45f, 10.3f, 0.0f, false, EActorKind::platform, 0);
   (void)CreateBox(19.1f, 0.0f, 0.45f, 10.3f, 0.0f, false, EActorKind::platform, 0);
   (void)CreateBox(0.0f, 10.15f, 19.3f, 0.4f, 0.0f, false, EActorKind::platform, 0);
   (void)CreateBox(-7.1f, -7.3f, 3.1f, 0.32f, -0.22f, false, EActorKind::platform, 0);
   (void)CreateBox(7.0f, -7.0f, 2.6f, 0.30f, 0.20f, false, EActorKind::platform, 0);
   (void)CreateBox(0.0f, -4.0f, 2.4f, 0.3f, 0.0f, false, EActorKind::platform, 0);

   // A staggered wall and two pyramids: impacts should propagate through
   // native narrow-phase contact, friction and rigid-body angular momentum.
   for(int iColumn = 0; iColumn < 7; ++iColumn) {
      for(int iRow = 0; iRow < 6; ++iRow) {
         if((iColumn + iRow) % 4 == 0 && iRow > 2) {
            continue;
            }
         (void)CreateBox(3.4f + iColumn * 1.05f, -9.4f + iRow * 1.02f,
                         0.48f, 0.47f, 0.0f, true, EActorKind::crate,
                         (iColumn + iRow) % 5);
         }
      }
   for(int iColumn = 0; iColumn < 5; ++iColumn) {
      for(int iRow = 0; iRow < 5 - iColumn; ++iRow) {
         (void)CreateBox(12.0f + 0.92f * iColumn + 0.45f * iRow,
                         -9.35f + iRow * 0.96f,
                         0.42f, 0.43f, 0.0f, true, EActorKind::crate, (iColumn + 2) % 5);
         }
      }
   for(int i = 0; i < 14; ++i) {
      (void)CreateOrb(-9.7f + 0.83f * static_cast<float>(i % 5),
                      -2.3f + 0.96f * static_cast<float>(i / 5),
                      0.28f + 0.035f * static_cast<float>(i % 3),
                      EActorKind::orb, 1 + i % 5);
      }
   }

/**
\brief Restores the baseline physics scene.
\details Ensures a fresh Box2D world with a single deterministic worker.
\throw std::runtime_error If the world cannot be created.
*/
void PhysicsArena::Reset() {
   if(b2World_IsValid(theWorld)) {
      b2DestroyWorld(theWorld);
      }
   theWorld = b2_nullWorldId;
   vecActors.clear();
   vecImpacts.clear();
   iShots = 0;
   iHits = 0;
   iScore = 0;
   bInverted = false;

   b2WorldDef theDef = b2DefaultWorldDef();
   theDef.gravity = b2Vec2 { 0.0f, -9.81f };
   theDef.workerCount = 1;
   theDef.hitEventThreshold = 1.0f;
   theWorld = b2CreateWorld(&theDef);
   if(!b2World_IsValid(theWorld)) {
      throw std::runtime_error("Box2D world creation failed");
      }
   BuildScene();
   }

/**
\brief Fires a continuous-collision projectile.
\details Velocity is applied through Box2D rather than direct coordinate animation.
\param theTarget Requested target position.
\returns ID of the created projectile.
\throw std::runtime_error On a failed body allocation.
*/
b2BodyId PhysicsArena::Launch(b2Vec2 const theTarget) {
   b2BodyId const theBody = CreateOrb(theCannon.x, theCannon.y, 0.32f,
                                     EActorKind::projectile, iShots % 5);
   b2Vec2 const theDelta = b2Sub(theTarget, theCannon);
   b2Vec2 const theDirection = b2Normalize(
      b2LengthSquared(theDelta) > 0.0001f ? theDelta : b2Vec2 { 1.0f, 0.35f });
   float const flMass = b2Body_GetMass(theBody);
   b2Body_ApplyLinearImpulseToCenter(theBody,
                                     b2MulSV(22.0f * flMass, theDirection), true);
   ++iShots;
   return theBody;
   }

/**
\brief Applies a radial explosion to surrounding rigid bodies.
\details Impulse falls off with distance and only affects dynamic bodies.
\param theCenter Explosion centre in world coordinates.
\throw Nothing intentionally.
*/
void PhysicsArena::Pulse(b2Vec2 const theCenter) {
   for(Actor const& theActor : vecActors) {
      if(!theActor.bDynamic || !b2Body_IsValid(theActor.theBody)) {
         continue;
         }
      b2Vec2 const thePosition = b2ToVec2(b2Body_GetPosition(theActor.theBody));
      b2Vec2 const theDelta = b2Sub(thePosition, theCenter);
      float const flDistanceSquared = b2LengthSquared(theDelta);
      if(flDistanceSquared < 0.001f || flDistanceSquared > 49.0f) {
         continue;
         }
      float const flDistance = std::sqrt(flDistanceSquared);
      float const flStrength = (1.0f - flDistance / 7.0f) * 15.0f;
      b2Body_ApplyLinearImpulseToCenter(theActor.theBody,
         b2MulSV(flStrength * b2Body_GetMass(theActor.theBody) / flDistance, theDelta),
         true);
      }
   }

/**
\brief Inverts gravity.
\details Uses Box2D's world-gravity setter without changing body transforms.
\throw Nothing intentionally.
*/
void PhysicsArena::ToggleGravity() {
   bInverted = !bInverted;
   b2World_SetGravity(theWorld, b2Vec2 { 0.0f, bInverted ? 9.81f : -9.81f });
   }

/**
\brief Removes out-of-bounds projectiles.
\details Structural cleanup happens after the solver step.
\throw Nothing intentionally.
*/
void PhysicsArena::PruneProjectiles() {
   std::erase_if(vecActors, [](Actor const& theActor) -> bool {
      if(theActor.eKind != EActorKind::projectile) {
         return false;
         }
      if(!b2Body_IsValid(theActor.theBody)) {
         return true;
         }
      b2Vec2 const thePosition = b2ToVec2(b2Body_GetPosition(theActor.theBody));
      bool const bOutside = std::abs(thePosition.x) > 30.0f ||
                            std::abs(thePosition.y) > 30.0f;
      if(bOutside) {
         b2DestroyBody(theActor.theBody);
         }
      return bOutside;
      });
   }

/**
\brief Advances the native solver and captures impact events.
\details Hit data is copied because Box2D invalidates the event storage on the next step.
\param flDeltaSeconds Fixed simulation step.
\throw Nothing intentionally.
*/
void PhysicsArena::Step(float const flDeltaSeconds) {
   b2World_Step(theWorld, flDeltaSeconds, 4);
   vecImpacts.clear();
   b2ContactEvents const theEvents = b2World_GetContactEvents(theWorld);
   for(int i = 0; i < theEvents.hitCount; ++i) {
      b2ContactHitEvent const& theEvent = theEvents.hitEvents[i];
      vecImpacts.push_back(Impact {
         .thePosition = b2ToVec2(theEvent.point),
         .flSpeed = theEvent.approachSpeed
         });
      iScore += static_cast<int>(std::min(150.0f, theEvent.approachSpeed * 3.0f));
      ++iHits;
      }
   PruneProjectiles();
   }

/**
\brief Provides borrowed actor descriptors.
\details IDs belong to the current Box2D world.
\returns Actor span.
\throw Nothing.
*/
std::span<Actor const> PhysicsArena::Actors() const noexcept {
   return vecActors;
   }

/**
\brief Provides recent native contact-hit data.
\details Values are copied on each simulation step.
\returns Impact span.
\throw Nothing.
*/
std::span<Impact const> PhysicsArena::Impacts() const noexcept {
   return vecImpacts;
   }

/**
\brief Reports projectile count.
\details Incremented on each Launch.
\returns Shots fired.
\throw Nothing.
*/
int PhysicsArena::Shots() const noexcept {
   return iShots;
   }

/**
\brief Reports contact-hit count.
\details Aggregated from Box2D hit events.
\returns Hit count.
\throw Nothing.
*/
int PhysicsArena::Hits() const noexcept {
   return iHits;
   }

/**
\brief Reports current score.
\details Score is an integer function of collision speed.
\returns Score.
\throw Nothing.
*/
int PhysicsArena::Score() const noexcept {
   return iScore;
   }

/**
\brief Reports whether gravity is inverted.
\details Normal mode points down.
\returns Gravity state.
\throw Nothing.
*/
bool PhysicsArena::GravityInverted() const noexcept {
   return bInverted;
   }

} // namespace adecc::tests
