// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file CommonTools.h
\brief Common tools and concepts.

*/

#include <ostream>
#include <type_traits>
#include <concepts>

namespace adecc {
	
   template <typename... output_ty>
   concept optional_ostream = sizeof...(output_ty) == 0 ||
                              (sizeof...(output_ty) == 1 &&
                               (std::derived_from<std::remove_cvref_t<output_ty>, std::ostream> && ...) &&
                               (std::is_lvalue_reference_v<output_ty&&> && ...)
                              );

   template <typename... output_ty>
   inline constexpr bool has_valid_output_v = sizeof...(output_ty) == 1 &&
                                              optional_ostream<output_ty...>;

   template <typename... output_ty>
   [[nodiscard]] constexpr bool hasValidOutput() noexcept {
      return sizeof...(output_ty) == 1 && optional_ostream<output_ty...>;
      }

   template <typename... output_ty> requires optional_ostream<output_ty...>
   [[nodiscard]]
   constexpr decltype(auto) optionalOutput(output_ty&&... outputArgs) noexcept {
      if constexpr (hasValidOutput<output_ty...>()) {
         return std::get<0>(std::forward_as_tuple(outputArgs...));
         }
      else return std::monostate{};
      }

} // end of namespace adecc