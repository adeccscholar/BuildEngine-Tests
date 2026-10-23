// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace adecc::geo {
struct Point { double latitude{}, longitude{}; };
struct Bounds {double minX{}, minY{}, maxX{}, maxY{};};
struct Dataset {std::vector<Point> points; std::size_t rejected{}; double readMs{};};
enum class Mode { Points, Density };
enum class Method { Serial, Tbb, OpenMP };
Dataset Load(std::string const& path, bool parallel);
Bounds ProjectBounds(std::vector<Point> const& points, bool parallel);
struct Image {
   int width{}, height{};
   std::vector<std::uint32_t> argb;
   std::size_t totalPoints{};
   double elapsedMs{};
   std::uint32_t peak{};
};
Image Rasterize(std::vector<Point> const& points, Bounds const& bounds,
   int width, int height, Mode mode, Method method, int threads, int cellSize);
}
