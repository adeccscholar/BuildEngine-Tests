// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

// The test data and console diagnostics use Windows ANSI (CP1252).
// Set only the console output code page; do not transcode file contents.
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

inline void ConfigureConsoleCodePage() noexcept {
#if defined(_WIN32)
   SetConsoleOutputCP(1252u);
#endif
}
