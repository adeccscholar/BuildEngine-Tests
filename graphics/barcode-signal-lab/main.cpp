#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/ImageView.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr int iModules = 95;
constexpr int iModulePixels = 6;
constexpr int iQuietModules = 18;
constexpr int iHeight = 272;
constexpr int iWidth = (iModules + iQuietModules * 2) * iModulePixels;

/**
\brief Constructs a deterministic retail barcode using the public EAN-13 bit layout.
\details The encoder is deliberately independent of ZXing; the actual verification
         roundtrip uses ZXing's native installed DLL decoder.
\param svDigits Twelve decimal data digits.
\returns A monochrome image of the full generated EAN-13 symbol.
\throw std::invalid_argument If the input is not twelve ASCII digits.
*/
std::vector<std::uint8_t> Generate(std::string_view const svDigits) {
   if(svDigits.size() != 12 ||
      !std::all_of(svDigits.begin(), svDigits.end(), [](char c) { return c >= '0' && c <= '9'; }))
      throw std::invalid_argument { "EAN-13 requires exactly 12 digits" };

   constexpr std::array<std::string_view, 10> arrL {
      "0001101","0011001","0010011","0111101","0100011",
      "0110001","0101111","0111011","0110111","0001011"
   };
   constexpr std::array<std::string_view, 10> arrG {
      "0100111","0110011","0011011","0100001","0011101",
      "0111001","0000101","0010001","0001001","0010111"
   };
   constexpr std::array<std::string_view, 10> arrR {
      "1110010","1100110","1101100","1000010","1011100",
      "1001110","1010000","1000100","1001000","1110100"
   };
   constexpr std::array<std::string_view, 10> arrParity {
      "LLLLLL","LLGLGG","LLGGLG","LLGGGL","LGLLGG",
      "LGGLLG","LGGGLL","LGLGLG","LGLGGL","LGGLGL"
   };

   int iChecksum = 0;
   for(std::size_t u = 0; u < 12; ++u)
      iChecksum += (svDigits[u] - '0') * (u % 2 == 0 ? 1 : 3);
   iChecksum = (10 - iChecksum % 10) % 10;

   std::string strBits { "101" };
   int const iFirst = svDigits[0] - '0';
   for(int i = 0; i < 6; ++i) {
      int const iDigit = svDigits[static_cast<std::size_t>(i + 1)] - '0';
      strBits += arrParity[static_cast<std::size_t>(iFirst)][static_cast<std::size_t>(i)] == 'L'
         ? arrL[static_cast<std::size_t>(iDigit)] : arrG[static_cast<std::size_t>(iDigit)];
   }
   strBits += "01010";
   for(int i = 0; i < 5; ++i)
      strBits += arrR[static_cast<std::size_t>(svDigits[static_cast<std::size_t>(i + 7)] - '0')];
   strBits += arrR[static_cast<std::size_t>(iChecksum)];
   strBits += "101";
   if(strBits.size() != iModules) throw std::runtime_error { "EAN-13 module count mismatch" };

   std::vector<std::uint8_t> vecPixels(static_cast<std::size_t>(iWidth * iHeight), 255);
   for(int iX = 0; iX < iModules; ++iX) {
      if(strBits[static_cast<std::size_t>(iX)] != '1') continue;
      int const iStart = (iQuietModules + iX) * iModulePixels;
      for(int iY = 22; iY < iHeight - 22; ++iY)
         for(int iPixel = 0; iPixel < iModulePixels; ++iPixel)
            vecPixels[static_cast<std::size_t>(iY * iWidth + iStart + iPixel)] = 0;
   }
   return vecPixels;
}

std::string FullDigits(std::string_view const svData) {
   int iSum = 0;
   for(std::size_t u = 0; u < 12; ++u)
      iSum += (svData[u] - '0') * (u % 2 == 0 ? 1 : 3);
   return std::string { svData } + static_cast<char>('0' + (10 - iSum % 10) % 10);
}

ZXing::Barcodes Decode(std::vector<std::uint8_t> const& vecPixels, bool const bTryHarder) {
   ZXing::ReaderOptions theOptions;
   theOptions.formats(ZXing::BarcodeFormat::EAN13);
   theOptions.tryHarder(bTryHarder);
   theOptions.tryRotate(false);
   theOptions.tryInvert(false);
   ZXing::ImageView const theView { vecPixels.data(), iWidth, iHeight, ZXing::ImageFormat::Lum };
   return ZXing::ReadBarcodes(theView, theOptions);
}

bool Verify() {
   std::string_view const svDigits = "590123412345";
   auto const vecPixels = Generate(svDigits);
   ZXing::Barcodes const vecCodes = Decode(vecPixels, true);
   bool const bPassed = std::any_of(vecCodes.begin(), vecCodes.end(),
      [svDigits](ZXing::Barcode const& theCode) {
         return theCode.isValid() && theCode.text() == FullDigits(svDigits);
      });
   std::println("SHOWCASE|CHECK|ean13-roundtrip|{}|ZXing decoded {} symbols", bPassed ? "PASS" : "FAIL", vecCodes.size());
   std::println("SHOWCASE|RESULT|{}|Independent EAN-13 generation and native ZXing decoding", bPassed ? "PASS" : "FAIL");
   return bPassed;
}

void DrawBarcode(std::vector<std::uint8_t> const& vecPixels, ImVec2 const aOrigin, float const flWidth, float const flHeight) {
   ImDrawList* const pDraw = ImGui::GetWindowDrawList();
   ImVec2 const aBottom { aOrigin.x + flWidth, aOrigin.y + flHeight };
   pDraw->AddRectFilled(aOrigin, aBottom, IM_COL32(241, 249, 254, 255), 14.0F);
   float const flScale = flWidth / static_cast<float>(iWidth);
   for(int iX = 0; iX < iWidth; iX += iModulePixels) {
      if(vecPixels[static_cast<std::size_t>(iHeight / 2 * iWidth + iX + iModulePixels / 2)] != 0) continue;
      float const flLeft = aOrigin.x + static_cast<float>(iX) * flScale;
      float const flRight = aOrigin.x + static_cast<float>(iX + iModulePixels) * flScale;
      pDraw->AddRectFilled({ flLeft, aOrigin.y + 15.0F }, { flRight, aOrigin.y + flHeight - 15.0F }, IM_COL32(11, 29, 55, 255));
   }
   pDraw->AddRect(aOrigin, aBottom, IM_COL32(54, 202, 225, 255), 14.0F, 0, 2.0F);
}

void Interactive() {
   NativeShowcase aApp { "BuildEngine | BARCODE SIGNAL LAB - Decode the Invisible", 1480, 850 };
   std::array<char, 13> arrDigits { '5','9','0','1','2','3','4','1','2','3','4','5','\0' };
   std::vector<std::uint8_t> vecPixels = Generate(arrDigits.data());
   ZXing::Barcodes vecResults = Decode(vecPixels, true);
   bool bTryHarder = true;
   float flZoom = 1.0F;
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({ 0.0F, 0.0F });
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("BARCODE SIGNAL LAB", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("BARCODE SIGNAL LAB / DECODE THE INVISIBLE",
                             "Live barcode synthesis  /  real ZXing-C++ 3.1.1 DLL  /  native C++23");
      ImGui::TextColored({ .46F, .93F, .88F, 1.0F }, "THE SCAN CHAMBER");
      ImGui::InputText("12-digit EAN data", arrDigits.data(), arrDigits.size());
      ImGui::SameLine();
      if(ImGui::Button("SCAN SIGNAL")) {
         try {
            vecPixels = Generate(arrDigits.data());
            vecResults = Decode(vecPixels, bTryHarder);
         }
         catch(std::exception const&) {
            vecResults.clear();
         }
      }
      ImGui::Checkbox("Try harder", &bTryHarder);
      ImGui::SameLine();
      ImGui::SliderFloat("Barcode scale", &flZoom, .6F, 1.0F);
      ImGui::Spacing();
      float const flWidth = std::min(ImGui::GetContentRegionAvail().x - 10.0F, 1060.0F) * flZoom;
      ImVec2 const aOrigin = ImGui::GetCursorScreenPos();
      DrawBarcode(vecPixels, aOrigin, flWidth, 270.0F);
      ImGui::Dummy({ flWidth, 285.0F });
      ImGui::Separator();
      ImGui::Text("DECODER STATUS  /  %d symbol(s) recognized", static_cast<int>(vecResults.size()));
      for(ZXing::Barcode const& theCode : vecResults)
         ImGui::TextColored({ .95F, .81F, .37F, 1.0F },
                            "DECODED EAN-13: %s", theCode.text().c_str());
      ImGui::TextDisabled("The bars are independently generated. Recognition uses the managed ZXing DLL.");
      ImGui::TextDisabled("No mocked detection, no online service, no ZXing writer backend.");
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
