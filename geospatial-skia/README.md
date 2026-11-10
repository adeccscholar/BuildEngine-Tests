# GeoSpatial Skia / OpenGL — Address Density and Synthetic Terrain

> Independent C++23 test program using the **CompareTest** dataset. Implemented for BCC64X/BuildEngine; native verification using the Berlin dataset remains pending.

## Objectives

Two alternative data sources and three representations: (1) address points on a Skia canvas, (2) a Skia density map, and (3) a **smoothly filtered density terrain with hills and valleys** as a 2D relief and interactive OpenGL 3D surface. The synthetic elevations represent nearby address counts, **not physical elevations or population counts**.

## Data sources and provenance

Both data formats originate from [CompareTest](../CompareTest/), specifically the routines \`Reading<T>\`, \`WriteToDirectory<T>\` and \`ReadFromDirectory<T>\` in \`MyProcess.h\` and \`Test_Read.cpp\`. The new application builds independently and does not modify CompareTest.

**A — flat input file \`berlin_infos.dat\`:**

\`\`\`text
City;Street;Number;ZipCode;UrbanUnit;UrbanUnitOld;District;Latitude;Longitude
\`\`\`

**B — hierarchical input, where directory structure is itself part of the data:**

\`\`\`text
Root/
  City/
    Borough/
      Neighborhood/
        StreetName.csv
\`\`\`

The seven CSV columns are exactly those retained when CompareTest writes directory-based data:

\`\`\`text
StreetNumber;ZipCode;UrbanUnitOld;Latitude;Longitude;Distance;Bearing
\`\`\`

The reader traverses three directory levels below the city, sorts files deterministically and optionally processes them in parallel using oneTBB. Rendering currently uses GPS coordinates; hierarchy names are part of the format but not yet used for borough filters. Missing or invalid coordinates are counted and excluded. No automatic geocoding is performed.

\`\`\`mermaid
flowchart TD
 A[CompareTest address data] --> B[berlin_infos.dat]
 A --> C[City / Borough / Neighborhood / Street.csv]
 B --> D[Sequential or oneTBB row parser]
 C --> E[Sequential or oneTBB file parser]
 D --> F[Validated GPS points]
 E --> F
 F --> G[Bounding box and local projection]
 G --> H[Skia point view]
 G --> I[Skia cell density]
 G --> J[OpenMP/oneTBB Gaussian smoothing]
 J --> K[Skia 2D-Relief]
 J --> L[OpenGL mesh representing 3D density terrain]
\`\`\`

## Cartographic projection

For Berlin, a local equirectangular approximation is used. With longitude \(\lambda\), latitude \(\varphi\) and reference latitude \(\varphi_0\):

$$
x=R(\lambda-\lambda_0)\cos(\varphi_0),\qquad
y=R(\varphi-\varphi_0).
$$

When fitting the projected coordinates to the canvas, the common factor \(R\) is omitted. Longitude and latitude must use the same coordinate reference, with reference latitude computed from the input data.

The display rectangle is derived from \((x_{\min},y_{\min},x_{\max},y_{\max})\) with 3% margins on each side while preserving aspect ratio:

$$
s=\min\left(\frac{0.94 W}{x_{\max}-x_{\min}},
            \frac{0.94 H}{y_{\max}-y_{\min}}\right).
$$

This local approximation is not suitable for worldwide data crossing the date line or approaching the poles.

## Density, smoothing and height

Let \(H(x,y)\) be the number of addresses in a grid cell. A logarithmic color scale can represent the discrete density:

$$
c(x,y)=\frac{\log(1+H(x,y))}{\log(1+H_{\max})}.
$$

To obtain a **visually continuous topology**, use a Gaussian kernel:

$$
G_\sigma(u,v)=\frac{1}{2\pi\sigma^2}
\exp\left(-\frac{u^2+v^2}{2\sigma^2}\right).
$$

The density surface is a convolution:

$$
D(x,y)=(H*G_\sigma)(x,y).
$$

Because \(G_\sigma(u,v)=g_\sigma(u)g_\sigma(v)\) is separable, smoothing can be performed efficiently in horizontal and vertical passes using disjoint rows and no competing writes.

The synthetic elevation used by the OpenGL viewer is defined by

$$
z(x,y)=z_{\max}\sqrt{\frac{D(x,y)}{D_{\max}}}
$$

The square root makes smaller hills visible. Lower address densities become valleys and stronger concentrations form peaks. This is **synthetic address-density terrain**, not a physical elevation map, building count by property or population distribution.

**Limitation:** The first implementation counts grid cells in pixel coordinates; changing resolution changes effective cell area. Comparable absolute addresses/km² require a future metric grid scale.

\`\`\`mermaid
sequenceDiagram
 participant Read as CompareTest Input
 participant TBB as oneTBB
 participant OMP as OpenMP
 participant Skia as Skia 2D
 participant GL as OpenGL 3D
 Read->>TBB: File and row parsing
 TBB-->>OMP: Coordinate sequence
 OMP->>OMP: Projection and histogram
 OMP->>OMP: Gaussian horizontal + vertical
 OMP-->>Skia: Grid points / density / relief
 OMP-->>GL: Mesh positions and elevations
 GL->>GL: Perspective, mouse rotation, zoom
\`\`\`

## Parallel execution

| Task | Sequential | oneTBB | OpenMP |
|---|---|---|---|
| Read flat file | Yes | Rows | — |
| Read hierarchy | Yes | Files | — |
| Bounding rectangle | Yes | parallel_reduce | — |
| Project GPS to pixels | Yes | parallel_for | parallel for |
| Build histogram | Yes | Projection | Thread-local histograms with deterministic reduction |
| Gaussian smoothing | Yes | Rows | Rows |
| Draw Skia/OpenGL | GUI thread | — | — |

\`\`\`cpp
#pragma omp parallel for num_threads(threads) schedule(static)
for(int row = 0; row < height; ++row) {
   // Only write disjoint destination rows.
   // Read exclusively from the immutable previous convolution pass.
}
\`\`\`

Neither Skia nor OpenGL is used concurrently from multiple worker threads on the same canvas/context. Oversized local OpenMP histograms currently fall back to a sequential, memory-saving reduction.

## Usage

| Program | Purpose |
|---|---|
| \`geospatial_viewer.exe\` | Skia points, cell density, smooth 2D relief |
| \`geospatial_terrain.exe\` | OpenGL 3D terrain from the smoothed grid |
| \`geospatial_benchmark.exe\` | Comparison of serial and parallel processing |

**Skia controls:** Ctrl+O opens the flat file; Ctrl+D opens a street CSV and infers the hierarchy root. A hierarchy directory may also be passed at launch. Keys 1/2/3 select points/cell density/smoothed relief; S/T/P select serial/oneTBB/OpenMP; +/- change cell size.

**3D controls:** Drag the left mouse button to rotate the density surface and use the wheel to zoom; 1/2/3 select top/side/oblique views; Esc exits. Both input formats support command-line arguments. The OpenGL renderer uses the system GPU, not a local Mesa \`opengl32.dll\`.

## Building with BuildEngine

From \`D:/local/embarcadero/test_v3/tests\`:

\`\`\`bat
git pull
cmake -S geospatial-skia -B D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release
cmake --build D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake
cmake --install D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake
\`\`\`

Example invocations:

\`\`\`bat
geospatial_viewer.exe D:/Test/berlin_infos.dat
geospatial_terrain.exe D:/Test/berlin_infos.dat
geospatial_terrain.exe D:/Test/Verzeichnisstruktur
geospatial_benchmark.exe D:/Test/berlin_infos.dat
\`\`\`

The active XML versions of Skia, oneTBB, LLVM OpenMP, fast_float, SDL2 and GLEW are used with dynamic BuildEngine runtime DLLs. An internal CMake OBJECT target is used rather than an additional third-party static library.

## Documentation-server regression

This Markdown page tests **Mermaid flowcharts and sequence diagrams**, MathJax inline and display equations, GFM tables and **C++ syntax highlighting**. The print view should render formulas and diagrams without incomplete graphics or blank placeholders.

## Pending verification

The actual build and real Berlin address dataset had not been run in this environment. First measured results require native BCC64X consumer tests and visual QA. Planned extensions include borough filters from hierarchy paths, metric density/km², configurable Gaussian bandwidth, height scaling, normal-based lighting and synthetic test data with known peak positions.

## SDL2/OpenGL: camera orientation and runtime data-source selection

Terrain rendering uses a **z-up** world: x = east, y = north, z = synthetic density elevation. The initial camera looks obliquely down rather than directly at a vertically oriented grid. The OpenGL projection computes depth and perspective division; mesh orientation is consistent with it.

The 3D viewer can be launched **without arguments**, with source switching handled inside the same SDL2/OpenGL application:

| Key | Action |
|---|---|
| `F` | `berlin_infos.dat` open with the Windows file picker |
| `D` | open the hierarchy root with the Windows folder picker |
| `L` | reload current data source |
| Drop file/folder onto the window | load an alternative source immediately |
| Left mouse button | rotate 3D view |
| Mouse wheel | Zoom |
| `1` | top view (z axis toward the viewer) |
| `2` | side view |
| `3` | oblique view from above |
| `Esc` | exit |

Both data variants call the same `build(path)` pipeline and refresh the GPU vertex buffer after successful loading. A failed source switch retains the previous display. The window title reports the input format, address count, renderer GPU and relief computation time.

```bat
geospatial_terrain.exe "D:/Test/berlin_infos.dat"
geospatial_terrain.exe "D:/Test/Adressverzeichnisse"
geospatial_terrain.exe
```

## Targeted zooming in the Skia address-point map

**This feature applies to `geospatial_viewer.exe`, not the OpenGL terrain viewer.** The mouse wheel zooms around the pointer in **geographic data space**. Drag the left mouse button to pan; `0` restores the original extent.

Each change creates a **fresh projection of original GPS coordinates into the current window**, not an enlarged bitmap. Zooming therefore changes visible point density and spacing without losing original coordinates. Zoom ranges from 1 to 2048 and is multiplied or divided by 1.25 per mouse-wheel step. The window title reports the zoom and number of addresses within the visible grid.

The initial bounding box is computed once at load time. The visible extent is:

$$
B_z=\left[c_x-\frac{w_0}{2z},\;c_y-\frac{h_0}{2z},\;c_x+\frac{w_0}{2z},\;c_y+\frac{h_0}{2z}\right].
$$

`z` is the zoom factor, `(c_x,c_y)` the movable geographic center, and `w_0,h_0` the original projected width and height. The projection preserves the window aspect ratio and a 3% margin on each side.

**Important:** Addresses outside the viewport are **discarded**, not clamped to an image edge. This was fixed in the shared raster core for point, density and Gaussian relief views. At high zoom, all loaded GPS points are checked and projected again. A spatial index (quadtree/R-tree) and asynchronous tile rendering are possible later optimizations.
