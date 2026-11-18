# Build — OpenGL Earth

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S opengl-sdl2-earth -B ../build/Test/opengl-sdl2-earth/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/opengl-sdl2-earth/Release
```

```bat
cmake --build ../build/Test/opengl-sdl2-earth/Release/cmake
```

```bat
cmake --install ../build/Test/opengl-sdl2-earth/Release/cmake
```

```bat
ctest --test-dir ../build/Test/opengl-sdl2-earth/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Applications and operator instructions

**OpenGLSDL2EarthDemo:** Native interactive SDL2/OpenGL 3D logo and day/night Earth.

```bat
..\build\Test\opengl-sdl2-earth\Release\OpenGLSDL2EarthDemo.exe
```

### Controls

Use on-screen scene buttons for Logo, Earth Day, Earth Night. Logo mode: Left/Right and Up/Down orbit the camera, Home/End adjust spotlight azimuth, PageUp/PageDown change spotlight elevation, +/- or wheel changes camera distance. Earth mode: arrows move the light, Home/End pan the camera, +/- or wheel adjusts distance. Esc exits.
