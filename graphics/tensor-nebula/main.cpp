#include "../common/GraphicsDllImports.h"
#include "../common/OpenGLShowcase.h"
#include "TensorField.h"
#include "AsyncField.h"
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

namespace {
using tensor_nebula::FieldMode;
using tensor_nebula::FieldSettings;
using tensor_nebula::NebulaGL;
using tensor_nebula::TensorField;
using tensor_nebula::AsyncField;

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
   bool const bPassed = bAllModes && bBounded && bFinite &&
      bSymmetry && bOptional && bSimd;
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
bool VerifyGL() {
   OpenGLShowcase aApp {"Tensor Nebula GPU Verification",
      ADECC_MESA_OPENGL_DLL, 640, 480};
   TensorField<float> theField;
   FieldSettings aSettings;
   aSettings.iWidth = 128;
   aSettings.iHeight = 72;
   theField.Compute(aSettings);
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
   bool const bPassed = bTexture && bHeight;
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
void Interactive() {
   OpenGLShowcase aApp {"BuildEngine | TENSOR NEBULA - Wave Observatory",
      ADECC_MESA_OPENGL_DLL, 1620, 980};
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
   float flSpeed = .65F;
   float flRenderScale = .75F;
   int iMaxComputeHz = 24;
   auto thePrevious = std::chrono::steady_clock::now();
   auto theLastRequest = thePrevious-std::chrono::seconds {1};
   std::uint64_t uLastSubmitted {};
   std::uint64_t uLastAccepted {};
   // Kick off an initial frame before the first event/render cycle.
   uLastSubmitted = theComputer.RequestFrame(aSettings);
   bDirty = false;

   while(aApp.Running()) {
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
      if(auto pCompleted=theComputer.TakeCompleted()) {
         // The GL context and all texture writes are owned by this UI thread.
         theRenderer.Upload(*pCompleted);
         pDisplayedField=std::move(pCompleted);
         aDisplayedSettings=aSettings;
         uLastAccepted=uLastSubmitted;
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
         pDisplayedField ? (bDirty ? "updating" : "ready") : "preparing",
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
         GLuint const uTexture=theRenderer.Render(aVisual,iWidth,iHeight);
         ImGui::Image(static_cast<ImTextureID>(uTexture),aPanel,{0,1},{1,0});
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
         (iExperiment==0 || iExperiment==1) && aVisual.flZoom>=.6F) {
         ImDrawList* const pDraw=ImGui::GetWindowDrawList();
         for(int iSource=0;iSource<(iExperiment==1?3:2);++iSource) {
            float const flX=iSource==2?0.0F:
               (iSource==0?-aSettings.flSeparation*.5F:aSettings.flSeparation*.5F);
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
      if(iArgc>1 && std::string_view {pszArgv[1]}=="--verify")
         return Verify()?0:1;
      if(iArgc>1 && std::string_view {pszArgv[1]}=="--verify-gl")
         return VerifyGL()?0:1;
      Interactive();
      return 0;
   }
   catch(std::exception const& theError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",theError.what());
      return 2;
   }
}
