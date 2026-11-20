# Vector Galaxy — The Shape of Similarity

An interactive mathematical observatory powered by **native BCC64X**, **hnswlib 0.9.0**, **libigl 2.6.4**, **Eigen 5.0.1**, **ImGui** and **SDL2**. Move a query through three-dimensional feature space and watch the closest stars illuminate in real time. The rolling nebula is a real triangular geometry field; libigl computes the vertex normals that influence its shading. The 851 stars are indexed once and searched through hnswlib, not faked by the GPU.

Use the three query sliders to explore the data cloud, set the number of nearest stars, adjust telescope zoom, freeze or resume orbiting, and reset the experiment. No external images, services, trained models, or online access are required.

## Build and run from BuildEngine-Tests root

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S graphics/vector-galaxy -B ../build/Test/vector-galaxy/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/vector-galaxy/Release
cmake --build ../build/Test/vector-galaxy/Release/cmake --parallel 4
ctest --test-dir ../build/Test/vector-galaxy/Release/cmake --output-on-failure
cmake --install ../build/Test/vector-galaxy/Release/cmake
..\build\Test\vector-galaxy\Release\vector-galaxy.exe
```

The headless `--verify` target checks that hnswlib returns the requested 21 distinct indexed points and libigl returns finite mesh normals for all 851 vertices. The interactive GUI is not required for CTest.

**[not verified]** Native compilation, graphical rendering, performance, and installed executable execution have not yet been verified on the Windows BCC64X host.
