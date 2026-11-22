#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <print>
#include <string>
#include <string_view>
namespace {
constexpr int iRate=48000;
constexpr std::size_t uSamples=48000;
constexpr double flPi=3.14159265358979323846;
/**
\brief Owns a real OpenAL device, context, source and PCM buffer.
\details Audio starts only on explicit request. RAII releases the context, source, buffer and device.
\throw std::runtime_error When native audio resources cannot be initialized.
*/
class AudioScene final {
   ALCdevice* pDevice {};
   ALCcontext* pContext {};
   ALuint uSource {};
   ALuint uBuffer {};
   bool bPlaying {};
public:
   AudioScene()=default;
   AudioScene(AudioScene const&)=delete;
   AudioScene& operator=(AudioScene const&)=delete;
   ~AudioScene(){Stop();}
   void Stop() noexcept {
      if(pContext) {
         alcMakeContextCurrent(pContext);
         if(uSource) {alSourceStop(uSource);alDeleteSources(1,&uSource);}
         if(uBuffer) alDeleteBuffers(1,&uBuffer);
         alcMakeContextCurrent(nullptr);
         alcDestroyContext(pContext);
      }
      if(pDevice)alcCloseDevice(pDevice);
      pContext=nullptr;pDevice=nullptr;uSource=0;uBuffer=0;bPlaying=false;
   }
   bool Start(float const flFrequency) {
      Stop();
      pDevice=alcOpenDevice(nullptr);
      if(!pDevice)return false;
      pContext=alcCreateContext(pDevice,nullptr);
      if(!pContext||alcMakeContextCurrent(pContext)!=ALC_TRUE) {Stop();return false;}
      std::array<std::int16_t,uSamples> arrAudio {};
      for(std::size_t uI=0;uI<arrAudio.size();++uI) {
         double const flT=double(uI)/iRate;
         double const flEnvelope=.32*(1-std::exp(-flT*12.0));
         arrAudio[uI]=static_cast<std::int16_t>(std::sin(2*flPi*flFrequency*flT)*flEnvelope*32767);
      }
      alGenBuffers(1,&uBuffer);
      alBufferData(uBuffer,AL_FORMAT_MONO16,arrAudio.data(),
         static_cast<ALsizei>(arrAudio.size()*sizeof(std::int16_t)),iRate);
      alGenSources(1,&uSource);
      if(alGetError()!=AL_NO_ERROR) {Stop();return false;}
      alSourcei(uSource,AL_BUFFER,static_cast<ALint>(uBuffer));
      alSourcei(uSource,AL_LOOPING,AL_TRUE);
      alSourcef(uSource,AL_GAIN,.45F);
      alSourcePlay(uSource);
      if(alGetError()!=AL_NO_ERROR) {Stop();return false;}
      bPlaying=true;return true;
   }
   void Position(float const flX,float const flZ) {
      if(bPlaying)alSource3f(uSource,AL_POSITION,flX,0.0F,flZ);
   }
   bool Playing() const noexcept{return bPlaying;}
};
bool Verify() {
   ALCenum const iMajor=alcGetEnumValue(nullptr,"ALC_MAJOR_VERSION");
   bool const bPassed=iMajor==ALC_MAJOR_VERSION;
   std::println("SHOWCASE|CHECK|openal-enum|{}|Device-free native OpenAL API",bPassed?"PASS":"FAIL");
   std::println("SHOWCASE|RESULT|{}|Acoustic Orbitarium",bPassed?"PASS":"FAIL");
   return bPassed;
}
void Interactive() {
   NativeShowcase aApp {"BuildEngine | ACOUSTIC ORBITARIUM - Sound in Space",1500,900};
   AudioScene theSound;
   float flFrequency=220.0F,flAzimuth=0.0F,flRadius=2.0F;
   bool bAudioFailure=false;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("ORBITARIUM",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("ACOUSTIC ORBITARIUM / SOUND IN SPACE","Native OpenAL Soft 1.25.2 / moving 3D sound source");
      ImGui::SliderFloat("Frequency Hz",&flFrequency,80.0F,1100.0F);
      ImGui::SliderFloat("Azimuth radians",&flAzimuth,-3.14159F,3.14159F);
      ImGui::SliderFloat("Orbit radius",&flRadius,.4F,5.0F);
      if(ImGui::Button(theSound.Playing()?"STOP AND CLOSE AUDIO DEVICE":"OPEN DEVICE / PLAY SPATIAL SOUND")) {
         if(theSound.Playing())theSound.Stop();
         else bAudioFailure=!theSound.Start(flFrequency);
      }
      if(bAudioFailure)ImGui::Text("No audio device available. Visual mode remains active.");
      float const flX=flRadius*std::cos(flAzimuth),flZ=flRadius*std::sin(flAzimuth);
      theSound.Position(flX,flZ);
      ImGui::Text("Source: X %.2f  Y 0.00  Z %.2f  |  Listener origin",flX,flZ);
      ImVec2 const aOrigin=ImGui::GetCursorScreenPos(),aSize=ImGui::GetContentRegionAvail();
      ImVec2 const aMid{aOrigin.x+aSize.x*.5F,aOrigin.y+aSize.y*.53F};
      float const flOrbit=std::min(aSize.x,aSize.y)*.37F;
      ImDrawList* const pDraw=ImGui::GetWindowDrawList();
      for(int i=1;i<=5;++i)
         pDraw->AddCircle(aMid,flOrbit*i/5,IM_COL32(30,150,190,100),180,1.5F);
      for(int i=0;i<24;++i) {
         float const flAngle=2.0F*float(flPi)*i/24;
         ImVec2 const aEnd{aMid.x+std::cos(flAngle)*flOrbit,aMid.y+std::sin(flAngle)*flOrbit};
         pDraw->AddLine(aMid,aEnd,IM_COL32(35,105,140,80),1.0F);
      }
      ImVec2 const aEmitter{aMid.x+std::cos(flAzimuth)*flOrbit*flRadius/5,
                            aMid.y+std::sin(flAzimuth)*flOrbit*flRadius/5};
      pDraw->AddCircleFilled(aMid,18,IM_COL32(80,235,245,255));
      pDraw->AddCircleFilled(aEmitter,19,IM_COL32(255,151,95,255));
      pDraw->AddLine(aMid,aEmitter,IM_COL32(255,193,102,220),3.0F);
      for(int i=1;i<=4;++i)pDraw->AddCircle(aEmitter,23.0F+i*19.0F,IM_COL32(255,124,190,100-i*16),80,2.0F);
      ImGui::End();aApp.EndFrame();
   }
}
}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1&&std::string_view {pszArgv[1]}=="--verify")return Verify()?0:1;
      Interactive();return 0;
   }
   catch(std::exception const& theError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",theError.what());return 2;
   }
}
