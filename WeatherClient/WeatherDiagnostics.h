// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherDiagnostics.h
    \brief Optional cross-platform diagnostics for GUI and CTest.
    \details No Win32 console attachment, no Python, no additional library.
*/
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

namespace weather_client::diagnostics {
   [[nodiscard]] inline bool Enabled(char const* name) noexcept {
      char const* value=std::getenv(name);
      return value && *value && *value!='0';
   }

   inline void Trace(std::string_view stage) {
      if(!Enabled("WEATHERCLIENT_TRACE_RENDER")) return;
      // Flush a persistent file on every step so a native crash preserves
      // the last successful stage even in a GUI subsystem executable.
      std::ofstream file{"weather-client-render.log",std::ios::app};
      if(file) file<<"[RENDER] "<<stage<<'\n'<<std::flush;
      std::cerr<<"[RENDER] "<<stage<<std::endl;
   }
} // namespace weather_client::diagnostics
