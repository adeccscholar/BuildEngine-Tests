# Legacy project migration

SDL2, GLEW and SOIL2 application; missing graphics/ textures and 3D assets in source archive; legacy CMake requires TP_CMAKE.

Original source: user-supplied Demo.zip. Sources still need a verified GitHub import; this directory alone is not buildable. Once imported, configure independently using the common `bootstrap/BuildEngineInit.cmake` and install through the existing BuildEngine runtime-staging contract.
