// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "FinancialBasics.h"

#include <algorithm>
#include <concepts>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace adecc::financial {




template <std::floating_point ty>
[[nodiscard]]
fund<ty> CombineFunds(std::span<fund<ty> const> funds,
                      std::string name = "Combined funds") {
   fund<ty> result { .name = std::move(name), .currency = std::string { default_currency } };
   std::size_t count {};
   std::ranges::for_each(funds, [&count](fund<ty> const& current) {
      count += current.cashflows.size();
      });
   result.cashflows.reserve(count);
   std::ranges::for_each(funds, [&result](fund<ty> const& current) {
      result.cashflows.insert(result.cashflows.end(),
                              current.cashflows.begin(), current.cashflows.end());
      });
   std::ranges::sort(result.cashflows, {}, &cashflow<ty>::due_date);
   return result;
   }

} // namespace adecc::financial
