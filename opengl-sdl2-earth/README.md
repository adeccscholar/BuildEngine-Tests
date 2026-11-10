# OpenGL + SDL2 Earth Demo

C++23 / OpenGL 3.3 Core demo for the adecc Scholar Studio-37/BCC64X build toolchain.

This demo is based on the user-provided `OpenGLSoil2Example_Earth_v4` and uses the already verified managed packages from the build toolchain:

- SDL2: windowing, events and OpenGL context,
- OpenGL 3.3 Core: rendering,
- GLEW: OpenGL function loading,
- SOIL2: JPG texture loading.

Skia is not used. This is deliberately a pure SDL2/OpenGL reference before the later Skia Ganesh/OpenGL integration.

## Build and run (BuildEngine Tests)

From `D:/local/embarcadero/test_v3/tests`:

```bat
cmake -S opengl-sdl2-earth -B D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release
cmake --build D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake
cmake --install D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake
```

`OpenGLSDL2EarthDemo.exe` and its DLLs are installed directly under `Release/`; resources are under `Release/graphics/`. The application locates resources relative to the executable using `SDL_GetBasePath`.

## Scenes

- `Logo`: adecc 3D mesh with dedicated camera and spotlight controls.
- `Earth (Day)`: Earth with `2k_earth_daymap.jpg`.
- `Earth (Night)`: Earth with `2k_earth_nightmap.jpg`.
- Both Earth variants use `2k_stars_milky_way.jpg` as the star background.

Switch scenes using the buttons drawn inside the OpenGL window.

## Controls

`Esc` exits the application.

### Logo

- `Left` / `Right`: Rotate the camera horizontally around the logo.
- `Up` / `Down`: Rotate the camera vertically around the logo.
- `Home` / `End`: Move the spotlight horizontally.
- `PageUp` / `PageDown`: Change the spotlight elevation.
- `+` / `-` oder Mausrad: Camera distance.

### Earth

- `Left` / `Right`: Move the light horizontally around Earth.
- `Up` / `Down`: Move the light vertically around Earth.
- `Home` / `End`: Pan the camera horizontally.
- `+` / `-` oder Mausrad: Camera distance.

## Windows / SDL Main

The studio_37 toolchain deliberately links SDL2 without `SDL2main`. The target defines `SDL_MAIN_HANDLED`, uses a real `WinMain` and calls `SDL_SetMainReady()` before `SDL_Init`. The `add_executable(... WIN32 ...)` target avoids a separate console window.

## Graphics assets

The `graphics` directory contains:

- `adecc_official_logo_planar_cutout_3d_opengl.obj`
- `adecc_official_logo_planar_cutout_3d_opengl.mtl`
- `2k_earth_daymap.jpg`
- `2k_earth_nightmap.jpg`
- `2k_stars_milky_way.jpg`

According to the supplied original project, Earth textures originate from Solar System Scope:
`https://www.solarsystemscope.com/textures/`

License noted by the original project: CC BY 4.0 — Attribution 4.0 International.

## RAD Studio IDE project (r89)

`OpenGLSDL2EarthDemo.cbproj` opens the same `src` files directly in RAD Studio 13 Florence for `Win64x` Debug/Release editing, building and debugging. See `IDE-PROJECT.md` for package paths, runtime deployment and the MSBuild verification command.
