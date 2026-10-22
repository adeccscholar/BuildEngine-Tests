# Legacy project migration

Existing real Financial C++ example in Demo.zip includes JSON, HTTPS/ECB and subtests; it replaces the prior placeholder but sources are not yet in GitHub. Legacy vendored CMake/third-party scripts must not be used.

Original source: user-supplied Demo.zip. Sources still need a verified GitHub import; this directory alone is not buildable. Once imported, configure independently using the common `bootstrap/BuildEngineInit.cmake` and install through the existing BuildEngine runtime-staging contract.
