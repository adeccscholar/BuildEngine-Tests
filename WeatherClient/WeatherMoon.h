// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherMoon.h
    \brief Offline approximate lunar phase and illuminated fraction.
    \details Mean synodic-month approximation; not ephemeris-grade astronomy.
    Uses the known new moon of 2000-01-06 18:14 UTC as a reference.
    Open-Meteo times are location-local: the local clock is used as an
    approximation (UTC offset is not known here), with a possible phase
    error of several hours. No new API, asset or operating-system call.
*/
#include <chrono>
#include <cmath>
#include <optional>
#include <string_view>

namespace weather_client {
   enum class MoonStage {
      New, WaxingCrescent, FirstQuarter, WaxingGibbous,
      Full, WaningGibbous, LastQuarter, WaningCrescent
   };

   struct MoonPhase {
      double cycle{};          // [0, 1): new moon 0; full moon ~0.5
      double illumination{};   // [0, 1]
      MoonStage stage{MoonStage::New};
      bool waxing{};
   };

   [[nodiscard]] inline std::optional<MoonPhase> CalculateMoonPhase(
      std::string_view dateTime
   ) {
      if(dateTime.size()<10 || dateTime[4]!='-' || dateTime[7]!='-')
         return std::nullopt;
      auto number=[&](std::size_t pos,std::size_t count)->int {
         int value=0;
         for(std::size_t i=0;i<count;++i) {
            char const c=dateTime[pos+i];
            if(c<'0'||c>'9') return -1;
            value=value*10+c-'0';
         }
         return value;
      };
      int const y=number(0,4),m=number(5,2),d=number(8,2);
      if(y<1||m<1||m>12||d<1||d>31) return std::nullopt;
      using namespace std::chrono;
      year_month_day const day{year{y},month{static_cast<unsigned>(m)},
         std::chrono::day{static_cast<unsigned>(d)}};
      if(!day.ok()) return std::nullopt;
      // No timezone conversion here: hourly Open-Meteo ISO strings carry
      // local clock values but usually no explicit numeric UTC offset.
      double hours=12.;
      if(dateTime.size()>=16 && dateTime[10]=='T') {
         int const h=number(11,2),minute=number(14,2);
         if(h<0||h>23||minute<0||minute>59||dateTime[13]!=':')
            return std::nullopt;
         hours=static_cast<double>(h)+static_cast<double>(minute)/60.;
      }
      constexpr double synodic=29.530588853;
      sys_days const epoch{year{2000}/January/6};
      sys_days const today{day};
      double const days=static_cast<double>((today-epoch).count())+
         (hours-18.-14./60.)/24.;
      double cycle=std::fmod(days/synodic,1.);
      if(cycle<0.) cycle+=1.;
      constexpr double pi=3.14159265358979323846;
      double const illumination=(1.-std::cos(2.*pi*cycle))*.5;
      unsigned const octant=static_cast<unsigned>(std::floor(cycle*8.+.5))%8U;
      MoonPhase result;
      result.cycle=cycle;
      result.illumination=illumination;
      result.stage=static_cast<MoonStage>(octant);
      result.waxing=cycle<.5;
      return result;
   }

   [[nodiscard]] constexpr std::string_view MoonStageName(MoonStage stage) {
      switch(stage) {
         case MoonStage::New: return "NEW MOON";
         case MoonStage::WaxingCrescent: return "WAXING CRESCENT";
         case MoonStage::FirstQuarter: return "FIRST QUARTER";
         case MoonStage::WaxingGibbous: return "WAXING GIBBOUS";
         case MoonStage::Full: return "FULL MOON";
         case MoonStage::WaningGibbous: return "WANING GIBBOUS";
         case MoonStage::LastQuarter: return "LAST QUARTER";
         case MoonStage::WaningCrescent: return "WANING CRESCENT";
      }
      return "MOON PHASE UNKNOWN";
   }
} // namespace weather_client
