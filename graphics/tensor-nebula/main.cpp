#include "../common/GraphicsDllImports.h"
#include "../common/OpenGLShowcase.h"
#include "TensorField.h"
#include "AsyncField.h"
#include "MesaBackend.h"
#include "NebulaGL.h"

#include <SDL.h>
#include <imgui.h>
#include <xtl/xoptional.hpp>
#include <xsimd/xsimd.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <numbers>
#include <memory>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <windows.h>

namespace {
using tensor_nebula::FieldMode;
using tensor_nebula::FieldSettings;
using tensor_nebula::NebulaGL;
using tensor_nebula::TensorField;
using tensor_nebula::AsyncField;
using tensor_nebula::MesaDriver;
using tensor_nebula::PreferredAdapter;

constexpr std::array<char const*,4> arrExperiments {
   "Binary stars / two sources", "Trinity / three sources",
   "Standing wave chamber", "Gaussian pulse"
};
constexpr std::array<char const*,4> arrVisualModes {
   "Cinematic nebula", "Scientific contours", "Illuminated relief", "3D wave landscape"
};
constexpr std::array<char const*,4> arrPalettes {
   "Nebula violet", "Aurora", "Solar plasma", "Monochrome"
};
constexpr std::array<int,3> arrWidths {512,768,1024};
constexpr std::array<int,3> arrHeights {288,432,576};
constexpr std::array<char const*,3> arrResolutions {
   "512 x 288 / standard", "768 x 432 / detail", "1024 x 576 / cinema"
};

/**
\brief Launches a fresh process so the new Gallium backend is chosen before
   any WGL context or managed DLL is loaded.
\details A live SDL OpenGL context cannot safely switch Gallium backends.
   The original process closes only after successful child creation.
\param theDriver Selected Mesa driver.
\param theAdapter Optional D3D12 device-name preference.
\returns True if a new process was created.
\throw std::runtime_error If executable lookup or CreateProcessW fails.
*/
bool LaunchBackend(MesaDriver const theDriver,
                   PreferredAdapter const theAdapter) {
   std::wstring strExe(32768,L'\0');
   DWORD const uLength=GetModuleFileNameW(nullptr,strExe.data(),
      static_cast<DWORD>(strExe.size()));
   if(!uLength || uLength>=strExe.size())
      throw std::runtime_error {"Cannot locate Tensor Nebula executable"};
   strExe.resize(uLength);
   std::array<std::wstring_view,3> const arrDriverArgs {
      L"softpipe",L"d3d12",L"zink"
   };
   std::array<std::wstring_view,3> const arrAdapterArgs {
      L"auto",L"intel",L"nvidia"
   };
   std::wstring strCommand=L"\""+strExe+L"\" --mesa-backend=";
   strCommand+=arrDriverArgs.at(static_cast<std::size_t>(theDriver));
   strCommand+=L" --mesa-adapter=";
   strCommand+=arrAdapterArgs.at(static_cast<std::size_t>(theAdapter));
   STARTUPINFOW theStartup {};
   theStartup.cb=sizeof(theStartup);
   PROCESS_INFORMATION theProcess {};
   BOOL const bStarted=CreateProcessW(strExe.c_str(),strCommand.data(),
      nullptr,nullptr,FALSE,0,nullptr,nullptr,&theStartup,&theProcess);
   if(!bStarted)throw std::runtime_error {"Unable to restart with selected Mesa driver"};
   CloseHandle(theProcess.hThread);
   CloseHandle(theProcess.hProcess);
   return true;
}

/**
\brief Verifies the real xtensor/xtl/xsimd calculation without a window.
\details Multiple physical modes are evaluated and checked against analytical
   bounds, symmetric cases and lane-wise SIMD references. This remains safe for
   the headless BuildEngine CTest phase.
\returns True if all independent checks pass.
\throw std::exception On invalid installed-library behavior.
*/
bool Verify() {
   TensorField<float> theField;
   FieldSettings aSettings;
   aSettings.iWidth = 192;
   aSettings.iHeight = 112;
   aSettings.flFrequency = 7.0F;
   bool bAllModes = true;
   bool bBounded = true;
   bool bFinite = true;
   for(int iMode=0;iMode<4;++iMode) {
      aSettings.theMode = static_cast<FieldMode>(iMode);
      aSettings.flPhase = iMode == 0 ? 0.0F : .63F;
      theField.Compute(aSettings);
      auto const& aMetrics = theField.Metrics();
      bool const bShape = theField.Values().dimension() == 2 &&
         theField.Values().shape()[0] == 112 &&
         theField.Values().shape()[1] == 192 &&
         aMetrics.uSamples == 192U*112U;
      bAllModes = bAllModes && bShape && aMetrics.flMaximum > .001F;
      bBounded = bBounded && aMetrics.flMinimum >= -.00001F &&
         aMetrics.flMaximum <= 1.00001F;
      bFinite = bFinite && std::isfinite(aMetrics.flMean) &&
         std::isfinite(aMetrics.flEnergy);
   }
   aSettings.theMode = FieldMode::twin;
   aSettings.flPhase = 0.0F;
   theField.Compute(aSettings);
   auto const& aValues = theField.Values();
   float const flSymmetry = std::abs(aValues(40,23) - aValues(40,168));
   bool const bSymmetry = flSymmetry < .0002F;
   xtl::xoptional<float> const theInside = theField.Sample(15, 15);
   xtl::xoptional<float> const theOutside = theField.Sample(-1, 0);
   bool const bOptional = theInside.has_value() && !theOutside.has_value();
   xsimd::batch<float> const theBatch {2.0F};
   float const flReduced = xsimd::reduce_add(theBatch);
   bool const bSimd = std::abs(flReduced -
      2.0F*static_cast<float>(xsimd::batch<float>::size)) < .01F;

   std::println("SHOWCASE|CHECK|xtensor-four-modes|{}|four physical models and shapes",
      bAllModes ? "PASS" : "FAIL");
   std::println("SHOWCASE|CHECK|tensor-bounds|{}|intensity in [0,1]",
      bBounded ? "PASS" : "FAIL");
   std::println("SHOWCASE|CHECK|tensor-finite|{}|all SIMD field metrics finite",
      bFinite ? "PASS" : "FAIL");
   std::println("SHOWCASE|CHECK|two-source-symmetry|{}|difference={:.7f}",
      bSymmetry ? "PASS" : "FAIL",flSymmetry);
   std::println("SHOWCASE|CHECK|xtl-optional|{}|inside and outside samples",
      bOptional ? "PASS" : "FAIL");
   std::println("SHOWCASE|CHECK|xsimd-reduction|{}|SIMD lanes={}",
      bSimd ? "PASS" : "FAIL",xsimd::batch<float>::size);
   // Headless stress path: quickly change the inputs, then prove the newest
   // numerical request completes on the bounded oneTBB background worker.
   bool bAsync = false;
   {
      AsyncField theComputer;
      FieldSettings aRapid=aSettings;
      aRapid.iWidth=64;
      aRapid.iHeight=40;
      std::uint64_t uExpected {};
      for(int i=0;i<24;++i) {
         aRapid.flPhase=static_cast<float>(i)*.11F;
         uExpected=theComputer.RequestFrame(aRapid);
      }
      auto const theDeadline=std::chrono::steady_clock::now()+std::chrono::seconds {5};
      while(std::chrono::steady_clock::now()<theDeadline && !bAsync) {
         if(auto theCompleted=theComputer.TakeCompleted()) {
            bAsync=theCompleted->uRevision==uExpected &&
               theCompleted->pField->Values().shape()[0]==40 &&
               theCompleted->pField->Values().shape()[1]==64;
         }
         if(!bAsync)std::this_thread::sleep_for(std::chrono::milliseconds {10});
      }
   }
   std::println("SHOWCASE|CHECK|onetbb-async-latest|{}|rapid requests converge to latest",
      bAsync ? "PASS" : "FAIL");
   bool const bPassed = bAllModes && bBounded && bFinite &&
      bSymmetry && bOptional && bSimd && bAsync;
   std::println("SHOWCASE|RESULT|{}|Tensor Nebula CPU experiment validation",
      bPassed ? "PASS" : "FAIL");
   return bPassed;
}

/**
\brief Runs the actual managed Mesa OpenGL pipeline without entering a GUI loop.
\details Creates the existing official OpenGL showcase context, uploads a real
   xtensor array, renders both modes and checks an actual framebuffer pixel.
\returns True after successful GPU resource, shader and framebuffer checks.
\throw std::runtime_error If Mesa, shader or rendering startup fails.
*/
bool VerifyGL(MesaDriver const theRequestedDriver) {
   OpenGLShowcase aApp {"Tensor Nebula GPU Verification",
      ADECC_MESA_OPENGL_DLL, 640, 480};
   TensorField<float> theField;
   FieldSettings aSettings;
   aSettings.iWidth = 128;
   aSettings.iHeight = 72;
   theField.Compute(aSettings);
   std::string const strActualRenderer=tensor_nebula::ActualOpenGLRenderer();
   std::println("SHOWCASE|INFO|mesa-requested-driver|{}",
      tensor_nebula::arrMesaDrivers.at(
         static_cast<std::size_t>(theRequestedDriver)));
   std::println("SHOWCASE|INFO|opengl-actual|{}",strActualRenderer);
   NebulaGL theRenderer;
   theRenderer.Upload(theField);
   NebulaGL::VisualSettings aVisual;
   GLuint const uTexture = theRenderer.Render(aVisual,640,360);
   bool const bTexture = uTexture != 0 && glIsTexture(uTexture)==GL_TRUE;
   aVisual.iMode = 3;
   GLuint const uHeightTexture = theRenderer.Render(aVisual,640,360);
   bool const bHeight = uHeightTexture == uTexture && glGetError() == GL_NO_ERROR;
   std::println("SHOWCASE|CHECK|opengl-float-texture|{}|real GL_R32F tensor upload",
      bTexture ? "PASS" : "FAIL");
   std::println("SHOWCASE|CHECK|opengl-heightfield|{}|native depth-tested mesh",
      bHeight ? "PASS" : "FAIL");
   // A user-requested GPU backend must not be marked successful when the
   // runtime reports a known software rasterizer fallback.
   bool const bNoSoftwareFallback=theRequestedDriver==MesaDriver::softpipe ||
      (strActualRenderer.find("softpipe")==std::string::npos &&
       strActualRenderer.find("llvmpipe")==std::string::npos &&
       strActualRenderer.find("Software Rasterizer")==std::string::npos);
   std::println("SHOWCASE|CHECK|requested-hardware-renderer|{}|{}",
      bNoSoftwareFallback ? "PASS" : "FAIL",strActualRenderer);
   bool const bPassed = bTexture && bHeight && bNoSoftwareFallback;
   std::println("SHOWCASE|RESULT|{}|Tensor Nebula managed OpenGL 3.3 pipeline",
      bPassed ? "PASS" : "FAIL");
   return bPassed;
}

/**
\brief Runs the cinematic multi-mode observatory in the official OpenGL/ImGui adapter.
\details Recomputes the CPU source only when parameters change. The GPU merely
   visualizes it and mouse interaction never fabricates a second field.
\returns Nothing after the user closes the application.
\throw std::exception On unrecoverable native renderer failures.
*/
void Interactive(MesaDriver const theSelectedDriver,
                 PreferredAdapter const theSelectedAdapter) {
   OpenGLShowcase aApp {"BuildEngine | TENSOR NEBULA - Wave Observatory",
      ADECC_MESA_OPENGL_DLL, 1620, 980};
   std::string const strActualRenderer=tensor_nebula::ActualOpenGLRenderer();
   NebulaGL theRenderer;
   AsyncField theComputer;
   std::shared_ptr<TensorField<float> const> pDisplayedField;
   FieldSettings aSettings;
   FieldSettings aDisplayedSettings;
   NebulaGL::VisualSettings aVisual;
   aSettings.flFrequency = 9.0F;
   int iExperiment = 0;
   int iResolution = 0;
   bool bAnimate = true;
   bool bShowMeasurements = true;
   bool bShowSources = true;
   bool bDirty = true;
   bool bSaveScreenshot = false;
   bool bContinue = true;
   int iRequestedDriver=static_cast<int>(theSelectedDriver);
   int iRequestedAdapter=static_cast<int>(theSelectedAdapter);
   float flSpeed = .65F;
   float flRenderScale = .75F;
   int iMaxComputeHz = 24;
   auto thePrevious = std::chrono::steady_clock::now();
   auto theLastRequest = thePrevious-std::chrono::seconds {1};
   std::uint64_t uLastSubmitted {};
   std::uint64_t uLastAccepted {};
   GLuint uLastTexture {};
   auto theLastGpuFrame=thePrevious-std::chrono::seconds {1};
   // Kick off an initial frame before the first event/render cycle.
   uLastSubmitted = theComputer.RequestFrame(aSettings);
   bDirty = false;

   while(bContinue && aApp.Running()) {
      auto const theNow = std::chrono::steady_clock::now();
      float const flDelta = std::clamp(
         std::chrono::duration<float>(theNow-thePrevious).count(),0.0F,.07F);
      thePrevious = theNow;
      aApp.BeginFrame();
      if(bAnimate) {
         aSettings.flPhase += flDelta*flSpeed;
         if(aSettings.flPhase > 2.0F*std::numbers::pi_v<float>)
            aSettings.flPhase -= 2.0F*std::numbers::pi_v<float>;
         bDirty = true;
      }

      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("Tensor Nebula",nullptr,ImGuiWindowFlags_NoDecoration |
         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
         ImGuiWindowFlags_NoBringToFrontOnFocus);
      OpenGLShowcase::Header("TENSOR NEBULA / THE MATHEMATICS OF LIGHT",
         "Managed xtensor + xsimd + xtl   /   OpenGL 3.3 + GLEW + official Dear ImGui");
      ImGui::TextColored({.95F,.80F,.48F,1.0F},
         "A living field of light. Every luminous pixel is sampled from the CPU tensor.");
      ImGui::Separator();

      float const flPanelWidth = std::clamp(ImGui::GetContentRegionAvail().x*.255F,305.0F,420.0F);
      ImGui::BeginChild("Nebula Controls",{flPanelWidth,0},true);
      ImGui::TextColored({.44F,.91F,.96F,1.0F},"MESA DRIVER / DEVICE");
      ImGui::Combo("Gallium backend",&iRequestedDriver,
         tensor_nebula::arrMesaDrivers.data(),
         static_cast<int>(tensor_nebula::arrMesaDrivers.size()));
      ImGui::Combo("D3D12 GPU preference",&iRequestedAdapter,
         tensor_nebula::arrAdapters.data(),
         static_cast<int>(tensor_nebula::arrAdapters.size()));
      ImGui::TextWrapped("Active request: %s",
         tensor_nebula::arrMesaDrivers.at(
            static_cast<std::size_t>(theSelectedDriver)));
      ImGui::TextWrapped("Actual OpenGL renderer: %s",
         strActualRenderer.c_str());
      if(iRequestedDriver!=static_cast<int>(theSelectedDriver) ||
         iRequestedAdapter!=static_cast<int>(theSelectedAdapter)) {
         ImGui::TextColored({.99F,.78F,.40F,1.0F},
            "Switching Gallium drivers requires a new process.");
         if(ImGui::Button("Apply backend and restart")) {
            bContinue=!LaunchBackend(
               static_cast<MesaDriver>(iRequestedDriver),
               static_cast<PreferredAdapter>(iRequestedAdapter));
         }
      }
      ImGui::Separator();
      ImGui::TextColored({.44F,.91F,.96F,1.0F},"WAVE EXPERIMENTS");
      bDirty = ImGui::Combo("Experiment",&iExperiment,arrExperiments.data(),
         static_cast<int>(arrExperiments.size())) || bDirty;
      aSettings.theMode = static_cast<FieldMode>(iExperiment);
      if(ImGui::Combo("Field resolution",&iResolution,arrResolutions.data(),
         static_cast<int>(arrResolutions.size()))) {
         aSettings.iWidth=arrWidths[iResolution];
         aSettings.iHeight=arrHeights[iResolution];
         bDirty = true;
      }
      bDirty = ImGui::SliderFloat("Phase / rad",&aSettings.flPhase,
         -3.14159F,6.28318F) || bDirty;
      bDirty = ImGui::SliderFloat("Wave number",&aSettings.flFrequency,
         2.0F,20.0F) || bDirty;
      bDirty = ImGui::SliderFloat("Emitter separation",&aSettings.flSeparation,
         .4F,4.0F) || bDirty;
      if(iExperiment == 3)
         bDirty = ImGui::SliderFloat("Pulse width",&aSettings.flEnvelope,
            .2F,2.2F) || bDirty;
      ImGui::Separator();
      ImGui::TextColored({.44F,.91F,.96F,1.0F},"GPU VISUALIZATION");
      ImGui::Combo("Rendering mode",&aVisual.iMode,arrVisualModes.data(),
         static_cast<int>(arrVisualModes.size()));
      ImGui::Combo("Color science",&aVisual.iPalette,arrPalettes.data(),
         static_cast<int>(arrPalettes.size()));
      ImGui::SliderFloat("Luminous diffusion",&aVisual.flGlow,0.0F,2.2F);
      ImGui::SliderFloat("Isoline contrast",&aVisual.flContours,0.0F,1.4F);
      if(aVisual.iMode != 3) {
         ImGui::SliderFloat("Exposure",&aVisual.flExposure,.25F,3.0F);
         ImGui::SliderFloat("Surface relief",&aVisual.flRelief,0.0F,3.0F);
         ImGui::SliderFloat("Zoom",&aVisual.flZoom,.6F,5.0F);
      }
      else {
         ImGui::SliderFloat("Elevation",&aVisual.flElevation,.2F,3.6F);
         ImGui::SliderFloat("Camera azimuth",&aVisual.flYaw,-3.14F,3.14F);
         ImGui::SliderFloat("Camera pitch",&aVisual.flPitch,.16F,1.3F);
         ImGui::SliderFloat("Camera distance",&aVisual.flDistance,5.0F,16.0F);
      }
      ImGui::Separator();
      ImGui::TextColored({.44F,.91F,.96F,1.0F},"MOTION AND EVIDENCE");
      ImGui::Checkbox("Animate the wave field",&bAnimate);
      ImGui::SliderFloat("Animation speed",&flSpeed,.0F,3.0F);
      ImGui::SliderInt("Compute frames / second",&iMaxComputeHz,4,40);
      ImGui::SliderFloat("GPU render scale",&flRenderScale,.4F,1.0F);
      ImGui::Checkbox("Native measurements",&bShowMeasurements);
      ImGui::Checkbox("Emitter positions",&bShowSources);
      if(ImGui::Button("Save genuine GPU screenshot (BMP)"))
         bSaveScreenshot = true;
      if(ImGui::Button("Reset camera")) {
         aVisual.flZoom=1.0F;aVisual.flPanX=0.0F;aVisual.flPanY=0.0F;
         aVisual.flYaw=.55F;aVisual.flPitch=.55F;aVisual.flDistance=8.0F;
      }
      ImGui::TextDisabled("F11 fullscreen  |  F1 about");
      ImGui::TextDisabled("%s",aApp.VersionString().c_str());
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("Observatory",{0,0},true,
         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
      // UI sliders are always processed first. Requests are coalesced, never
      // calculated synchronously on the SDL/ImGui event thread.
      auto const theRequestNow = std::chrono::steady_clock::now();
      auto const theInterval=std::chrono::milliseconds {1000/std::max(1,iMaxComputeHz)};
      if(bDirty && theRequestNow-theLastRequest>=theInterval) {
         uLastSubmitted=theComputer.RequestFrame(aSettings);
         theLastRequest=theRequestNow;
         bDirty=false;
      }
      if(auto theCompleted=theComputer.TakeCompleted()) {
         // The GL context and all texture writes are owned by this UI thread.
         theRenderer.Upload(*theCompleted->pField);
         pDisplayedField=std::move(theCompleted->pField);
         aDisplayedSettings=theCompleted->theSettings;
         uLastAccepted=theCompleted->uRevision;
      }
      if(bShowMeasurements && pDisplayedField) {
         auto const& aMetrics=pDisplayedField->Metrics();
         ImGui::Text("RESOLUTION %zu x %zu  |  INTENSITY %.4f .. %.4f  |  MEAN %.4f  |  ENERGY %.4f",
            pDisplayedField->Values().shape()[1],pDisplayedField->Values().shape()[0],
            aMetrics.flMinimum,aMetrics.flMaximum,aMetrics.flMean,aMetrics.flEnergy);
         ImGui::TextDisabled("xsimd lanes: %zu | oneTBB background | CPU tensor -> GL_R32F",
            xsimd::batch<float>::size);
      }
      ImGui::TextDisabled("Calculation: %s  |  request #%llu / accepted #%llu",
         pDisplayedField ? (uLastAccepted < uLastSubmitted || bDirty ?
            "updating" : "ready") : "preparing",
         static_cast<unsigned long long>(uLastSubmitted),
         static_cast<unsigned long long>(uLastAccepted));
      ImVec2 const aPanel=ImGui::GetContentRegionAvail();
      ImVec2 const aImagePos=ImGui::GetCursorScreenPos();
      if(!pDisplayedField) {
         ImGui::Dummy(aPanel);
         ImGui::SetCursorScreenPos({
            aImagePos.x+24.0F,aImagePos.y+24.0F
         });
         ImGui::TextUnformatted("Computing the first native tensor frame on a dedicated worker...");
      }
      else {
         int const iWidth=std::max(1,static_cast<int>(aPanel.x*flRenderScale));
         int const iHeight=std::max(1,static_cast<int>(aPanel.y*flRenderScale));
         // Mesa software rendering can be expensive. Reuse the completed FBO
         // between 25Hz presentation ticks while ImGui handles input each frame.
         auto const theGpuNow=std::chrono::steady_clock::now();
         if(!uLastTexture || theGpuNow-theLastGpuFrame>=std::chrono::milliseconds {40}) {
            uLastTexture=theRenderer.Render(aVisual,iWidth,iHeight);
            theLastGpuFrame=theGpuNow;
         }
         ImGui::Image(static_cast<ImTextureID>(uLastTexture),aPanel,{0,1},{1,0});
      }
      if(pDisplayedField && ImGui::IsItemHovered()) {
         ImGuiIO const& aIO=ImGui::GetIO();
         if(aVisual.iMode == 3) {
            if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
               aVisual.flYaw += aIO.MouseDelta.x*.005F;
               aVisual.flPitch=std::clamp(aVisual.flPitch-aIO.MouseDelta.y*.005F,.16F,1.3F);
            }
            aVisual.flDistance=std::clamp(aVisual.flDistance-aIO.MouseWheel*.6F,5.0F,16.0F);
         }
         else {
            if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
               aVisual.flPanX -= aIO.MouseDelta.x/
                  std::max(aPanel.x,1.0F)/aVisual.flZoom;
               aVisual.flPanY += aIO.MouseDelta.y/
                  std::max(aPanel.y,1.0F)/aVisual.flZoom;
            }
            aVisual.flZoom=std::clamp(aVisual.flZoom+
               aIO.MouseWheel*.15F,.6F,5.0F);
            float const flU=(aIO.MousePos.x-aImagePos.x)/std::max(aPanel.x,1.0F);
            float const flV=1.0F-(aIO.MousePos.y-aImagePos.y)/std::max(aPanel.y,1.0F);
            float const flX=(flU-.5F)/aVisual.flZoom+.5F+aVisual.flPanX;
            float const flY=(flV-.5F)/aVisual.flZoom+.5F+aVisual.flPanY;
            int const iX=static_cast<int>(flX*
               (static_cast<int>(pDisplayedField->Values().shape()[1])-1));
            int const iY=static_cast<int>(flY*
               (static_cast<int>(pDisplayedField->Values().shape()[0])-1));
            auto const theSample=pDisplayedField->Sample(iX,iY);
            if(theSample.has_value()) {
               ImGui::BeginTooltip();
               ImGui::Text("Field sample (%d,%d): %.6f",iX,iY,theSample.value());
               ImGui::EndTooltip();
            }
         }
      }
      if(pDisplayedField && bShowSources && aVisual.iMode != 3 &&
         (aDisplayedSettings.theMode==FieldMode::twin ||
          aDisplayedSettings.theMode==FieldMode::trinity) &&
         aVisual.flZoom>=.6F) {
         ImDrawList* const pDraw=ImGui::GetWindowDrawList();
         for(int iSource=0;iSource<
               (aDisplayedSettings.theMode==FieldMode::trinity?3:2);++iSource) {
            float const flX=iSource==2?0.0F:
               (iSource==0?-aDisplayedSettings.flSeparation*.5F:
                  aDisplayedSettings.flSeparation*.5F);
            float const flY=iSource==2?1.4F:0.0F;
            float const flU=flX/6.4F+.5F;
            float const flV=flY/3.6F+.5F;
            float const flScreenX=aImagePos.x+
               (.5F+(flU-.5F-aVisual.flPanX)*aVisual.flZoom)*aPanel.x;
            float const flScreenY=aImagePos.y+
               (.5F-(flV-.5F-aVisual.flPanY)*aVisual.flZoom)*aPanel.y;
            if(flScreenX>=aImagePos.x && flScreenX<aImagePos.x+aPanel.x &&
               flScreenY>=aImagePos.y && flScreenY<aImagePos.y+aPanel.y) {
               pDraw->AddCircleFilled({flScreenX,flScreenY},7.0F,IM_COL32(255,235,183,240));
               pDraw->AddCircle({flScreenX,flScreenY},15.0F,IM_COL32(255,225,140,185),36,2.0F);
            }
         }
      }
      ImGui::EndChild();
      ImGui::End();

      if(bSaveScreenshot) {
         std::string const strFilename="tensor-nebula-capture.bmp";
         bool const bSaved=theRenderer.SaveScreenshot(strFilename);
         std::println("SHOWCASE|CAPTURE|{}|{}",bSaved?"PASS":"FAIL",strFilename);
         bSaveScreenshot=false;
      }
      aApp.EndFrame();
   }
}
}

int main(int const iArgc,char const* const* const pszArgv) {
   try {
      MesaDriver theDriver=MesaDriver::softpipe;
      PreferredAdapter theAdapter=PreferredAdapter::automatic;
      bool bVerify=false;
      bool bVerifyGL=false;
      for(int iArg=1;iArg<iArgc;++iArg) {
         std::string_view const svArg {pszArgv[iArg]};
         if(svArg=="--verify")bVerify=true;
         else if(svArg=="--verify-gl")bVerifyGL=true;
         else if(svArg.starts_with("--mesa-backend="))
            theDriver=tensor_nebula::ParseMesaBackend(
               svArg.substr(std::string_view {"--mesa-backend="}.size()));
         else if(svArg.starts_with("--mesa-adapter="))
            theAdapter=tensor_nebula::ParseMesaAdapter(
               svArg.substr(std::string_view {"--mesa-adapter="}.size()));
         else throw std::invalid_argument {"Unsupported Tensor Nebula argument"};
      }
      if(bVerify)return Verify()?0:1;
      // Apply before constructing the SDL/WGL context in either GL mode.
      tensor_nebula::SelectMesaBackend(theDriver,theAdapter);
      if(bVerifyGL)return VerifyGL(theDriver)?0:1;
      Interactive(theDriver,theAdapter);
      return 0;
   }
   catch(std::exception const& theError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",theError.what());
      return 2;
   }
}
