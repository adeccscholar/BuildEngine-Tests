// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace adecc::galaxy {
struct Star {
   float x{}, y{}, z{}, vx{}, vy{}, vz{}, brightness{};
   std::uint32_t color{};
};
enum class Schedule { Serial, Static, Dynamic, Guided };
struct Configuration {
   std::size_t count{20000};
   std::uint32_t seed{123456789};
   float gravity{4.5f}, softening{0.6f}, dt{0.008f};
};
struct Galaxy {
   std::vector<Star> stars;
   Configuration config;
};
Galaxy Initialize(Configuration configuration);
void Step(Galaxy& galaxy, Schedule schedule, int threads);
char const* ScheduleName(Schedule schedule) noexcept;
double Energy(Galaxy const& galaxy);
double MaxDifference(Galaxy const& a, Galaxy const& b);
}
