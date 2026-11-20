#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <print>
#include <stdexcept>
#include <string_view>

namespace {
constexpr ma_uint32 uSampleRate = 48000;
constexpr ma_uint32 uSamples = 2048;

ma_waveform_type GetWaveform(int const iMode) {
   switch(iMode) {
      case 1: return ma_waveform_type_square;
      case 2: return ma_waveform_type_triangle;
      case 3: return ma_waveform_type_sawtooth;
      default: return ma_waveform_type_sine;
   }
}

/**
\brief Synthesizes real floating-point PCM frames through miniaudio.
\details The waveform data used for the oscilloscope is produced by the
         managed miniaudio.h implementation, not by a handwritten sine function.
\param iMode Selected oscillator shape.
\param flFrequency Oscillator frequency in Hz.
\param flAmplitude Normalized amplitude.
\returns Exactly 2048 generated mono samples.
\throw std::runtime_error If miniaudio cannot initialize or generate frames.
*/
std::array<float, uSamples> Generate(int const iMode, float const flFrequency,
                                     float const flAmplitude) {
   ma_waveform theWaveform {};
   ma_waveform_config const theConfig = ma_waveform_config_init(
      ma_format_f32, 1, uSampleRate, GetWaveform(iMode), flAmplitude, flFrequency);
   if(ma_waveform_init(&theConfig, &theWaveform) != MA_SUCCESS)
      throw std::runtime_error { "miniaudio waveform init failed" };
   std::array<float, uSamples> arrSamples {};
   ma_uint64 uFramesRead {};
   ma_result const theResult = ma_waveform_read_pcm_frames(
      &theWaveform, arrSamples.data(), uSamples, &uFramesRead);
   ma_waveform_uninit(&theWaveform);
   if(theResult != MA_SUCCESS || uFramesRead != uSamples)
      throw std::runtime_error { "miniaudio synthesis returned incomplete frames" };
   return arrSamples;
}

/**
\brief Owns optional native playback without opening an audio device by default.
\details Every start and stop is synchronous. Waveform state remains alive
         until after the device has stopped its audio callback thread.
*/
class AudioChannel final {
   ma_waveform theWaveform {};
   ma_device theDevice {};
   bool bStarted {};
   bool bInitialized {};
   static void Callback(ma_device* const pDevice, void* const pOutput,
                        void const* const pInput, ma_uint32 const uFrames) {
      (void)pInput;
      ma_waveform* const pWaveform = static_cast<ma_waveform*>(pDevice->pUserData);
      (void)ma_waveform_read_pcm_frames(pWaveform, pOutput, uFrames, nullptr);
   }

public:
   AudioChannel() = default;
   AudioChannel(AudioChannel const&) = delete;
   AudioChannel& operator=(AudioChannel const&) = delete;
   ~AudioChannel() { Stop(); }

   void Stop() noexcept {
      if(bStarted) {
         ma_device_uninit(&theDevice);
         bStarted = false;
      }
      if(bInitialized) {
         ma_waveform_uninit(&theWaveform);
         bInitialized = false;
      }
   }

   bool Start(int const iMode, float const flFrequency, float const flAmplitude) {
      Stop();
      ma_device_config theConfig = ma_device_config_init(ma_device_type_playback);
      theConfig.playback.format = ma_format_f32;
      theConfig.playback.channels = 2;
      theConfig.sampleRate = uSampleRate;
      theConfig.dataCallback = &AudioChannel::Callback;
      theConfig.pUserData = &theWaveform;
      ma_waveform_config const theWaveConfig = ma_waveform_config_init(
         ma_format_f32, 2, uSampleRate, GetWaveform(iMode), flAmplitude, flFrequency);
      if(ma_waveform_init(&theWaveConfig, &theWaveform) != MA_SUCCESS) return false;
      bInitialized = true;
      if(ma_device_init(nullptr, &theConfig, &theDevice) != MA_SUCCESS) {
         Stop();
         return false;
      }
      bStarted = true;
      if(ma_device_start(&theDevice) != MA_SUCCESS) {
         Stop();
         return false;
      }
      return true;
   }

   [[nodiscard]] bool Playing() const noexcept { return bStarted; }
};

void DrawOscilloscope(std::array<float, uSamples> const& arrSamples,
                      ImVec2 const aOrigin, ImVec2 const aSize, float const flGain) {
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, IM_COL32(7, 19, 35, 255), 15.0F);
   for(int i = 0; i <= 8; ++i) {
      float const flY = aOrigin.y + aSize.y * i / 8.0F;
      pDraw->AddLine({aOrigin.x, flY}, {aOrigin.x + aSize.x, flY}, IM_COL32(24, 74, 104, 95));
   }
   for(int i = 0; i <= 16; ++i) {
      float const flX = aOrigin.x + aSize.x * i / 16.0F;
      pDraw->AddLine({flX, aOrigin.y}, {flX, aOrigin.y + aSize.y}, IM_COL32(24, 74, 104, 95));
   }
   pDraw->PushClipRect(aOrigin, {aOrigin.x + aSize.x, aOrigin.y + aSize.y}, true);
   for(int i = 1; i < static_cast<int>(uSamples); ++i) {
      float const flX0 = static_cast<float>(i - 1) / (uSamples - 1) * aSize.x;
      float const flX1 = static_cast<float>(i) / (uSamples - 1) * aSize.x;
      float const flY0 = .5F - .39F * arrSamples[static_cast<std::size_t>(i - 1)] * flGain;
      float const flY1 = .5F - .39F * arrSamples[static_cast<std::size_t>(i)] * flGain;
      ImVec2 const aA {aOrigin.x + flX0, aOrigin.y + flY0 * aSize.y};
      ImVec2 const aB {aOrigin.x + flX1, aOrigin.y + flY1 * aSize.y};
      pDraw->AddLine(aA, aB, IM_COL32(32, 244, 216, 33), 8.0F);
      pDraw->AddLine(aA, aB, IM_COL32(80, 248, 234, 190), 2.0F);
   }
   pDraw->PopClipRect();
}

bool Verify() {
   auto const arrSine = Generate(0, 440.0F, .7F);
   auto const arrSquare = Generate(1, 440.0F, .7F);
   bool const bNonzero = std::any_of(arrSine.begin(), arrSine.end(),
      [](float const flValue) { return std::abs(flValue) > .1F; });
   bool const bFinite = std::all_of(arrSine.begin(), arrSine.end(),
      [](float const flValue) { return std::isfinite(flValue) && std::abs(flValue) <= 1.001F; });
   bool const bDifferent = !std::equal(arrSine.begin(), arrSine.end(), arrSquare.begin());
   bool const bPassed = bNonzero && bFinite && bDifferent;
   std::println("SHOWCASE|CHECK|waveforms|{}|miniaudio PCM finite={} nonzero={} distinct={}",
      bPassed ? "PASS" : "FAIL", bFinite, bNonzero, bDifferent);
   std::println("SHOWCASE|RESULT|{}|Native miniaudio waveform synthesis", bPassed ? "PASS" : "FAIL");
   return bPassed;
}

void Interactive() {
   NativeShowcase aApp { "BuildEngine | PULSE ENGINE - The Shape of Sound", 1510, 900 };
   AudioChannel theAudio;
   constexpr std::array<char const*, 4> arrModes {
      "Sine", "Square", "Triangle", "Sawtooth"
   };
   int iMode = 0;
   float flFrequency = 440.0F, flAmplitude = .45F, flGain = 1.0F;
   bool bAudioUnavailable = false;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0.0F, 0.0F});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("PULSE ENGINE", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("PULSE ENGINE / THE SHAPE OF SOUND",
                             "True miniaudio PCM synthesis  /  48 kHz  /  live native oscilloscope");
      bool bChanged = ImGui::Combo("Waveform", &iMode, arrModes.data(), static_cast<int>(arrModes.size()));
      bChanged = ImGui::SliderFloat("Frequency (Hz)", &flFrequency, 60.0F, 1400.0F) || bChanged;
      bChanged = ImGui::SliderFloat("Amplitude", &flAmplitude, .05F, .85F) || bChanged;
      ImGui::SliderFloat("Display gain", &flGain, .5F, 2.3F);
      if(theAudio.Playing() && bChanged)
         bAudioUnavailable = !theAudio.Start(iMode, flFrequency, flAmplitude);
      if(ImGui::Button(theAudio.Playing() ? "MUTE / CLOSE DEVICE" : "PLAY ON SPEAKERS")) {
         if(theAudio.Playing()) theAudio.Stop();
         else bAudioUnavailable = !theAudio.Start(iMode, flFrequency, flAmplitude);
      }
      ImGui::SameLine();
      ImGui::TextColored({.44F, .94F, .79F, 1.0F},
          "%s", theAudio.Playing() ? "AUDIO LIVE" : "OFFLINE / VISUAL ONLY");
      if(bAudioUnavailable)
         ImGui::TextColored({1.0F, .48F, .52F, 1.0F},
             "Audio device unavailable. The oscillator remains accessible offline.");
      auto const arrSamples = Generate(iMode, flFrequency, flAmplitude);
      float flPeak = 0.0F, flEnergy = 0.0F;
      for(float const flSample : arrSamples) {
         flPeak = std::max(flPeak, std::abs(flSample));
         flEnergy += flSample * flSample;
      }
      ImGui::Text("FREQUENCY %.0f Hz   PEAK %.3f   RMS %.3f   2048 native PCM samples",
         flFrequency, flPeak, std::sqrt(flEnergy / uSamples));
      ImVec2 const aOrigin = ImGui::GetCursorScreenPos();
      ImVec2 const aSize = ImGui::GetContentRegionAvail();
      DrawOscilloscope(arrSamples, aOrigin,
                      {aSize.x, std::max(310.0F, aSize.y - 10.0F)}, flGain);
      ImGui::End();
      aApp.EndFrame();
   }
}
}

int main(int const iArgc, char const* const* const pszArgv) {
   try {
      if(iArgc > 1 && std::string_view { pszArgv[1] } == "--verify")
         return Verify() ? 0 : 1;
      Interactive();
      return 0;
   }
   catch(std::exception const& theException) {
      std::println(stderr, "SHOWCASE|ERROR|{}", theException.what());
      return 2;
   }
}
