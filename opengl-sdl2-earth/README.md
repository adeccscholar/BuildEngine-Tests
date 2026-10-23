# OpenGL + SDL2 Earth Demo

C++23 / OpenGL 3.3 Core Demo fuer die adecc Scholar Studio-37/BCC64X-Buildchain.

Das Demo basiert auf dem vom Benutzer bereitgestellten `OpenGLSoil2Example_Earth_v4` und nutzt die bereits verifizierten Managed Packages der Buildchain:

- SDL2: Fenster, Events und OpenGL-Kontext,
- OpenGL 3.3 Core: Rendering,
- GLEW: Laden der OpenGL-Funktionen,
- SOIL2: Laden der JPG-Texturen.

Es wird keine Skia-Library verwendet. Das Demo dient bewusst als reine SDL2/OpenGL-Referenz vor der spaeteren Skia-Ganesh/OpenGL-Integration.

## Build und Start (BuildEngine Tests)

Aus `D:/local/embarcadero/test_v3/tests`:

```bat
cmake -S opengl-sdl2-earth -B D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release
cmake --build D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake
cmake --install D:/local/embarcadero/test_v3/build/test/opengl-sdl2-earth/Release/cmake
```

`OpenGLSDL2EarthDemo.exe` liegt zusammen mit den DLLs direkt in `Release/`,
die Ressourcen in `Release/graphics/`. Die Anwendung sucht Ressourcen
relativ zur EXE mittels SDL_GetBasePath.

## Szenen

- `Logo`: adecc 3D Mesh mit eigener Kamera- und Scheinwerfersteuerung.
- `Earth (Day)`: Erde mit `2k_earth_daymap.jpg`.
- `Earth (Night)`: Erde mit `2k_earth_nightmap.jpg`.
- Beide Earth-Varianten verwenden `2k_stars_milky_way.jpg` als Sternenhintergrund.

Die Szenen koennen ueber die im OpenGL-Fenster gezeichneten Buttons umgeschaltet werden.

## Steuerung

`Esc` beendet die Anwendung.

### Logo

- `Left` / `Right`: Kamera horizontal um das Logo drehen.
- `Up` / `Down`: Kamera vertikal um das Logo drehen.
- `Home` / `End`: Scheinwerfer horizontal bewegen.
- `PageUp` / `PageDown`: Scheinwerferhoehe aendern.
- `+` / `-` oder Mausrad: Kameraabstand.

### Earth

- `Left` / `Right`: Lichtquelle horizontal um die Erde bewegen.
- `Up` / `Down`: Lichtquelle vertikal um die Erde bewegen.
- `Home` / `End`: Kamera horizontal verschieben.
- `+` / `-` oder Mausrad: Kameraabstand.

## Windows / SDL Main

Die studio_37-Buildchain linkt SDL2 absichtlich ohne `SDL2main`. Deshalb setzt das Target `SDL_MAIN_HANDLED`, verwendet ein echtes `WinMain` und ruft vor `SDL_Init` einmal `SDL_SetMainReady()` auf. Durch `add_executable(... WIN32 ...)` entsteht kein separates Konsolenfenster.

## Grafikdateien

Im Verzeichnis `graphics` liegen:

- `adecc_official_logo_planar_cutout_3d_opengl.obj`
- `adecc_official_logo_planar_cutout_3d_opengl.mtl`
- `2k_earth_daymap.jpg`
- `2k_earth_nightmap.jpg`
- `2k_stars_milky_way.jpg`

Die Earth-Texturen stammen laut bereitgestelltem Projekt von Solar System Scope:
`https://www.solarsystemscope.com/textures/`

Lizenz laut bereitgestelltem Projekt: CC BY 4.0 - Attribution 4.0 International.

## RAD Studio IDE project (r89)

`OpenGLSDL2EarthDemo.cbproj` opens the same `src` files directly in RAD Studio 13 Florence for `Win64x` Debug/Release editing, building and debugging. See `IDE-PROJECT.md` for package paths, runtime deployment and the MSBuild verification command.
