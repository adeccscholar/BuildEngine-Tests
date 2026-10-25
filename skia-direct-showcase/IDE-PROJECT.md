<!--
Copyright (c) 2026 adecc Systemhaus GmbH
SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
Project: adecc Scholar
-->
# C++Builder IDE project

Open `SkiaDirectShowcase.cbproj` in RAD Studio 13 Florence and select `Win64x`.

The project file intentionally lists only the real C++ translation unit:

```text
main.cpp
```

There is no `MainSource` metadata, no `Source Name="MainSource"`, and no extra header/document/CMake items in the project file. The `.cpp` file is a normal `CppCompile` item with `BuildOrder`.

The CMake/Ninja project remains separate and uses the same physical source file.

## Builder-native project behavior

The project is a Windows GUI application with `FrameworkType=None`; it is not a console project. C++Builder owns the output convention: `.\$(Platform)\$(Config)` (for example `Win64x\Debug`). The post-build event only copies the four image assets beside the Builder-generated executable.


## r97 Builder start/deployment metadata

The project registers `.\Win64x\Debug\SkiaDirectShowcase.exe` and `.\Win64x\Release\SkiaDirectShowcase.exe` as Builder `ProjectOutput`; the Debug PDB is registered as `DebugSymbols`. The standard optional `$(MSBuildProjectName).deployproj` import is present. This follows the frozen Builder-generated `reference\cbproj\Project1.cbproj` convention and does not change the source list or output directories.

## r99 Builder convention
The project now deliberately keeps the supplied Builder console-project identity (`AppType=Console`, `CppConsoleApplication`) and selects the GUI subsystem only through Builder's Windows-app properties. Output directories use Builder's native `.\$(Platform)\$(Config)` convention without custom roots or trailing-backslash workarounds.
