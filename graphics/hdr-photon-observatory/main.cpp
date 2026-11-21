#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#include <ceres/ceres.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>
#include <Imath/ImathBox.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int iImageWidth = 320;
constexpr int iImageHeight = 192;
constexpr int iSamples = 48;

struct PhotonModel {
   double flAmplitude { 7.5 };
   double flPosition { 0.59 };
   double flWidth { 0.105 };
};

struct PhotonResidual {
   double flX {};
   double flObservation {};

   template<typename T>
   bool operator()(T const* const pParameter, T* const pResidual) const {
      using std::exp;
      T const flDistance = (T(flX) - pParameter[1]) / pParameter[2];
      pResidual[0] = pParameter[0] * exp(T(-0.5) * flDistance * flDistance) +
                     T(0.04) - T(flObservation);
      return true;
   }
};

struct Calibration {
   PhotonModel theModel;
   double flInitialCost {};
   double flFinalCost {};
   int iIterations {};
   bool bUsable {};
};

[[nodiscard]] double Radiance(double const flX, double const flY,
                              PhotonModel const& theModel) {
   double const flDx = (flX - theModel.flPosition) / theModel.flWidth;
   double const flDy = (flY - 0.52) / (theModel.flWidth * 1.7);
   double const flHalo = std::exp(-0.5 * (flDx * flDx + flDy * flDy));
   double const flDiffraction = 0.15 * std::exp(-std::abs(flY - 0.52) * 28.0) *
      std::exp(-std::abs(flDx) * 0.8);
   return 0.04 + theModel.flAmplitude * (flHalo + flDiffraction);
}

[[nodiscard]] std::vector<Imf::Rgba> GenerateHDR(PhotonModel const& theModel) {
   std::vector<Imf::Rgba> vecImage(
      static_cast<std::size_t>(iImageWidth) * iImageHeight);
   for(int iY = 0; iY < iImageHeight; ++iY) {
      for(int iX = 0; iX < iImageWidth; ++iX) {
         double const flX = static_cast<double>(iX) / (iImageWidth - 1);
         double const flY = static_cast<double>(iY) / (iImageHeight - 1);
         double const flCore = Radiance(flX, flY, theModel);
         double const flNebula = 0.14 *
            (1.0 + std::sin(flX * 11.0 + flY * 7.0)) *
            std::exp(-std::pow((flY - 0.5) * 3.5, 2.0));
         vecImage[static_cast<std::size_t>(iY) * iImageWidth + iX] = Imf::Rgba {
            static_cast<float>(flCore + flNebula * 0.35),
            static_cast<float>(flCore * 0.51 + flNebula * 0.61),
            static_cast<float>(flCore * 1.27 + flNebula),
            1.0F
         };
      }
   }
   return vecImage;
}

[[nodiscard]] std::array<double, iSamples> SensorSamples(PhotonModel const& theModel) {
   std::array<double, iSamples> arrObservations {};
   for(int i = 0; i < iSamples; ++i) {
      double const flX = static_cast<double>(i) / (iSamples - 1);
      double const flDistance = (flX - theModel.flPosition) / theModel.flWidth;
      arrObservations[static_cast<std::size_t>(i)] =
         theModel.flAmplitude * std::exp(-0.5 * flDistance * flDistance) + 0.04;
   }
   return arrObservations;
}

[[nodiscard]] Calibration Calibrate(
   std::array<double, iSamples> const& arrObservations,
   PhotonModel const& theInitial) {
   std::array<double, 3> arrParameters {
      theInitial.flAmplitude, theInitial.flPosition, theInitial.flWidth
   };
   ceres::Problem theProblem;
   for(int i = 0; i < iSamples; ++i) {
      double const flX = static_cast<double>(i) / (iSamples - 1);
      theProblem.AddResidualBlock(
         new ceres::AutoDiffCostFunction<PhotonResidual, 1, 3>(
            new PhotonResidual {flX, arrObservations[static_cast<std::size_t>(i)]}),
         nullptr, arrParameters.data());
   }
   theProblem.SetParameterLowerBound(arrParameters.data(), 0, 0.05);
   theProblem.SetParameterLowerBound(arrParameters.data(), 2, 0.02);
   theProblem.SetParameterUpperBound(arrParameters.data(), 2, 0.55);
   ceres::Solver::Options theOptions;
   theOptions.max_num_iterations = 80;
   theOptions.minimizer_progress_to_stdout = false;
   theOptions.linear_solver_type = ceres::DENSE_QR;
   ceres::Solver::Summary theSummary;
   ceres::Solve(theOptions, &theProblem, &theSummary);
   return Calibration {
      .theModel = {arrParameters[0], arrParameters[1], arrParameters[2]},
      .flInitialCost = theSummary.initial_cost,
      .flFinalCost = theSummary.final_cost,
      .iIterations = static_cast<int>(theSummary.iterations.size()),
      .bUsable = theSummary.IsSolutionUsable()
   };
}

[[nodiscard]] std::filesystem::path HDRPath() {
   return std::filesystem::temp_directory_path() /
      "buildengine-hdr-photon-observatory.exr";
}

[[nodiscard]] std::vector<Imf::Rgba> ReadHDR(std::filesystem::path const& thePath) {
   Imf::RgbaInputFile theInput(thePath.string().c_str());
   Imath::Box2i const aWindow = theInput.dataWindow();
   if(aWindow.min.x != 0 || aWindow.min.y != 0 ||
      aWindow.max.x != iImageWidth - 1 ||
      aWindow.max.y != iImageHeight - 1)
      throw std::runtime_error {"OpenEXR data window does not match the HDR fixture"};
   std::vector<Imf::Rgba> vecImage(
      static_cast<std::size_t>(iImageWidth) * iImageHeight);
   theInput.setFrameBuffer(vecImage.data(), 1, iImageWidth);
   theInput.readPixels(0, iImageHeight - 1);
   return vecImage;
}

void WriteHDR(std::filesystem::path const& thePath,
              std::vector<Imf::Rgba> const& vecImage) {
   if(vecImage.size() != static_cast<std::size_t>(iImageWidth) * iImageHeight)
      throw std::runtime_error {"Invalid HDR raster dimensions"};
   Imf::RgbaOutputFile theOutput(
      thePath.string().c_str(), iImageWidth, iImageHeight, Imf::WRITE_RGBA);
   theOutput.setFrameBuffer(vecImage.data(), 1, iImageWidth);
   theOutput.writePixels(iImageHeight);
}

[[nodiscard]] double RoundtripError(
   std::vector<Imf::Rgba> const& vecOriginal,
   std::vector<Imf::Rgba> const& vecReloaded) {
   if(vecOriginal.size() != vecReloaded.size())
      throw std::runtime_error {"OpenEXR round-trip pixel count mismatch"};
   double flMaximum {};
   for(std::size_t u = 0; u < vecOriginal.size(); ++u) {
      flMaximum = std::max(flMaximum,
         std::abs(static_cast<double>(vecOriginal[u].r) -
                  static_cast<double>(vecReloaded[u].r)));
      flMaximum = std::max(flMaximum,
         std::abs(static_cast<double>(vecOriginal[u].g) -
                  static_cast<double>(vecReloaded[u].g)));
      flMaximum = std::max(flMaximum,
         std::abs(static_cast<double>(vecOriginal[u].b) -
                  static_cast<double>(vecReloaded[u].b)));
   }
   return flMaximum;
}

[[nodiscard]] bool Verify() {
   PhotonModel const theTruth {};
   std::array<double, iSamples> const arrSensor = SensorSamples(theTruth);
   Calibration const theFit = Calibrate(arrSensor, {4.2, 0.44, 0.23});
   bool const bFit = theFit.bUsable &&
      std::abs(theFit.theModel.flAmplitude - theTruth.flAmplitude) < 0.02 &&
      std::abs(theFit.theModel.flPosition - theTruth.flPosition) < 0.002 &&
      std::abs(theFit.theModel.flWidth - theTruth.flWidth) < 0.002 &&
      theFit.flFinalCost < theFit.flInitialCost * 0.01;
   std::println("SHOWCASE|CHECK|ceres-calibration|{}|initial={:.8f} final={:.8f} iterations={} position={:.5f}",
      bFit ? "PASS" : "FAIL", theFit.flInitialCost,
      theFit.flFinalCost, theFit.iIterations, theFit.theModel.flPosition);

   auto const vecOriginal = GenerateHDR(theTruth);
   std::filesystem::path const thePath = HDRPath();
   WriteHDR(thePath, vecOriginal);
   auto const vecReloaded = ReadHDR(thePath);
   double const flError = RoundtripError(vecOriginal, vecReloaded);
   bool const bEXR = flError <= 0.015 && flError >= 0.0;
   bool const bHDR = static_cast<double>(
      vecReloaded[static_cast<std::size_t>(iImageHeight / 2) *
                  iImageWidth + iImageWidth / 2].b) > 1.0;
   std::println("SHOWCASE|CHECK|openexr-roundtrip|{}|max-channel-error={:.8f} pixels={}",
      bEXR ? "PASS" : "FAIL", flError, vecReloaded.size());
   std::println("SHOWCASE|CHECK|hdr-dynamic-range|{}|linear-radiance-above-one={}",
      bHDR ? "PASS" : "FAIL", bHDR);
   std::println("SHOWCASE|RESULT|{}|Ceres nonlinear fit and OpenEXR FP16 HDR round-trip",
      bFit && bEXR && bHDR ? "PASS" : "FAIL");
   return bFit && bEXR && bHDR;
}

void DrawHDR(std::vector<Imf::Rgba> const& vecImage,
             float const flExposure, ImVec2 const aSize) {
   ImVec2 const aTop = ImGui::GetCursorScreenPos();
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   constexpr int iTilesX = 100;
   constexpr int iTilesY = 60;
   for(int iY = 0; iY < iTilesY; ++iY) {
      for(int iX = 0; iX < iTilesX; ++iX) {
         std::size_t const uIndex =
            static_cast<std::size_t>((iY * iImageHeight / iTilesY) * iImageWidth +
                                    (iX * iImageWidth / iTilesX));
         auto const& aPixel = vecImage[uIndex];
         auto const Convert = [flExposure](float const flValue) -> int {
            return std::clamp(static_cast<int>(
               255.0 * (1.0 - std::exp(-static_cast<double>(flValue) * flExposure))),
               0, 255);
         };
         ImU32 const uColor = IM_COL32(Convert(aPixel.r), Convert(aPixel.g),
                                      Convert(aPixel.b), 255);
         ImVec2 const aMin {aTop.x + aSize.x * iX / iTilesX,
                            aTop.y + aSize.y * iY / iTilesY};
         ImVec2 const aMax {aTop.x + aSize.x * (iX + 1) / iTilesX + 0.5F,
                            aTop.y + aSize.y * (iY + 1) / iTilesY + 0.5F};
         pDraw->AddRectFilled(aMin, aMax, uColor);
      }
   }
   pDraw->AddRect(aTop, {aTop.x + aSize.x, aTop.y + aSize.y},
                  IM_COL32(113, 208, 238, 255), 8.0F, 0, 2.0F);
   ImGui::Dummy(aSize);
}

void DrawCalibration(std::array<double, iSamples> const& arrSamples,
                     PhotonModel const& theModel, ImVec2 const aSize) {
   ImVec2 const aTop = ImGui::GetCursorScreenPos();
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aTop, {aTop.x + aSize.x, aTop.y + aSize.y},
                        IM_COL32(11, 22, 44, 255), 10.0F);
   auto const Point = [&](double const flX, double const flY) -> ImVec2 {
      return {aTop.x + 20.0F + (aSize.x - 40.0F) * static_cast<float>(flX),
              aTop.y + aSize.y - 20.0F -
                 (aSize.y - 40.0F) *
                    static_cast<float>(std::clamp(flY / 10.0, 0.0, 1.0))};
   };
   for(int i = 0; i < iSamples; ++i) {
      double const flX = static_cast<double>(i) / (iSamples - 1);
      pDraw->AddCircleFilled(Point(flX, arrSamples[static_cast<std::size_t>(i)]),
                             3.1F, IM_COL32(106, 236, 243, 255));
   }
   for(int i = 0; i < 240; ++i) {
      double const flA = static_cast<double>(i) / 240.0;
      double const flB = static_cast<double>(i + 1) / 240.0;
      auto const Y = [&](double flX) {
         double const flD = (flX - theModel.flPosition) / theModel.flWidth;
         return theModel.flAmplitude * std::exp(-0.5 * flD * flD) + 0.04;
      };
      pDraw->AddLine(Point(flA, Y(flA)), Point(flB, Y(flB)),
                     IM_COL32(255, 186, 104, 255), 2.4F);
   }
   ImGui::Dummy(aSize);
}

void Interactive() {
   NativeShowcase aApp {"BuildEngine | HDR Photon Observatory", 1480, 930};
   PhotonModel theTruth {};
   PhotonModel theGuess {4.2, 0.44, 0.23};
   std::array<double, iSamples> arrSensor = SensorSamples(theTruth);
   std::vector<Imf::Rgba> vecHDR = GenerateHDR(theTruth);
   Calibration theCalibration {};
   float flExposure = 0.37F;
   float flRadiance = 7.5F;
   float flPosition = 0.59F;
   float flWidth = 0.105F;
   double flRoundtripError {};
   std::string strStatus = "Ready: synthesize a new HDR field or calibrate the Ceres model.";
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGuiIO const& aIO = ImGui::GetIO();
      ImGui::SetNextWindowPos({0, 0});
      ImGui::SetNextWindowSize(aIO.DisplaySize);
      ImGui::Begin("HDR Observatory", nullptr,
         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("HDR PHOTON OBSERVATORY",
         "A physical light field  /  OpenEXR FP16  /  Ceres nonlinear calibration  /  Native BCC64X");
      ImGui::SeparatorText("1. Photon field and sensor settings");
      ImGui::SliderFloat("Peak radiance", &flRadiance, 1.0F, 14.0F, "%.2f");
      ImGui::SliderFloat("Light position", &flPosition, 0.2F, 0.82F, "%.3f");
      ImGui::SliderFloat("Diffusion width", &flWidth, 0.035F, 0.26F, "%.3f");
      ImGui::SliderFloat("Display exposure", &flExposure, 0.04F, 1.3F, "%.3f");
      if(ImGui::Button("SYNTHESIZE HDR FIELD")) {
         theTruth = {flRadiance, flPosition, flWidth};
         arrSensor = SensorSamples(theTruth);
         vecHDR = GenerateHDR(theTruth);
         theGuess = {4.2, 0.44, 0.23};
         theCalibration = {};
         strStatus = "A new scene was generated from real floating-point radiance.";
      }
      ImGui::SameLine();
      if(ImGui::Button("FIT WITH CERES")) {
         theCalibration = Calibrate(arrSensor, theGuess);
         theGuess = theCalibration.theModel;
         strStatus = std::format("Ceres: {} | iterations={} | final cost={:.8f}",
            theCalibration.bUsable ? "solution usable" : "fit failed",
            theCalibration.iIterations, theCalibration.flFinalCost);
      }
      ImGui::SameLine();
      if(ImGui::Button("EXR WRITE / READ")) {
         try {
            std::filesystem::path const thePath = HDRPath();
            WriteHDR(thePath, vecHDR);
            auto const vecRead = ReadHDR(thePath);
            flRoundtripError = RoundtripError(vecHDR, vecRead);
            vecHDR = vecRead;
            strStatus = std::format("OpenEXR read/write completed: maximum channel difference={:.7f}",
                                    flRoundtripError);
         }
         catch(std::exception const& aError) {
            strStatus = std::format("OpenEXR failure: {}", aError.what());
         }
      }
      ImGui::TextWrapped("%s", strStatus.c_str());
      ImGui::SeparatorText("2. Floating-point HDR field / live tone mapping");
      float const flWidthAvail = std::max(500.0F, ImGui::GetContentRegionAvail().x);
      DrawHDR(vecHDR, flExposure, {flWidthAvail, 330.0F});
      ImGui::TextDisabled("Preview is an explicit tone-map of OpenEXR-compatible floating-point pixels; values above 1.0 are retained.");
      ImGui::SeparatorText("3. Ceres inverse problem / observed sensor versus model");
      DrawCalibration(arrSensor, theGuess, {flWidthAvail, 220.0F});
      ImGui::TextColored({0.46F, 0.92F, 0.88F, 1.0F},
         "Fit: amplitude %.4f  |  position %.4f  |  width %.4f",
         theGuess.flAmplitude, theGuess.flPosition, theGuess.flWidth);
      ImGui::TextDisabled("Cyan = measured samples  /  Gold = nonlinear model  /  F11 = fullscreen");
      ImGui::End();
      aApp.EndFrame();
   }
}

} // namespace

int main(int const iArgc, char const* const* const pszArgv) {
   try {
      if(iArgc > 1 && std::string_view {pszArgv[1]} == "--verify")
         return Verify() ? 0 : 1;
      Interactive();
      return 0;
   }
   catch(std::exception const& aError) {
      std::println(stderr, "SHOWCASE|ERROR|{}", aError.what());
      return 2;
   }
}
