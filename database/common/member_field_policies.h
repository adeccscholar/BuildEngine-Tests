// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/**
\file member_field_policies.h
\brief Validated member birth dates and positive contribution amounts.
\details Shares the exact conversion and validation policy composition
used by the MSSQL CSV booking example: ISO/EU/US dates and German decimals.
Source CSV types stay strongly validated, while the persistent SQL tuple
uses adecc::date_ty and adecc::money_ty after adecc::Transform.
*/
#include <adecc/cpp_domain/policy_common.h>
#include <adecc/cpp_domain/policies_hawkers_tray.h>
#include <adecc/cpp_core/value_types.h>
#include <string>

namespace buildengine_tests::database::members {
using IsoBirthdayCase = adecc::CaseConversion<
   adecc::RegexStringPolicy<R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)">,
   adecc::ConvertToConversionPolicy<std::string, adecc::date_ty>>;
using EuropeanBirthdayCase = adecc::CaseConversion<
   adecc::RegexStringPolicy<R"(^[0-9]{2}\.[0-9]{2}\.[0-9]{4}$)">,
   adecc::ConvertToConversionPolicy<std::string, adecc::date_ty, adecc::DateFmt::EUROPE>>;
using UsBirthdayCase = adecc::CaseConversion<
   adecc::RegexStringPolicy<R"(^[0-9]{2}/[0-9]{2}/[0-9]{4}$)">,
   adecc::ConvertToConversionPolicy<std::string, adecc::date_ty, adecc::DateFmt::US>>;
using BirthdayConversion = adecc::ConversionChainPolicy<
   adecc::TrimStringConversionPolicy,
   adecc::CaseConversionPolicy<IsoBirthdayCase, EuropeanBirthdayCase, UsBirthdayCase>>;
using Geburtstag_ty = adecc::ConvertedPolicyValue<
   "Geburtstag", BirthdayConversion,
   adecc::ValidYearMonthDayPolicy,
   adecc::ConvertToStringWithArgumentsPolicy<adecc::date_ty, adecc::DateFmt::EUROPE>>;

using ContributionConversion = adecc::ConversionChainPolicy<
   adecc::TrimStringConversionPolicy,
   adecc::GermanDecimalToFloatConversionPolicy<double>>;
using Beitrag_ty = adecc::ConvertedPolicyValue<
   "Beitrag", ContributionConversion, adecc::PositiveNumberPolicy<double>>;
} // namespace buildengine_tests::database::members
