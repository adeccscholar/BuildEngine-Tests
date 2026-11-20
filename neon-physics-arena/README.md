# Neon Physics Arena — SDL3 + Box2D

An independent **C++Builder 13 / BCC64X / C++23** cross-package integration test using the unmodified public interfaces of BuildEngine-managed **SDL3 3.4.20** and **Box2D 3.2.0**.

The interactive Windows application opens a resizable 1600 × 900 neon-blue arena with animated physics towers, ramps, ricocheting orbs, projectile impacts, a live score, radial explosion forces, an animated particle system and a HUD. Box2D performs the actual physics, contact processing and impulses; SDL3 performs native accelerated rendering, event/input handling and real framebuffer capture.

There are **two programs**: `neon-physics-arena.exe` is a Windows GUI application and `neon-physics-verify.exe` is a console-based, headless numerical regression/stress tester. The two CTest cases use only the second executable.

## Interactive controls

| Input | Action |
| --- | --- |
| Left mouse click | Launch a continuous-collision projectile towards the pointer |
| Right mouse click | Apply a radial Box2D impulse near the pointer |
| G | Invert/restore gravity |
| Space | Pause/resume physics |
| R | Reset the deterministic arena |
| P | Capture the **real SDL3 framebuffer** as `neon-physics-arena.bmp` |
| Escape | Exit |

The solver runs at a fixed **120 Hz** with four Box2D substeps per tick. Window size changes use SDL3 logical presentation. SDL3's own built-in debug font renders the HUD without a separately bundled font.

## Build and test

From the BuildEngine-Tests checkout under the production BuildEngine root:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S neon-physics-arena -B ../build/Test/neon-physics-arena/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/neon-physics-arena/Release
cmake --build ../build/Test/neon-physics-arena/Release/cmake --parallel 4
ctest --test-dir ../build/Test/neon-physics-arena/Release/cmake --output-on-failure
cmake --install ../build/Test/neon-physics-arena/Release/cmake
..\build\Test\neon-physics-arena\Release\neon-physics-arena.exe
```

The active versions are resolved from `admin/build-libraries.xml` through the common BuildEngine-Tests helper; no local vendor source or unknown toolchain is selected. Both managed DLLs, matching import libraries and selected Debug/Release packages are required.

## Automated verification

- `NeonPhysicsDeterminism`: SDL3 and Box2D DLL version checks, headless SDL3 initialization, world construction, physical impulses, contact events, a fixed-step state signature, two identical simulation runs and collision-based scoring.
- `NeonPhysicsStress`: the same integration and repeatability checks over 2,400 steps, with performance metrics but no hardware-dependent time limit.

Both tests print `ARENA|CHECK`, `ARENA|METRIC` and `ARENA|RESULT` records, including actual body counts, collision events, score, position signature and elapsed time. The GUI is optional and does not run inside headless CTest.

See [architecture](Description.md), [exact build commands](build.md), [license](LICENSE.md) and [SBOM declaration](SBOM.md).

**[not verified]** BCC64X native compilation, actual screenshot and runtime behaviour require a run on the Windows build host.

## Jolt Physics versus Box2D: Differential Physics Challenge

A third executable, **neon-physics-duel.exe**, independently builds a three-dimensional Jolt Physics 5.7 world and a two-dimensional Box2D 3.2 world from equivalent falling-sphere scenarios. Each solver performs 720 steps at 120 Hz. Two runs of each implementation must reproduce the same sampled positions; independent checks verify gravity, floor contact, finite positions and bounded penetration. The trajectory samples and elapsed times are printed as `DUEL|METRIC|` evidence. The comparison deliberately does not require the two different engines to produce bitwise identical trajectories.

The Jolt consumer follows the accepted DLL's public feature definitions and calls `VerifyJoltVersionID()` before registration. Its owned factory, world and bodies are cleaned up in the required order. A separate `NeonPhysicsDifferential` CTest runs without opening a video or audio device.

```bat
ctest --test-dir ../build/Test/neon-physics-arena/Release/cmake -R NeonPhysicsDifferential --output-on-failure
```

```bat
..\build\Test\neon-physics-arena\Release\neon-physics-duel.exe
```

**[not verified]** The new differential executable has not yet been compiled or run on the native BCC64X host. The operator's successful library FSM run establishes installed Jolt readiness, not execution of this new cross-engine application.
