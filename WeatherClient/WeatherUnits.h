// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherUnits.h
    \brief Pure C++23 weather-unit conversion and display formatting.
    \details WeatherModel keeps Open-Meteo values in metric units. Switching
    the view never changes measurements, timezone or HTTP API parameters.
*/
#include <format>
#include <string>
#include <string_view>

namespace weather_client {
   enum class UnitSystem { Metric, US };

   [[nodiscard]] constexpr double Temperature(double celsius,UnitSystem units) noexcept {
      return units==UnitSystem::US?celsius*9.0/5.0+32.0:celsius;
   }

   [[nodiscard]] constexpr double TemperatureDelta(double celsius,UnitSystem units) noexcept {
      return units==UnitSystem::US?celsius*9.0/5.0:celsius;
   }

   [[nodiscard]] constexpr double WindSpeed(double kmh,UnitSystem units) noexcept {
      return units==UnitSystem::US?kmh*0.621371192237334:kmh;
   }

   [[nodiscard]] constexpr double Precipitation(double millimetres,UnitSystem units) noexcept {
      return units==UnitSystem::US?millimetres/25.4:millimetres;
   }

   [[nodiscard]] constexpr double Pressure(double hpa,UnitSystem units) noexcept {
      // Standard conversion: 1 hPa = 0.029529983071445 inHg.
      return units==UnitSystem::US?hpa*0.029529983071445:hpa;
   }

   [[nodiscard]] constexpr std::string_view UnitsId(UnitSystem units) noexcept {
      return units==UnitSystem::US?"us":"metric";
   }

   [[nodiscard]] constexpr UnitSystem ParseUnits(std::string_view value) noexcept {
      return value=="us"||value=="US"||value=="imperial"
         ?UnitSystem::US:UnitSystem::Metric;
   }

   [[nodiscard]] constexpr std::string_view TempUnit(UnitSystem units) noexcept {
      return units==UnitSystem::US?"\u00b0F":"\u00b0C";
   }

   [[nodiscard]] constexpr std::string_view SpeedUnit(UnitSystem units) noexcept {
      return units==UnitSystem::US?"mph":"km/h";
   }

   [[nodiscard]] constexpr std::string_view RainUnit(UnitSystem units) noexcept {
      return units==UnitSystem::US?"in":"mm";
   }

   [[nodiscard]] constexpr std::string_view PressureUnit(UnitSystem units) noexcept {
      return units==UnitSystem::US?"inHg":"hPa";
   }

   [[nodiscard]] inline std::string TempText(double value,UnitSystem units,
      bool whole=false) {
      if(whole) return std::format("{:.0f} {}",Temperature(value,units),TempUnit(units));
      return std::format("{:.1f} {}",Temperature(value,units),TempUnit(units));
   }

   [[nodiscard]] inline std::string TempDeltaText(double value,UnitSystem units) {
      return std::format("{:.1f} {}",TemperatureDelta(value,units),TempUnit(units));
   }

   [[nodiscard]] inline std::string WindText(double value,UnitSystem units) {
      return std::format("{:.0f} {}",WindSpeed(value,units),SpeedUnit(units));
   }

   [[nodiscard]] inline std::string RainText(double value,UnitSystem units) {
      if(units==UnitSystem::US)
         return std::format("{:.2f} in",Precipitation(value,units));
      return std::format("{:.1f} mm",value);
   }

   [[nodiscard]] inline std::string PressureText(double value,UnitSystem units) {
      if(units==UnitSystem::US)
         return std::format("{:.2f} inHg",Pressure(value,units));
      return std::format("{:.1f} hPa",value);
   }

   [[nodiscard]] inline std::string UnitsButton(UnitSystem units) {
      return units==UnitSystem::US?"UNITS: US":"UNITS: METRIC";
   }
} // namespace weather_client
