#pragma once

// Import the already-built BuildEngine DLLs. These definitions must precede
// all ImGui and ImPlot headers. CMake must not split __declspec(dllimport)
// into separate -D arguments for bcc64x.
#if defined(_WIN32)
#ifndef IMGUI_API
#define IMGUI_API __declspec(dllimport)
#endif
#ifndef IMPLOT_API
#define IMPLOT_API __declspec(dllimport)
#endif
#endif
