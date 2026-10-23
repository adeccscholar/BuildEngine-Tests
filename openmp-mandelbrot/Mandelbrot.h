#pragma once
#include <cstdint>
#include <vector>
namespace adecc::mandelbrot {
enum class Schedule { Serial, Static, Dynamic, Guided };
struct Settings {
   int width{1280}, height{720}, maxIterations{512}, threads{8}, chunk{4};
   double centerX{-0.5}, centerY{0.0}, spanX{3.4};
   Schedule schedule{Schedule::Dynamic};
};
struct Result {
   std::vector<std::uint32_t> pixels;
   std::uint64_t iterationTotal{};
   double milliseconds{};
   int threadsUsed{};
};
Result Compute(Settings const& settings);
char const* Name(Schedule schedule) noexcept;
}
