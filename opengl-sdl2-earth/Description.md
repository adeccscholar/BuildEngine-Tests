# OpenGL Earth

**Test purpose:** A textured rotating globe, star field and 3D logo demonstrate managed SDL2/OpenGL rendering.

This independent BuildEngine-Tests project exercises **SDL2, GLEW, SOIL2, OpenGL** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** OpenGLSDL2EarthDemo.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`OpenGLSDL2EarthDemo.exe`** — Native interactive SDL2/OpenGL 3D logo and day/night Earth.

Use on-screen scene buttons for Logo, Earth Day, Earth Night. Logo mode: Left/Right and Up/Down orbit the camera, Home/End adjust spotlight azimuth, PageUp/PageDown change spotlight elevation, +/- or wheel changes camera distance. Earth mode: arrows move the light, Home/End pan the camera, +/- or wheel adjusts distance. Esc exits.
