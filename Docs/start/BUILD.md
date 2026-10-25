# Building and running the test documentation server

From the **BuildEngine-Tests checkout**, using the C++Builder 13 Developer
Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -P docu-server/PrepareDocumentationAssets.cmake
cmake -S docu-server -B ../build/Test/docu-server/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/docu-server/Release
cmake --build ../build/Test/docu-server/Release/cmake
cmake --install ../build/Test/docu-server/Release/cmake
```

From the repository root, start the installed executable with the explicit
repository root and XML configuration:

```bat
..uildTestdocu-serverReleaseuildengine_tests_docu_server.exe --root . --config DocsDocumentation.xml
```

Open http://127.0.0.1:8781/docs/ .

Use separate Debug and Release build directories. The server binds to loopback
by default, serves Markdown and dedicated documentation JS/images and test SBOMs,
and never executes examples or build scripts via HTTP.
