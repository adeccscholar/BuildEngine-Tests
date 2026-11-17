// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once

#include <box2d/box2d.h>

#include <span>
#include <vector>

namespace adecc::tests {

enum class EActorKind {
   platform,
   crate,
   orb,
   projectile
   };

struct Actor {
   b2BodyId theBody { b2_nullBodyId };
   EActorKind eKind { EActorKind::crate };
   float flHalfWidth {};
   float flHalfHeight {};
   float flRadius {};
   int iColor {};
   bool bDynamic {};
   };

struct Impact {
   b2Vec2 thePosition {};
   float flSpeed {};
   };

/**
\brief Owns a native Box2D world for the SDL3 physics demonstration.
\details Provides fixed-timestep dynamics, real collision events, rigid bodies,
         projectiles and impulses. Rendering and SDL event handling are separate.
\note Box2D remains its native C library, without a replacement API.
*/
class PhysicsArena {
public:
   /**
   \brief Constructs the default arena.
   \details Allocates a Box2D world and populates it with repeatable geometry.
   \throw std::runtime_error If a Box2D world cannot be created.
   */
   PhysicsArena();

   /**
   \brief Destroys the owned Box2D world.
   \details World destruction also destroys its bodies and shapes.
   \throw Nothing.
   */
   ~PhysicsArena() noexcept;

   PhysicsArena(PhysicsArena const&) = delete;
   PhysicsArena& operator=(PhysicsArena const&) = delete;

   /**
   \brief Restores the original arena.
   \details Discards the old world and constructs the original scene.
   \throw std::runtime_error If the new Box2D world cannot be created.
   */
   void Reset();

   /**
   \brief Advances the physical world.
   \details Uses the native Box2D solver and copies transient contact events.
   \param flDeltaSeconds Fixed step duration in seconds.
   \throw Nothing intentionally.
   */
   void Step(float flDeltaSeconds);

   /**
   \brief Fires a dynamic continuous-collision projectile.
   \details Launches from the left cannon towards a world-space target.
   \param theTarget World-space target coordinates.
   \returns The created Box2D body ID.
   \throw std::runtime_error If body creation fails.
   */
   [[nodiscard]] b2BodyId Launch(b2Vec2 theTarget);

   /**
   \brief Applies an explosion-like radial impulse to nearby dynamic bodies.
   \details All motion and resulting collisions are handled by Box2D.
   \param theCenter Explosion centre in world coordinates.
   \throw Nothing intentionally.
   */
   void Pulse(b2Vec2 theCenter);

   /**
   \brief Toggles gravity direction.
   \details Modifies the native Box2D world gravity vector.
   \throw Nothing intentionally.
   */
   void ToggleGravity();

   /**
   \brief Exposes the current physics actors.
   \details This borrowed view is invalidated by Reset or structural changes.
   \returns Read-only actor descriptors.
   \throw Nothing.
   */
   [[nodiscard]] std::span<Actor const> Actors() const noexcept;

   /**
   \brief Exposes transient collision hit values copied during Step.
   \details The view is valid until the next Step.
   \returns Read-only hit records.
   \throw Nothing.
   */
   [[nodiscard]] std::span<Impact const> Impacts() const noexcept;

   /**
   \brief Reports projectiles fired since the last reset.
   \details Counter resets with the world.
   \returns Projectile count.
   \throw Nothing.
   */
   [[nodiscard]] int Shots() const noexcept;

   /**
   \brief Reports collision hits since the last reset.
   \details Counts native Box2D hit events, not visual effects.
   \returns Cumulative hit count.
   \throw Nothing.
   */
   [[nodiscard]] int Hits() const noexcept;

   /**
   \brief Reports the collision-based score.
   \details Contact approach speed contributes to the score.
   \returns Current game score.
   \throw Nothing.
   */
   [[nodiscard]] int Score() const noexcept;

   /**
   \brief Indicates inverted gravity.
   \details The normal scene uses downward gravity.
   \returns True when gravity points upward.
   \throw Nothing.
   */
   [[nodiscard]] bool GravityInverted() const noexcept;

private:
   [[nodiscard]] b2BodyId CreateBox(float flX, float flY, float flHalfWidth,
                                     float flHalfHeight, float flAngle,
                                     bool bDynamic, EActorKind eKind, int iColor);
   [[nodiscard]] b2BodyId CreateOrb(float flX, float flY, float flRadius,
                                     EActorKind eKind, int iColor);
   void BuildScene();
   void PruneProjectiles();

   b2WorldId theWorld { b2_nullWorldId };
   std::vector<Actor> vecActors;
   std::vector<Impact> vecImpacts;
   int iShots {};
   int iHits {};
   int iScore {};
   bool bInverted {};
   };

} // namespace adecc::tests
