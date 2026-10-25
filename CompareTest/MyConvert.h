// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "MyType_Traits.h"

#include <charconv>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstdlib>


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
[[nodiscard]] inline constexpr std::string my_Double_to_String_short(ty const& value, int iPrecision) {
   std::string target{ "0000000000000000" };
   std::to_chars(target.data(), target.data() + target.size(), value, std::chars_format::fixed, iPrecision);
   //target.resize(target.find_last_not_of("0") + 1);
   return target.erase(target.find_last_not_of("0") + 1);
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


// Append a fixed-precision floating-point number directly to the CSV buffer.
// Keep the original trailing-zero rule (including a possible final decimal
// point) so that this optimization does not silently change file contents.
template <std::floating_point ty>
inline void append_Double_to_String_short(std::string& target, ty value, int precision) {
   std::array<char, 128> digits;
   auto const result = std::to_chars(digits.data(), digits.data() + digits.size(),
                                    value, std::chars_format::fixed, precision);
   if (result.ec != std::errc{}) {
      throw std::runtime_error("floating-point CSV formatting failed");
      }

   char const* last = result.ptr;
   while (last != digits.data() && last[-1] == '0') {
      --last;
      }
   target.append(digits.data(), static_cast<std::size_t>(last - digits.data()));
}
