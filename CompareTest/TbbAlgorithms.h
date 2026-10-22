#pragma once

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_reduce.h>
#include <cstddef>
#include <string_view>

// Independent chunks: no parallel STL execution-policy implementation required.
[[nodiscard]] inline std::size_t CountNewlinesTbb(std::string_view text) {
   return oneapi::tbb::parallel_reduce(
      oneapi::tbb::blocked_range<std::size_t>(0, text.size()),
      std::size_t {0},
      [text](auto const& chunk, std::size_t sum) {
         for (auto i = chunk.begin(); i != chunk.end(); ++i)
            if (text[i] == '\n') ++sum;
         return sum;
      },
      [](std::size_t lhs, std::size_t rhs) { return lhs + rhs; });
}
