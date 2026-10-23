adecc Scholar - BCC64X third-party buildchain
Copyright (c) 2026 adecc Systemhaus GmbH
SPDX-License-Identifier: MIT
Version 2026-08-10-r32

Target:
  RAD Studio 13 Florence
  Windows 64-bit Modern / BCC64X
  Kitware CMake + Ninja

Verified core stack:
  Windows OpenGL
  GLEW DLL
  SDL2 DLL
  SOIL2 DLL
  pugixml 1.16 DLL (C++17, PUGIXML_CHARCONV_FLOAT)
  VCL + DFM + generated project resources
  Skia Release via clang-cl/GN/Ninja
  SkiaBridge Release with BCC64X C ABI import library

Important documentation:
  docs\HANDZETTEL.md            - complete checklist
  docs\VERIFICATION_STATUS.md   - what is verified/open
  docs\DECISIONS.md             - architecture decisions
  docs\NEXT_STEPS.md            - next session
  docs\SESSION_STATE.md         - frozen project state
  docs\ANLEITUNG.md
  docs\BUILDCHAIN.md
  docs\SKIA.md
  docs\VCL_RESOURCE_MIGRATION.md

License:
  Project-authored files: MIT, adecc Systemhaus GmbH, project adecc Scholar.
  Third-party/vendor files retain their own licenses; see THIRD_PARTY_NOTICES.md.


r33 adds OpenSSL 3.5.7 LTS as the TLS/crypto prerequisite before ACE/TAO.
Use scripts\21-download-openssl.cmd, 23-build-openssl.cmd and 24-test-openssl.cmd.
OpenSSL is built as DLLs through the official Win64 build model with private clang-cl; BCC64X import libraries are regenerated from the DLL exports.

r34 embeds the Financial C++23 integration demo using the C++Builder-provided Boost 1.89 headers plus installed pugixml/OpenSSL. Build with scripts\76-build-financial-demo.cmd.
