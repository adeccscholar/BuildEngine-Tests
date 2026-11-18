# Build and run the Markdown documentation server

Run each command separately from the BuildEngine-Tests checkout root in the RAD Studio 13 BCC64X developer shell. Each code block contains exactly one copyable CMD command.

## Bootstrap

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Prepare documentation assets

```bat
cmake -P docu-server/PrepareDocumentationAssets.cmake
```

## Configure Release

```bat
cmake -S docu-server -B ../build/Test/docu-server/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/docu-server/Release
```

## Build Release

```bat
cmake --build ../build/Test/docu-server/Release/cmake
```

## Install Release

```bat
cmake --install ../build/Test/docu-server/Release/cmake
```

## Run the documentation server

```bat
..\build\Test\docu-server\Release\buildengine_tests_docu_server.exe --root . --config Docs\Documentation.xml
```

Browse http://127.0.0.1:8781/docs/ after launch. Use independent Debug/Release directories. The server binds to loopback and does not execute examples over HTTP.
