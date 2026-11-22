#pragma once

#include <GL/glew.h>
#include <windows.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tensor_nebula {

/**
\brief User-selectable Gallium backend for a managed Mesa Windows OpenGL DLL.
\details This is a runtime driver request, not a guarantee of successful GPU use.
*/
enum class MesaDriver : int {
   softpipe = 0,
   d3d12 = 1,
   zink = 2
};

enum class PreferredAdapter : int {
   automatic = 0,
   intel = 1,
   nvidia = 2
};

inline constexpr std::array<char const*,3> arrMesaDrivers {
   "Softpipe (CPU reference)",
   "D3D12 (experimental GPU)",
   "Zink / Vulkan (experimental GPU)"
};
inline constexpr std::array<char const*,3> arrAdapters {
   "Automatic", "Intel", "NVIDIA"
};

/**
\brief Requests the Gallium driver before an SDL OpenGL context is created.
\details GALLIUM_DRIVER is interpreted by the loaded Mesa Gallium WGL
   implementation. MESA_D3D12_DEFAULT_ADAPTER_NAME is documented for the D3D12
   driver only. Restarting the OpenGL context is required when switching paths.
   A selected name does not prove GPU ownership; read GL_VENDOR/GL_RENDERER.
\param theDriver Requested Mesa backend.
\param theAdapter Requested D3D12 adapter-name substring.
\returns Nothing.
\throw std::runtime_error If the Windows process environment cannot be updated.
\pre No Mesa WGL context has been created for the new selection.
*/
inline void SelectMesaBackend(MesaDriver const theDriver,
                              PreferredAdapter const theAdapter) {
   wchar_t const* szDriver {};
   switch(theDriver) {
      case MesaDriver::softpipe: szDriver = L"softpipe"; break;
      case MesaDriver::d3d12: szDriver = L"d3d12"; break;
      case MesaDriver::zink: szDriver = L"zink"; break;
   }
   wchar_t const* szAdapter {};
   if(theDriver == MesaDriver::d3d12) {
      switch(theAdapter) {
         case PreferredAdapter::intel: szAdapter = L"Intel"; break;
         case PreferredAdapter::nvidia: szAdapter = L"NVIDIA"; break;
         default: break;
      }
   }
   if(!SetEnvironmentVariableW(L"GALLIUM_DRIVER", szDriver))
      throw std::runtime_error {"Unable to select Mesa Gallium driver"};
   if(szAdapter) {
      if(!SetEnvironmentVariableW(L"MESA_D3D12_DEFAULT_ADAPTER_NAME",szAdapter))
         throw std::runtime_error {"Unable to select preferred D3D12 adapter"};
   }
   else {
      // Deleting an absent environment variable is a successful no-op
      // for this selection and must not prevent Softpipe startup.
      if(!SetEnvironmentVariableW(L"MESA_D3D12_DEFAULT_ADAPTER_NAME",nullptr) &&
         GetLastError()!=ERROR_ENVVAR_NOT_FOUND)
         throw std::runtime_error {"Unable to reset D3D12 adapter selection"};
   }
}

/**
\brief Returns actual renderer information from the active OpenGL context.
\details This report is evidence and is deliberately independent of the
   requested backend or adapter string.
\returns Vendor, renderer, OpenGL version and GLSL version.
\throw Nothing intentionally.
*/
[[nodiscard]] inline std::string ActualOpenGLRenderer() {
   auto ToText = [](GLenum const eName)->std::string {
      auto const* pText = glGetString(eName);
      return pText ? reinterpret_cast<char const*>(pText) : "(unavailable)";
   };
   return "Vendor: " + ToText(GL_VENDOR) +
      " | Renderer: " + ToText(GL_RENDERER) +
      " | Version: " + ToText(GL_VERSION) +
      " | GLSL: " + ToText(GL_SHADING_LANGUAGE_VERSION);
}

/**
\brief Parses an explicitly supported Mesa backend token.
\param svValue Requested command-line value.
\returns Driver enumeration.
\throw std::invalid_argument If the token is unknown.
*/
[[nodiscard]] inline MesaDriver ParseMesaBackend(std::string_view const svValue) {
   if(svValue == "softpipe") return MesaDriver::softpipe;
   if(svValue == "d3d12") return MesaDriver::d3d12;
   if(svValue == "zink") return MesaDriver::zink;
   throw std::invalid_argument {"Unknown Mesa backend; expected softpipe, d3d12 or zink"};
}

/**
\brief Parses a D3D12 GPU adapter selection.
\param svValue Selection token.
\returns Adapter enumeration.
\throw std::invalid_argument If unsupported.
*/
[[nodiscard]] inline PreferredAdapter ParseMesaAdapter(std::string_view const svValue) {
   if(svValue == "auto") return PreferredAdapter::automatic;
   if(svValue == "intel") return PreferredAdapter::intel;
   if(svValue == "nvidia") return PreferredAdapter::nvidia;
   throw std::invalid_argument {"Unknown Mesa adapter; expected auto, intel or nvidia"};
}
}
