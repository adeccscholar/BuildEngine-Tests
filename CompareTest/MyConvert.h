#pragma once

#include "MyType_Traits.h"

#include <charconv>
#include <string>
#include <string_view>
#include <cstdlib>
#include <array>
#include <stdexcept>


template <std::integral ty>
[[nodiscard]] inline constexpr ty string_to_int(myString auto const&  strValue) {
   if (strValue.size() == 0) return 0;
   auto str = strValue.substr(strValue.find_first_not_of(" "), strValue.find_last_not_of(" ") - strValue.find_first_not_of(" ") + 1);
   if(str.size() > 0) {
      ty value = 0;
      // auto [ptr, ec] =
      std::from_chars(strValue.data(), strValue.data() + strValue.size(), value);
      // Fehlerbehandlung
      return value;
      }
   else {
      return 0;
      }
   }




//template <typename ty>
//   requires std::floating_point<ty>
//inline constexpr std::string my_Double_to_String(ty const& value) {
[[nodiscard]] inline constexpr std::string my_Double_to_String(std::floating_point auto const& value) {
   std::array<char, 20> target;
   auto [ptrValue, ec] = std::to_chars(target.data(), target.data() + target.size(), value);
   return std::string(target.data(), ptrValue);
}

template <typename ty>
   requires std::floating_point<ty>
[[nodiscard]] inline std::string my_Double_to_String_short(ty const& value, int iPrecision) {
   std::array<char, 128> aBuffer{};
   auto const aResult = std::to_chars(aBuffer.data(), aBuffer.data() + aBuffer.size(),
      value, std::chars_format::fixed, iPrecision);
   if (aResult.ec != std::errc{}) throw std::runtime_error("floating point formatting failed");
   std::string strResult(aBuffer.data(), aResult.ptr);
   auto const iPoint = strResult.find('.');
   if (iPoint != std::string::npos) {
      while (strResult.size() > iPoint + 1 && strResult.back() == '0') {
         strResult.pop_back();
         }
      if (strResult.back() == '.') strResult.pop_back();
      }
   return strResult;
}

//template <typename ty>
//requires std::floating_point<ty>
//inline std::string my_Double_to_String(ty const& value, int iPrecision) {
[[nodiscard]] inline constexpr std::string my_Double_to_String(std::floating_point auto const& value, int iPrecision) {
   std::array<char, 18> target;
   auto [ptrValue, ec] = std::to_chars(target.data(), target.data() + target.size(), value, std::chars_format::fixed, iPrecision);
   return std::string(target.data(), ptrValue);
}

[[nodiscard]] inline constexpr std::string my_Double_to_String_G(std::floating_point auto const& value, int iPrecision) {
   std::array<char, 18> target;
   auto [ptrValue, ec] = std::to_chars(target.data(), target.data() + target.size(), value, std::chars_format::fixed, iPrecision);
   auto itPoint = std::find(target.begin(), target.end(), '.');
   *itPoint = ',';
   auto iLen = itPoint - target.begin();
   auto d = std::div((int)iLen, 3);
   if (std::string strVal(target.data(), ptrValue); d.quot > 0) {
      size_t start = d.rem == 0 ? 3 : d.rem;
      for (size_t a = start == 3 ? 1 : 0; a < d.quot; ++a) {
         strVal.insert(strVal.begin() + start, '.');
         start += 4;
      }
      return strVal;
   }
   else return strVal;
}
