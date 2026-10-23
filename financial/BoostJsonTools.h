// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file BoostJsonTools.h
\brief Type-safe Boost.JSON conversion and validation utilities.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{MIT}
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#pragma once


// Boost
#include <boost/json/value.hpp>
#include <boost/json/string.hpp>
#include <boost/json/parse.hpp>

#include <boost/system/system_error.hpp> 

#include "chrono_workaround.h"  

// standard libraries
#include <iomanip>        // for std::get_time in the case if chrono::parse not available
#include <string>
#include <sstream>
#include <chrono>
#include <optional>
#include <charconv>
#include <type_traits>
#include <concepts>
#include <stdexcept>

namespace adecc {

/// namespace to wrap the types and functions for this boost tools
namespace boost_tools {

/*!
\brief Chrono-based type aliases for standardized date/time handling
*/
using timepoint_ty       = std::chrono::sys_seconds;                      ///< Absolute UTC time point (seconds precision)
using local_timepoint_ty = std::chrono::local_time<std::chrono::seconds>; ///< Local time point
using date_ty            = std::chrono::year_month_day;                   ///< Calendar date
using time_ty            = std::chrono::hh_mm_ss<std::chrono::seconds>;   ///< Time-of-day (hh:mm:ss)


using namespace std::string_literals;

/*!
  \brief Extracts a sub-object from a JSON value by key.
  \param root JSON value (should be an object)
  \param key  Key for the sub-object
  \returns Reference to the found sub-object
  \throw std::runtime_error if the root is not an object, key does not exist, or sub-value is not an object.
*/
inline boost::json::object const& extract_subobject(boost::json::value const& root, std::string_view key) {
   if (!root.is_object())
      throw std::runtime_error("JSON root is not an object");

   auto const& obj = root.as_object();
   if (!obj.contains(key))
      throw std::runtime_error(std::format("Key '{}' not found in JSON object", key));

   auto const& sub = obj.at(key);
   if (!sub.is_object())
      throw std::runtime_error(std::format("Key '{}' does not contain a JSON object", key));

   return sub.as_object();
   }


/**
  \brief Parses a JSON string and extracts a sub-object by key.
  \param json_str JSON document as string view
  \param key      Key of the desired sub-object
  \returns The found sub-object
  \throw std::runtime_error if JSON is invalid, key not found, or sub-object is not an object.
*/
inline boost::json::object extract_json_object(std::string_view json_str, std::string_view key) {
   boost::json::value jv;
   try {
      jv = boost::json::parse(json_str);
      }
   catch (std::exception const& ex) {
      throw std::runtime_error(std::format("Invalid JSON: {}", ex.what()));
      }
   return extract_subobject(jv, key);
   }

/**
  \brief Parses a JSON string and returns the top-level object.
  \param json_str JSON document as string view
  \returns The top-level JSON object
  \throw std::runtime_error if the JSON is invalid or not an object.
*/
inline boost::json::object extract_json_object(std::string_view json_str) {
   try {
      boost::json::value jv = boost::json::parse(json_str);
      return jv.as_object();
      }
   catch (std::exception const& ex) {
      throw std::runtime_error(std::format("Invalid JSON: {}", ex.what()));
      }
   }


/**
\name Traits-based JSON Value Conversion

\brief
Type-safe, extensible pattern for converting Boost.JSON values to C++ types.

\details
This collection of type trait specializations implements a generic, extensible, and robust pattern for converting JSON values to strongly typed C++ objects.  
Instead of centralizing all conversion logic in a single monolithic template, the system leverages **traits-based delegation**:
- The primary template `value_converter<T>` acts as a dispatcher and compile-time error guard.
- Each supported type (e.g. `bool`, `std::string`, integers, floating point, chrono/date types) receives its own explicit specialization, containing only the conversion logic for that type.
- This pattern results in clean separation of parsing logic, easy extensibility (simply specialize for new types), and improved compile-time error reporting using SFINAE.

Typical usage:
\code
boost::json::value jv = ...;
int x = value_converter<int>::convert(jv);
double d = value_converter<double>::convert(jv);
std::string s = value_converter<std::string>::convert(jv);
\endcode

Advantages:
- **Compile-time safety**: Only supported types can be converted; misuse is caught early.
- **Maintainability**: Parsing logic for each type is isolated.
- **Extensibility**: New type conversions can be added without modifying existing code.
- **Clarity**: Clear error messages for unsupported conversions.

This pattern is well-suited for server-side, data-import, or domain-mapping code that interacts with dynamic or schema-less data sources.

\warning
Always ensure a suitable specialization exists for your target type; instantiating the primary template will cause a compile-time error.

\see value_converter (primary template and all specializations)
*/
/// \{


/**
   \brief Primary template for type-safe JSON value conversion (traits-based delegation).
   
   \details
   This struct defines the primary template for JSON-to-type conversion.  
   Actual conversions are implemented via specializations for each supported type 
   (e.g., \c bool, \c int, \c std::string, chrono types). The traits-based approach enables 
   a clean separation of parsing logic, straightforward extensibility, and robust compile-time 
   type checking using SFINAE. Attempting to instantiate this primary template with a type for 
   which no specialization exists will result in a compile-time error.

   \details
   Traits-based delegation:  
   Rather than placing all type dispatch logic into a single template monolith, the type conversion 
   mechanism uses a specialization strategy via `struct value_converter<T>`.  
   This offers:
   - easy extensibility for new types by adding specializations,
   - clean separation of parsing logic,
   - improved compile-time and maintainability,
   - stable type checking using SFINAE.

   Each specialization must implement \c static ty convert(boost::json::value const&) for its respective type.
   
   \see Specializations of \c value_converter for supported types
   \tparam ty Target type for conversion
   \param value The JSON value to convert
   \returns No return value; this template is intentionally undefined for unsupported types.
   \throw This template does not provide an implementation and triggers a static assertion if instantiated.
   \warning Only use this primary template for documentation or extension; do not instantiate directly. 
   Always provide or use an appropriate specialization for your type.

*/
template <typename ty>
struct value_converter {
   static ty convert(boost::json::value const&) {
      static_assert(sizeof(ty) == 0, "No value_converter defined for this type");
      }
   };

// --- Specializations for standard types ---
// See file content for all value_converter specializations, e.g. for bool, std::string, 
// integral, floating point, date_ty, time_ty, timepoint_ty, etc.


/**
\brief Specialization of \c value_converter for converting JSON values to \c bool.

\details
This specialization implements the static \c convert method for the \c bool type.
It accepts JSON values that are:
- a native boolean (\c true/\c false)
- an integer (interpreted as true if nonzero)
- an unsigned integer (interpreted as true if nonzero)

If the value cannot be interpreted as a boolean, a \c std::runtime_error is thrown.

This struct is a specialization of the primary template \c value_converter.
For unsupported types, refer to the documentation of the primary template.

\see value_converter (primary template)
\tparam Specialization for \c bool
\param value The JSON value to convert to \c bool.
\return The converted boolean value.
\throw std::runtime_error if the JSON value cannot be interpreted as a boolean.
*/
template <>
struct value_converter<bool> {
   static bool convert(boost::json::value const& value) {
      if (value.is_bool()) [[likely]] return value.as_bool();
      if (value.is_int64())           return value.as_int64() != 0;
      if (value.is_uint64())          return value.as_uint64() != 0;
      throw std::runtime_error("JSON value not convertible to bool");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to \c std::string.

\details
This specialization implements the static \c convert method for the \c std::string type.
It accepts only JSON values that are of type string.  
If the value is not a string, a \c std::runtime_error is thrown.

This struct is a specialization of the primary template \c value_converter.
For details and extension points, see the primary template documentation.

\see value_converter (primary template)
\tparam Specialization for \c std::string
\param value The JSON value to convert to \c std::string.
\return The converted \c std::string value.
\throw std::runtime_error if the JSON value is not a string.
\warning Does not support automatic conversion from numeric or boolean values. Extend only if needed.
\code{.cpp}
boost::json::value jv = "adecc Scholar"s;
std::string s = value_converter<std::string>::convert(jv); // s == "adecc Scholar"
\endcode

\todo Support for automatic numeric to string conversion could be considered if required by the application.
*/
template <>
struct value_converter<std::string> {
   static std::string convert(boost::json::value const& value) {
      if (value.is_string()) [[likely]] return std::string(value.as_string().c_str());
      throw std::runtime_error("JSON value not convertible to string");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to unsigned integral types.

\details
This specialization implements conversion for all unsigned integer types except \c bool.
It accepts JSON values that are unsigned integer or signed integer types.
Negative values or values outside the target type's range result in an exception.

This struct is a specialization of the primary template \c value_converter.
For details, see the primary template documentation.

\see value_converter (primary template)
\tparam ty The unsigned integral target type (e.g. \c uint32_t).
\param value The JSON value to convert.
\return The converted unsigned integer value of type \c ty.
\throw std::runtime_error if the JSON value is not a (u)int, is negative, or is out of range.
\warning Conversion from floating point or string is not supported and will throw. Only use for strict JSON integer fields.

\code{.cpp}
boost::json::value jv = 42u;
uint32_t u = value_converter<uint32_t>::convert(jv); // u == 42
\endcode

\todo Add optional conversion from string if application JSON may encode numbers as strings.
*/
template <typename ty>
   requires std::is_integral_v<ty>&& std::is_unsigned_v<ty> && (!std::is_same_v<ty, bool>)
struct value_converter<ty> {
   static ty convert(boost::json::value const& value) {
      if (value.is_uint64()) {
         uint64_t raw = value.as_uint64();
         if (raw > std::numeric_limits<ty>::max())
            throw std::runtime_error("unsigned integer value out of bounds");
         return static_cast<ty>(raw);
         }
      if (value.is_int64()) {
         int64_t raw = value.as_int64();
         if (raw < 0 || static_cast<uint64_t>(raw) > std::numeric_limits<ty>::max())
            throw std::runtime_error("signed value out of range for unsigned target");
         return static_cast<ty>(raw);
         }
      throw std::runtime_error("JSON value not convertible to unsigned integer");
      }
};

/**
\brief Specialization of \c value_converter for converting JSON values to signed integral types.

\details
This specialization implements conversion for all signed integer types except \c bool.
It accepts JSON values that are signed or unsigned integer types.
Values outside the representable range of the target type result in an exception.

This struct is a specialization of the primary template \c value_converter.
For extension, see the primary template documentation.

\see value_converter (primary template)
\tparam ty The signed integral target type (e.g. \c int32_t).
\param value The JSON value to convert.
\return The converted signed integer value of type \c ty.
\throw std::runtime_error if the JSON value is not an integer or is out of range.
\warning Conversion from floating point or string is not supported and will throw.
\code{.cpp}
boost::json::value jv = -17;
int32_t i = value_converter<int32_t>::convert(jv); // i == -17
\endcode
\todo Add conversion from string where JSON encodes numbers as text, if such data sources must be handled.
*/
template <typename ty>
   requires std::is_integral_v<ty>&& std::is_signed_v<ty> && (!std::is_same_v<ty, bool>)
struct value_converter<ty> {
   static ty convert(boost::json::value const& value) {
      if (value.is_uint64()) {
         uint64_t raw = value.as_uint64();
         if (raw > static_cast<uint64_t>(std::numeric_limits<ty>::max()))
            throw std::runtime_error("unsigned value out of bounds for signed type");
         return static_cast<ty>(raw);
      }
      if (value.is_int64()) {
         int64_t raw = value.as_int64();
         if (raw < static_cast<int64_t>(std::numeric_limits<ty>::min()) ||
            raw > static_cast<int64_t>(std::numeric_limits<ty>::max()))
            throw std::runtime_error("signed integer out of bounds");
         return static_cast<ty>(raw);
      }
      throw std::runtime_error("JSON value not convertible to signed integer");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to floating point types.

\details
This specialization enables robust conversion of JSON values to floating point types (\c float, \c double, \c long double).
It supports native double, integer types, and string values.
String values are cleaned of whitespace and common currency symbols, thousand separators are removed, and EU-style decimals (comma) are converted to dot.
Parsing uses \c std::from_chars for efficiency and robustness.

This struct is a specialization of the primary template \c value_converter.
For unsupported types, refer to the documentation of the primary template.

\see value_converter (primary template)
\tparam ty The floating point type (\c float, \c double, or \c long double).
\param value The JSON value to convert.
\return The converted floating point value of type \c ty.
\throw std::runtime_error if the value is not convertible to floating point.
\warning No locale-based number parsing; only "en-US" and common EU-style numbers supported for strings.

\code{.cpp}
boost::json::value jv1 = 3.1415;
double pi = value_converter<double>::convert(jv1); // pi == 3.1415

boost::json::value jv2 = "2,50"s;
double x = value_converter<double>::convert(jv2); // x == 2.5 (EU style decimal)
\endcode

\todo Locale-aware number parsing could be considered for full internationalization.
\todo Support for scientific notation in string conversion could be extended.
*/
template <typename ty>
   requires std::is_floating_point_v<ty>
struct value_converter<ty> {
   static ty convert(boost::json::value const& value) {
      if (value.is_double()) [[likely]] return static_cast<ty>(value.as_double());
      if (value.is_int64())             return static_cast<ty>(value.as_int64());
      if (value.is_uint64())            return static_cast<ty>(value.as_uint64());

      if (value.is_string()) {
         std::string str = std::string(value.as_string().c_str());

         // Remove whitespace
         str.erase(std::remove_if(str.begin(), str.end(), [](char ch) {
                                    return std::isspace(static_cast<unsigned char>(ch));
                                    }), str.end());

         // Remove common currency symbols
         static constexpr std::string_view known_symbols[] = {
            "€", "$", "£", "CHF", "EUR", "USD"
            };
         for (auto const& symbol : known_symbols) {
            if (auto pos = str.find(symbol); pos != std::string::npos) {
               str.erase(pos, symbol.size());
               }
            }

         // Replace ',' with '.' if appropriate (EU style)
         if (str.find(',') != std::string::npos && str.find('.') == std::string::npos) {
            std::replace(str.begin(), str.end(), ',', '.');
            }
         else {
            // Remove thousand separators (comma or point)
            str.erase(std::remove(str.begin(), str.end(), ','), str.end());
            }

         // Versuch, mit from_chars zu parsen
         ty result{};
         auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
         if (ec == std::errc()) return result;
         }

      throw std::runtime_error("JSON value not convertible to floating point");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to \c date_ty.

\details
Parses ISO-formatted date strings ("YYYY-MM-DD") into \c date_ty (\c std::chrono::year_month_day).
If parsing fails or the value is not a string, a \c std::runtime_error is thrown.

This struct is a specialization of the primary template \c value_converter.
See primary template for extension details.

\see value_converter (primary template)
\tparam Specialization for \c date_ty
\param value The JSON value to convert.
\return The parsed date.
\throw std::runtime_error if parsing fails or the value is not a string.
\warning Does not support alternate date formats.

\code{.cpp}
boost::json::value jv = "2024-07-06"s;
date_ty date = value_converter<date_ty>::convert(jv); // date == July 6, 2024
\endcode

\todo Extend to support additional date formats or automatic timezone handling if required.
*/
template <>
struct value_converter<date_ty> {
   static date_ty convert(boost::json::value const& value) {
      if (value.is_string()) [[likely]] {
#if defined CHRONO_WORK_AROUND
         auto s = std::string(value.as_string().c_str());
         return parseDate( { s.data(), s.size() }, DateFmt::ISO).value(); 
#else
         std::chrono::sys_days tp;
         std::istringstream ss(std::string(value.as_string().c_str()));
         ss >> std::chrono::parse("%Y-%m-%d", tp);
         if (ss.fail()) throw std::runtime_error("Failed to parse date string");
         return std::chrono::year_month_day{ tp };
#endif
      }
      throw std::runtime_error("JSON value not convertible to date");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to \c time_ty.

\details
Supports multiple formats for time of day extraction:
- String in ISO 8601 format ("YYYY-MM-DDTHH:MM")
- Floating-point seconds since midnight
- Integer seconds since midnight

Throws if the value is not convertible or if the seconds are out of range.

This struct is a specialization of the primary template \c value_converter.
See the primary template for details.

\see value_converter (primary template)
\tparam Specialization for \c time_ty
\param value The JSON value to convert.
\return The parsed \c time_ty (hh:mm:ss).
\throw std::runtime_error if parsing fails, or if the value is out of the valid 0..86399 range.
\warning The implementation expects times to be in the 24-hour range (0–86399 seconds). Fractional values will be truncated.

\code{.cpp}
boost::json::value jv1 = "2024-07-06T15:30"s;
time_ty t1 = value_converter<time_ty>::convert(jv1); // t1 == 15:30:00

boost::json::value jv2 = 3600;
time_ty t2 = value_converter<time_ty>::convert(jv2); // t2 == 01:00:00
\endcode

\todo Add support for "HH:MM" or "HH:MM:SS" only strings if required by data.
*/
template <>
struct value_converter<time_ty> {
   static time_ty convert(boost::json::value const& value) {
      if (value.is_string()) [[likely]] {
#if defined CHRONO_WORK_AROUND
         auto s = std::string(value.as_string().c_str());
         auto tp = parseDateTime( { s.data(), s.size() }, DateTimeFmt::ISO_HMS); 
         if(!tp) throw std::runtime_error("Failed to parse time string");
         std::chrono::sys_seconds tpSeconds = std::chrono::time_point_cast<std::chrono::seconds>(*tp);
         std::chrono::seconds secs = tpSeconds.time_since_epoch();
         //std::chrono::local_time<std::chrono::seconds> lt = tpSeconds;
         return std::chrono::hh_mm_ss{ secs % std::chrono::days{1} };
#else
         std::chrono::local_time<std::chrono::seconds> lt;
         std::istringstream ss(std::string(value.as_string().c_str()));
         ss >> std::chrono::parse("%Y-%m-%dT%H:%M", lt);
         if (ss.fail()) throw std::runtime_error("Failed to parse time string");
         return std::chrono::hh_mm_ss{ lt.time_since_epoch() % std::chrono::days{1} };
#endif
         }

      if (value.is_double()) {
         double sec = value.as_double();
         if (sec < 0.0 || sec >= 86400.0)
            throw std::runtime_error(std::format("invalid floating seconds for hh_mm_ss: {}", sec));
         return time_ty{ std::chrono::seconds{ static_cast<int64_t>(sec) } };
         }

      if (value.is_int64()) {
         int64_t const sec = value.as_int64();
         if (sec < 0 || sec >= 86400)
            throw std::runtime_error(std::format("invalid seconds for hh_mm_ss: {}", sec));
         return time_ty{ std::chrono::seconds{ sec } };
         }

      if (value.is_uint64()) {
         uint64_t const sec = value.as_uint64();
         if (sec >= 86400)
            throw std::runtime_error(std::format("invalid seconds for hh_mm_ss: {}", sec));
         return time_ty{ std::chrono::seconds{ sec } };
         }
      throw std::runtime_error("JSON value not convertible to time");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to \c timepoint_ty.

\details
Parses ISO-formatted datetime strings ("YYYY-MM-DDTHH:MM") to \c timepoint_ty (\c std::chrono::sys_seconds).
Also supports integer and unsigned integer values as seconds since epoch.

This struct is a specialization of the primary template \c value_converter.
See the primary template for details.

\see value_converter (primary template)

\tparam Specialization for \c timepoint_ty
\param value The JSON value to convert.
\return The parsed absolute timepoint.
\throw std::runtime_error if parsing fails or the value is not convertible.
\warning Millisecond or timezone handling is not supported.

\code
boost::json::value jv = "2024-07-06T10:42"s;
timepoint_ty tp = value_converter<timepoint_ty>::convert(jv);
// tp represents 2024-07-06 10:42:00 UTC
\endcode

\todo Add support for ISO8601 with timezone or fractional seconds if required by data.
*/
template <>
struct value_converter<timepoint_ty> {
   static timepoint_ty convert(boost::json::value const& value) {
      if (value.is_string()) [[likely]] {
#if defined CHRONO_WORK_AROUND
         auto s = std::string(value.as_string().c_str());
         auto tp = parseDateTime( { s.data(), s.size() }, DateTimeFmt::ISO_HMS).value(); 
         return std::chrono::floor<std::chrono::seconds>(tp);   
#else
         std::chrono::sys_seconds tp;
         std::istringstream ss(std::string(value.as_string().c_str()));
         ss >> std::chrono::parse("%Y-%m-%dT%H:%M", tp);
         if (ss.fail()) throw std::runtime_error("Failed to parse timepoint string");
         return tp;
#endif
      }
      if (value.is_int64())  return timepoint_ty{ std::chrono::seconds(value.as_int64()) };
      if (value.is_uint64()) return timepoint_ty{ std::chrono::seconds(value.as_uint64()) };
      throw std::runtime_error("JSON value not convertible to timepoint");
   }
};

/**
\brief Specialization of \c value_converter for converting JSON values to \c local_timepoint_ty.

\details
Parses ISO-formatted local datetime strings ("YYYY-MM-DDTHH:MM") to \c local_timepoint_ty (\c std::chrono::local_time<std::chrono::seconds>).
Also supports integer and unsigned integer values as seconds since local epoch.

This struct is a specialization of the primary template \c value_converter.
See the primary template for extension and details.

\see value_converter (primary template)
\tparam Specialization for \c local_timepoint_ty
\param value The JSON value to convert.
\return The parsed local timepoint.
\throw std::runtime_error if parsing fails or the value is not convertible.
\warning No timezone offset parsing, local time is assumed.

\code
boost::json::value jv = "2024-07-06T12:34"s;
local_timepoint_ty ltp = value_converter<local_timepoint_ty>::convert(jv);
// ltp represents 2024-07-06 12:34:00 (local time)
\endcode

\todo Support parsing ISO8601 with timezone if required by application.
*/

inline std::chrono::local_time<std::chrono::seconds> toLocalSeconds(std::chrono::system_clock::time_point const& tp) {
   using namespace std::chrono;

   std::time_t t = system_clock::to_time_t(tp);

   std::tm localTm{};
#if defined(_WIN32)
   if(::localtime_s(&localTm, &t) != 0) {
      throw std::runtime_error("localtime_s failed");
   }
#else
   if(!::localtime_r(&t, &localTm)) {
      throw std::runtime_error("localtime_r failed");
   }
#endif

   // zurück in system_clock
   std::time_t locTimeT = std::mktime(&localTm);
   return floor<seconds>(std::chrono::local_time<seconds>(
      seconds{locTimeT}));
}

template <>
struct value_converter<local_timepoint_ty> {
   static local_timepoint_ty convert(boost::json::value const& value) {
      if (value.is_string()) [[likely]] {
#if defined CHRONO_WORK_AROUND
         auto s = std::string(value.as_string().c_str());
         auto tp = parseDateTime( { s.data(), s.size() }, DateTimeFmt::ISO_HMS);
         if(!tp) throw std::runtime_error("Failed to parse local_timepoint string");
         return toLocalSeconds(*tp);   
#else
         local_timepoint_ty tp;
         std::istringstream ss(std::string(value.as_string().c_str()));
         ss >> std::chrono::parse("%Y-%m-%dT%H:%M", tp);
         if (ss.fail()) throw std::runtime_error("Failed to parse local_timepoint string");
         return tp;
#endif
         }
      if (value.is_int64())  return local_timepoint_ty{ std::chrono::seconds(value.as_int64()) };
      if (value.is_uint64()) return local_timepoint_ty{ std::chrono::seconds(value.as_uint64()) };
      throw std::runtime_error("JSON value not convertible to local_timepoint");
      }
   };

/// \}

/**
\name Policy-based Validation for JSON Type Conversion

\brief
Flexible, reusable pattern for validating values during JSON-to-C++ type conversion.

\details
This set of validator classes demonstrates the application of the **policy pattern** in modern C++:
- Instead of embedding all validation logic inside the type converter or relying on inheritance, each validator is a distinct template, passed as a type parameter to conversion routines.
- This decouples the core conversion ("what" is being converted) from any contextual or application-specific checks ("how" is the result validated).
- Validators can perform checks such as value ranges, mandatory fields, regular expression matching, plausibility, or domain-specific rules, and throw exceptions if validation fails.

Usage pattern:
\code
using MyValidator = IntegerRangeValidator<1, 100>;
int value = get_value<int, false, MyValidator>(jv, "key"); // Only accepts values in [1, 100]
\endcode

Advantages:
- **Extensibility**: New checks can be added independently of conversion logic.
- **Separation of concerns**: Cleanly splits type conversion and validation.
- **Compile-time selection**: Chosen via template parameters for zero runtime overhead.

This pattern is especially useful for robust data ingestion from external APIs or user input, where type safety and domain validation are required.

\warning
Always ensure that the validation policies used are consistent with your application's data requirements.

\see default_validator, DateForecastValidator, IntegerRangeValidator
*/
/// \{

/**
\brief Default no-op validator for type conversion.

\details
This policy struct provides an empty \c check method for any type \c ty.
Used as the default when no validation is needed, or as a base for further specialization.

\tparam ty The type being validated.
\param value The value to (not) validate.
\return None
\throw Never throws.

\see validator_for (default type alias for selecting validator policy)
*/
template <typename ty>
struct default_validator {
   static void check(ty const&) noexcept {}
   };

/**
\brief Type alias to select the default validator for a type.

\details
Allows deferred resolution of the validator policy until template instantiation, enabling easy specialization per type if necessary.

\tparam ty The type for which a validator is required.
\see default_validator
*/
template <typename ty>
using validator_for = default_validator<ty>;

/**
\brief Date forecast range validator for weather/time domain applications.

\details
Checks whether a given date is within the forecast window of [today, today + forecast_days] (inclusive).  
Throws an exception if the date is outside this window.

\tparam forecast_days The number of days into the future considered valid.
\param date The date value to validate.
\throw std::runtime_error if \c date is before today or beyond the forecast horizon.

\code
using Validator = DateForecastValidator<7>;
date_ty date = value_converter<date_ty>::convert(jv);
Validator::check(date); // Validates the date is within today and 7 days ahead
\endcode

\warning The system clock is used for "today"; time zone handling depends on the system.
\todo Extend to support minimum date in the past if required.
*/
template <std::int32_t forecast_days>
struct DateForecastValidator {
   static void check(date_ty const& date) {
      auto const today_tp = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
      std::chrono::year_month_day const today{ std::chrono::sys_days{ today_tp } };
      std::chrono::year_month_day const upper{ std::chrono::sys_days{ today_tp + std::chrono::days{ forecast_days } } };

      if (date < today || date > upper) {
         throw std::runtime_error(std::format("date out of forecast range: allowed = [{} - {}], got = {}",
                                  today, upper, date));
         }
      }
   };

/**
\brief Integer range validator policy for arbitrary min/max values.

\details
Checks that an integral value is within the allowed range [min_val, max_val] (inclusive).
Throws an exception if the value is out of bounds.

\tparam min_val The minimum allowed value (inclusive).
\tparam max_val The maximum allowed value (inclusive).
\param value The integral value to validate.
\throw std::runtime_error if \c value is outside the allowed range.

\code
using Validator = IntegerRangeValidator<0, 100>;
uint32_t val = value_converter<uint32_t>::convert(jv);
Validator::check(val); // Only allows 0 <= val <= 100
\endcode

\warning Only intended for use with integral types. Will not compile for floating point or non-integral types.
\todo Support for range checking on floating point or custom types could be added as needed.
*/
template <auto min_val, auto max_val>
struct IntegerRangeValidator {
   static_assert(std::is_integral_v<decltype(min_val)>, "IntegerRangeValidator requires integral values");
   static_assert(std::is_integral_v<decltype(max_val)>, "IntegerRangeValidator requires integral values");
   static_assert(min_val <= max_val, "min_val must be less than or equal to max_val");

   template <typename ty>
   static void check(ty const& value) {
      static_assert(std::is_integral_v<ty>, "IntegerRangeValidator can only be used with integral types");
      if (value < min_val || value > max_val) {
         throw std::runtime_error(std::format(
            "value out of allowed range: [{} - {}], got = {}",
            min_val, max_val, value));
         }
      }
   };

/// \}


/**
\name Generic Extraction and Validation for JSON Values

\brief
Unified, type-safe interface for extracting and validating typed values from Boost.JSON structures.

\details
This family of overloaded template functions implements a flexible and extensible pattern for mapping untyped JSON data onto strongly-typed C++ values.  
The approach uses:
- **Traits-based conversion:** Conversion is delegated to the corresponding `value_converter<T>` specialization for type safety and extensibility.
- **Policy-based validation:** Optional validation logic is injected as a policy (template parameter) and called immediately after conversion, enabling domain-specific checks (e.g. value ranges, forecast intervals).
- **Optional return:** By setting `opt_val=true`, the result can be returned as a `std::optional<T>`, mapping JSON `null` to `std::nullopt` instead of throwing.
- **Uniform API:** The same function template is used for values, objects (by key), and arrays (by index), reducing code duplication and promoting a consistent approach to JSON mapping.

This pattern is ideal for backend services and data pipelines that require robust deserialization of arbitrary or schema-less JSON documents.

**Example usage:**
\code
boost::json::object obj = ...;
int ival = get_value<int>(obj, "count");
std::optional<double> dopt = get_value<double, true>(obj, "ratio");

using RangeValidator = IntegerRangeValidator<0, 100>;
int pct = get_value<int, false, RangeValidator>(obj, "percent"); // Ensures percent in [0,100]

boost::json::array arr = ...;
std::string s = get_value<std::string>(arr, 0); // From array
\endcode

\warning
If no matching `value_converter<T>` exists for the target type, this API will fail to compile.  
Domain validation failures will throw a `std::runtime_error` at runtime.

\see value_converter (type conversion), validator_for, IntegerRangeValidator, DateForecastValidator
*/
/// \{
	
/**
\brief Extracts and converts a value from a JSON value to a target type, with optional validation and support for null.

\details
Delegates conversion to the appropriate `value_converter<ty>`, then (optionally) validates using the supplied Validator policy.
If `opt_val` is `true`, `null` values result in `std::nullopt`. Otherwise, a `null` value throws.

\tparam ty        The target type to which the value should be converted.
\tparam opt_val   If `true`, return type is `std::optional<ty>`, and JSON null is permitted.
\tparam Validator Policy type for value validation (default: `validator_for<ty>`).
\param value      The JSON value to convert.
\return Converted and validated value, or `std::nullopt` if `opt_val` is `true` and the value is null.
\throw std::runtime_error if conversion or validation fails, or if a null value is encountered and `opt_val` is `false`.

\code{.cpp}
boost::json::value v = 42;
int x = get_value<int>(v); // x == 42

boost::json::value vnull = nullptr;
std::optional<int> onull = get_value<int, true>(vnull); // onull == std::nullopt
\endcode
*/	
template <typename ty, bool opt_val = false, typename Validator = validator_for<ty>>
std::conditional_t<opt_val, std::optional<ty>, ty> get_value(boost::json::value const& value) {
   if constexpr (opt_val) {
      if (value.is_null()) return std::nullopt;
      }
   else {
      if (value.is_null()) throw std::runtime_error("unexpeced null value.");
      }
   ty result = value_converter<ty>::convert(value);
   Validator::check(result);
   return result;
   }

/**
\brief Extracts, converts, and validates a value from a JSON object by key.

\details
Retrieves the value associated with `key` from the JSON object, then delegates to the main `get_value` logic for conversion and validation.

\tparam ty        The target type to which the value should be converted.
\tparam opt_val   If `true`, return type is `std::optional<ty>`, and JSON null is permitted.
\tparam Validator Policy type for value validation (default: `validator_for<ty>`).
\param current    The JSON object containing the field.
\param key        The key of the value to extract.
\return Converted and validated value, or `std::nullopt` if `opt_val` is `true` and the value is null.

\throw std::runtime_error if key is missing, or conversion/validation fails.

\code{.cpp}
boost::json::object obj = ...;
double d = get_value<double>(obj, "price");
\endcode
*/
template <typename ty, bool opt_val = false, typename Validator = validator_for<ty>>
std::conditional_t<opt_val, std::optional<ty>, ty> get_value(boost::json::object const& current, std::string const& key) {
   if (!current.contains(key)) throw std::runtime_error(std::format("value {} does not exist.", key));
   try {
      return get_value<ty, opt_val, Validator>(current.at(key));
      }
   catch (std::exception const& ex) {
      throw std::runtime_error(std::format("error for field {}: {}", key, ex.what()));
      }
   }

/**
\brief Extracts, converts, and validates a value from a JSON array by index.

\details
Retrieves the value at `index` from the JSON array, then delegates to the main `get_value` logic for conversion and validation.

\tparam ty        The target type to which the value should be converted.
\tparam opt_val   If `true`, return type is `std::optional<ty>`, and JSON null is permitted.
\tparam Validator Policy type for value validation (default: `validator_for<ty>`).
\param arr        The JSON array containing the value.
\param index      The index of the value to extract.
\return Converted and validated value, or `std::nullopt` if `opt_val` is `true` and the value is null.
\throw std::runtime_error if index is out of range, or conversion/validation fails.

\code{.cpp}	
boost::json::array arr = ...;
bool flag = get_value<bool>(arr, 1);
\endcode
*/
template <typename ty, bool opt_val = false, typename Validator = validator_for<ty>>
std::conditional_t<opt_val, std::optional<ty>, ty> get_value(boost::json::array const& arr, size_t index) {
   if (index >= arr.size() ) throw std::runtime_error(std::format("index {} is out of range (0, {})", index, arr.size() - 1));
   try {
      return  get_value<ty, opt_val, Validator>(arr[index]);
      }
   catch (std::exception const& ex) {
      throw std::runtime_error(std::format("error for field with index {}: {}", index, ex.what()));
      }
   }

/// \}

} // end of namespace boost_tools

} // end of namespace adecc

