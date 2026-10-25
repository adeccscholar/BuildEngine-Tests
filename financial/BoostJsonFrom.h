// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file BoostJsonFrom.h
\brief Generic Boost.JSON-to-C++ object mapping utilities.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#include <boost/json/value.hpp>
#include <boost/json/object.hpp>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <format>
#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

namespace adecc {

namespace boost_tools {

   /**
     \brief Tag type for ADL-dispatch of user-defined JSON conversion functions.

     \details
       This struct is used solely as a tag for distinguishing user-defined overloads of \c from_json from unrelated functions.  
       It prevents accidental matching of unrelated functions in ADL.
   */
   struct from_json_tag {};

   /**
     \brief Type trait to detect whether a type \c ty has an ADL-discovered \c from_json function with the correct signature.

     \details
        The primary template yields \c false for types with no matching function; a partial specialization uses SFINAE to check 
		for the presence of a suitable overload.

     \tparam ty Type to test.
   */   
   template <typename ty, typename = void>
   struct has_from_json : std::false_type {};

   /**
     \brief Partial specialization: detected if \c from_json(ty&, boost::json::object const&, from_json_tag) is available.

     \details
      This enables compile-time detection (via \c has_from_json_v) of mapping support for a type.

     \tparam ty Type to test.
   */
   template <typename ty>
   struct has_from_json <ty, std::void_t<decltype(
             from_json(std::declval<ty&>(), std::declval<boost::json::object const&>(), from_json_tag{}))>> : std::true_type {};

   /**
     \brief Convenience variable template for querying \c has_from_json for a type.

     \details
       Use as \c has_from_json_v<T> to test if a suitable \c from_json function exists.

     \tparam ty Type to test.
   */
   template <typename ty>
   constexpr bool has_from_json_v = has_from_json<ty>::value;

   /**
     \brief Entry point for generic JSON-to-object mapping.

     \details
     Attempts to convert a Boost.JSON value to type \c ty using an ADL-found, tag-dispatched \c from_json overload.  
     - Throws if the JSON value is not an object.
     - If a suitable \c from_json function exists (see above), it is called to fill the result.
     - If not, a static assertion fails at compile time with a clear message.

     \tparam ty The type to which to convert.
     \param jv The Boost.JSON value (should be an object).
     \return An instance of \c ty, constructed from the JSON object.
     \throw std::runtime_error if \c jv is not a JSON object.
     \throw static_assert at compile time if no suitable \c from_json function exists.

     \code{.cpp}
     struct Foo { int x; std::string y; };
     namespace boost_tools {
        void from_json(Foo& dest, boost::json::object const& obj, from_json_tag) {
           dest.x = get_value<int>(obj, "x");
           dest.y = get_value<std::string>(obj, "y");
           }
        }
     Foo foo = boost_tools::from_json<Foo>(some_json_value);
     \endcode

     \warning The user must provide a \c from_json function in the same namespace as the target type (to enable ADL).

     \see has_from_json_v, from_json_tag
   */
   template <typename ty>
   ty from_json(boost::json::value const& jv) {
      if (!jv.is_object())
         throw std::runtime_error("expected JSON object");

      boost::json::object const& obj = jv.as_object();
      ty result;

      if constexpr (has_from_json_v<ty>) {
         from_json(result, obj, from_json_tag{});
         }
      else {
         static_assert(sizeof(ty) == 0, "No from_json overload found for this type.");
         }
      return result;
      }


   /**
      \brief Converts a JSON array to a vector of domain objects using from_json<T>.
   */
   template <typename ty>
   std::vector<ty> from_json_array(boost::json::value const& jv) {
      if (!jv.is_array())
         throw std::runtime_error("expected JSON array");

      auto const& arr = jv.as_array();
      std::vector<ty> result;
      result.reserve(arr.size());

      std::ranges::transform(arr, std::back_inserter(result),
         [](boost::json::value const& current) {
            return from_json<ty>(current);
            });

      return result;
      }

} // namespace boost_tools

} // namespace adecc
