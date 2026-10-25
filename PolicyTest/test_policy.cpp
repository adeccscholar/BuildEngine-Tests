// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#include "policy_common.h"
#include "policies_hawkers_tray.h"
#include "sepa_policies_hawkers_tray.h"
#include "converted_string.h"
#include "fixed_numeric.h"
#include "value_types.h"

#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
   struct Runner {
      int total{};
      int failed{};
      void Check(bool ok, std::string_view name) {
         ++total;
         if(!ok) ++failed;
         std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
      }
      template<class T, class Value>
      void Accept(std::string_view name, Value&& value) {
         try { T object{std::forward<Value>(value)}; (void)object; Check(true,name); }
         catch(std::exception const& e) { Check(false,std::format("{} unexpected: {}",name,e.what())); }
      }
      template<class T, class Value>
      void Reject(std::string_view name, Value&& value) {
         try { T object{std::forward<Value>(value)}; (void)object; Check(false,name); }
         catch(adecc::PolicyViolation const& e) { Check(!e.Kind().empty() && !e.Message().empty(),name); }
         catch(std::exception const& e) { Check(false,std::format("{} wrong exception: {}",name,e.what())); }
      }
   };

   using Date = std::chrono::year_month_day;
   using DateISO = adecc::ConversionChainPolicy<adecc::TrimStringConversionPolicy,
      adecc::ConvertToConversionPolicy<std::string,Date>>;
   using DateDE = adecc::ConversionChainPolicy<adecc::TrimStringConversionPolicy,
      adecc::ConvertToConversionPolicy<std::string,Date,adecc::DateFmt::EUROPE>>;
   using BirthPolicy = adecc::AllOfPolicy<Date,adecc::ValidYearMonthDayPolicy,
      adecc::NotBeforeFixedDatePolicy<"1900-01-01">,adecc::NotAfterTodayPolicy>;
   using BirthISO = adecc::ConvertedPolicyValue<"Birthdate",DateISO,BirthPolicy>;
   using BirthDE = adecc::ConvertedPolicyValue<"Birthdate",DateDE,BirthPolicy,
      adecc::ConvertToStringWithArgumentsPolicy<Date,adecc::DateFmt::EUROPE>>;
   using IsoPattern = adecc::RegexStringPolicy<R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)">;
   using GermanPattern = adecc::RegexStringPolicy<R"(^[0-9]{2}\.[0-9]{2}\.[0-9]{4}$)">;
   using IsoCase = adecc::CaseConversion<IsoPattern,
      adecc::ConvertToConversionPolicy<std::string,std::optional<Date>>>;
   using GermanCase = adecc::CaseConversion<GermanPattern,
      adecc::ConvertToConversionPolicy<std::string,std::optional<Date>,adecc::DateFmt::EUROPE>>;
   using FlexibleConversion = adecc::ConversionChainPolicy<adecc::TrimStringConversionPolicy,
      adecc::CaseConversionPolicy<IsoCase,GermanCase>>;
   using FlexibleBirthDate = adecc::ConvertedPolicyValue<"Birthdate",FlexibleConversion,BirthPolicy,
      adecc::ConvertToStringWithArgumentsPolicy<std::optional<Date>,adecc::DateFmt::EUROPE>>;
   using GermanNumber = adecc::ConversionChainPolicy<adecc::TrimStringConversionPolicy,
      adecc::GermanDecimalToFloatConversionPolicy<double>>;
   using Positive = adecc::AllOfPolicy<double,adecc::PositiveNumberPolicy<double>>;
   using Amount = adecc::ConvertedPolicyValue<"Amount",GermanNumber,Positive,
      adecc::FloatToGermanDecimalWithThousandsStringPolicy<double,2>>;
   using CheckedAmount = adecc::PolicyValue<"Amount",double,Positive,
      adecc::FloatToGermanDecimalWithThousandsStringPolicy<double,2>>;
   using Email = adecc::ConvertedPolicyValue<"Email",adecc::TrimStringConversionPolicy,adecc::Email_Policy>;
   using URL = adecc::ConvertedPolicyValue<"URL",adecc::TrimStringConversionPolicy,adecc::Url_Policy>;
   using Phone = adecc::ConvertedPolicyValue<"Phone",adecc::TrimStringConversionPolicy,adecc::Phone_Policy>;
   using Bic = adecc::ConvertedPolicyValue<"BIC",adecc::TrimStringConversionPolicy,adecc::Bic_Policy>;
   using Postal = adecc::ConvertedPolicyValue<"Postal",adecc::TrimStringConversionPolicy,adecc::PostalCode_DE_Policy>;
   using UUID = adecc::ConvertedPolicyValue<"UUID",adecc::TrimStringConversionPolicy,adecc::Uuid_Policy>;
   using BeneficiaryPolicy = adecc::AllOfPolicy<std::string,adecc::StringLengthPolicy<1,70>>;
   using Beneficiary = adecc::ConvertedPolicyValue<"Beneficiary",adecc::TrimStringConversionPolicy,BeneficiaryPolicy>;

   constexpr std::array<std::string_view,50> ibans{
      "DE23100000001234567890","DE40100001371234575809","DE57100002741234583728",
      "DE74100004111234591647","DE91100005481234599566","DE11100006851234607485",
      "DE28100008221234615404","DE45100009591234623323","DE62100010961234631242",
      "DE79100012331234639161","DE96100013701234647080","DE16100015071234654999",
      "DE33100016441234662918","DE50100017811234670837","DE67100019181234678756",
      "DE84100020551234686675","DE04100021921234694594","DE21100023291234702513",
      "DE38100024661234710432","DE55100026031234718351","DE72100027401234726270",
      "DE89100028771234734189","DE09100030141234742108","DE26100031511234750027",
      "DE43100032881234757946","DE60100034251234765865","DE77100035621234773784",
      "DE94100036991234781703","DE14100038361234789622","DE31100039731234797541",
      "AT111234567890123456","BE52246913578024","CH7837037036703703703",
      "ES4949382715604938271560","FI0361728394506172","FR4874074073407407407340740",
      "GR8186419752308641975230864","IT2998765431209876543120987","LI8811111111011111111",
      "LU061234567890123456","NO7113580246791","PL18148148146814814814681481",
      "PT64160493825716049382571","SE9817283950461728395046","SI20185185183518518",
      "SK2519753086241975308624","CZ4820987654132098765413","DK0722222222022222",
      "EE462345678991234567","HU22246913578024691357802469"
   };

   void TestIban(Runner& r) {
      for(auto const value : ibans) {
         r.Accept<adecc::Iban>(std::string("IBAN valid ")+std::string(value),value);
      }
      for(auto const value : std::array<std::string_view,15>{
         "DE24100000001234567890","DE41100001371234575809",
         "DE58100002741234583728","DE7410000411123459164",
         "DE911000054812345995660","AT121234567890123456",
         "PL181481481468148148146814","1234567890123456789012",
         "","DE","D","DE2310000000123456789!",
         "de2310000000123456789@","DE2310000000123456789#",
         "DE2310000000123456789/"}) {
         r.Reject<adecc::Iban>(std::string("IBAN invalid ")+std::string(value),value);
      }
      const adecc::Iban normalized{" de23 1000 0000 1234 5678 90 "};
      r.Check(normalized.Value()=="DE23100000001234567890","IBAN normalize whitespace/case");
      r.Check(static_cast<std::string>(normalized)=="DE23 1000 0000 1234 5678 90","IBAN grouping");
      r.Accept<adecc::CheckedIban>("CheckedIban valid normalized",std::string("DE23100000001234567890"));
      r.Reject<adecc::CheckedIban>("CheckedIban rejects checksum",std::string("DE24100000001234567890"));
   }

   void TestDates(Runner& r) {
      r.Accept<BirthISO>("date ISO valid"," 1980-05-23 ");
      r.Accept<BirthDE>("date German valid"," 23.05.1980 ");
      r.Accept<BirthISO>("date lower boundary","1900-01-01");
      r.Accept<FlexibleBirthDate>("flexible ISO branch","2001-06-18");
      r.Accept<FlexibleBirthDate>("flexible German branch","18.06.2001");
      r.Reject<FlexibleBirthDate>("flexible date rejects unknown format","18/06/2001");
      r.Reject<FlexibleBirthDate>("flexible date rejects impossible date","31.02.2001");
      for(auto v : {"1899-12-31","2999-01-01","2025-02-29","2025-13-01","","abc","1900-02-30"})
         r.Reject<BirthISO>(std::string("date ISO invalid ")+v,v);
      for(auto v : {"31.02.2025","32.01.2025","01.13.2025","01.01.1800","01.01.2999","","2025-01-01"})
         r.Reject<BirthDE>(std::string("date German invalid ")+v,v);
      auto const dt=BirthDE{" 23.05.1980 "};
      r.Check(static_cast<std::string>(dt)=="23.05.1980","date output formatting");
      using CheckDate=adecc::PolicyValue<"Birthdate",Date,BirthPolicy>;
      r.Accept<CheckDate>("date native typed",Date{std::chrono::year{2000},std::chrono::month{2},std::chrono::day{29}});
      r.Reject<CheckDate>("date native invalid",Date{std::chrono::year{2025},std::chrono::month{2},std::chrono::day{29}});
   }

   void TestAmounts(Runner& r) {
      r.Accept<Amount>("amount German"," 1.234,56 ");
      const Amount amount{" 1.234,56 "};
      r.Check(std::abs(amount.Value()-1234.56)<0.000001,"amount parse");
      r.Check(static_cast<std::string>(amount)=="1.234,56","amount format thousands");
      r.Accept<CheckedAmount>("amount typed positive",1234.56);
      for(auto v : {"0","-1,20","","abc","1234,56x","1,2,3"})
         r.Reject<Amount>(std::string("amount invalid ")+v,v);
      r.Reject<CheckedAmount>("amount typed zero",0.0);
      r.Reject<CheckedAmount>("amount typed negative",-0.01);
      using Away=adecc::numeric<double,0,adecc::RoundHalfAwayFromZeroPolicy<double>>;
      using Even=adecc::numeric<double,0,adecc::RoundHalfToEvenPolicy<double>>;
      using Toward=adecc::numeric<double,0,adecc::RoundTowardZeroPolicy<double>>;
      r.Check(Away{2.5}.Get()==3.0 && Away{-2.5}.Get()==-3.0,"round half away zero both signs");
      r.Check(Even{2.5}.Get()==2.0 && Even{3.5}.Get()==4.0 && Even{-2.5}.Get()==-2.0,"banker rounding ties");
      r.Check(Toward{2.9}.Get()==2.0 && Toward{-2.9}.Get()==-2.0,"truncate both signs");
   }

   void TestStrings(Runner& r) {
      r.Accept<Email>("email trimmed"," demo@example.org ");
      for(auto v : {"x","@example.org","x@","x@@example.org","x@example","x example@example.org"})
         r.Reject<Email>(std::string("email invalid ")+v,v);
      r.Accept<URL>("https URL","https://example.org/docs?a=1#section");
      r.Accept<URL>("http URL"," http://example.org/ ");
      for(auto v : {"ftp://example.org","javascript:alert(1)","https://","https://localhost",""})
         r.Reject<URL>(std::string("URL invalid ")+v,v);
      r.Accept<Phone>("E164 number","+491776203549");
      for(auto v : {"00491776203549","01776203549","+0","+49-1776203549","+49 (177) 6203549",""})
         r.Reject<Phone>(std::string("E164 invalid ")+v,v);
      r.Accept<Bic>("BIC eight","DEUTDEFF");
      r.Accept<Bic>("BIC eleven","DEUTDEFF500");
      for(auto v : {"DEUTDEF","DEUTDEFF50","deutdeff","DEU1DEFF",""})
         r.Reject<Bic>(std::string("BIC invalid ")+v,v);
      r.Accept<Postal>("postal five","10115");
      for(auto v : {"1011","101150","AB115","10 15",""})
         r.Reject<Postal>(std::string("postal invalid ")+v,v);
      r.Accept<UUID>("UUID v4","550e8400-e29b-41d4-a716-446655440000");
      for(auto v : {"550e8400-e29b-61d4-a716-446655440000","550e8400-e29b-41d4-0716-446655440000","550e8400e29b41d4a716446655440000",""})
         r.Reject<UUID>(std::string("UUID invalid ")+v,v);
      r.Accept<Beneficiary>("beneficiary simple","Valid Test Name");
      r.Reject<Beneficiary>("beneficiary empty","");
      r.Reject<Beneficiary>("beneficiary too long",std::string(71,'X'));
      r.Accept<Beneficiary>("beneficiary upper length",std::string(70,'X'));
   }

   void TestSepaAndVat(Runner& r) {
      // The current SEPA charset/transliteration APIs remain in adecc::old;
      // new generic PolicyValue APIs supply the reusable length validation.
      using SepaLength=adecc::StringLengthPolicy<1,70>;
      using SepaCharset=adecc::old::SepaLatinCharacterPolicy<>;
      r.Check(SepaLength::IsValid("Payment / Invoice: 2026") &&
         SepaCharset::IsValid("Payment / Invoice: 2026"),"SEPA allowed ASCII");
      r.Check(!SepaLength::IsValid(""),"SEPA rejects empty");
      r.Check(!SepaCharset::IsValid("Name@domain"),"SEPA rejects @");
      r.Check(!SepaLength::IsValid(std::string(71,'A')),"SEPA rejects long");
      auto const normalized=adecc::old::GermanSepaStringConversionPolicy::Convert("Müller Straße");
      r.Check(normalized=="Mueller Strasse","SEPA German transliteration");
      using AnyNumeric=adecc::AnyOfPolicy<double,adecc::PositiveNumberPolicy<double>,
         adecc::NonNegativeFloatingPointPolicy<double>>;
      r.Check(AnyNumeric::IsValid(0.0) && AnyNumeric::IsValid(2.0) && !AnyNumeric::IsValid(-1.0),
         "AnyOf positive/nonnegative policies");
      r.Accept<adecc::VatId_DE>("German VAT ID valid","DE123456788");
      r.Accept<adecc::VatId_DE>("German VAT ID normalized"," de 123 456 788 ");
      r.Accept<adecc::VatId_DE_Grouped>("German VAT ID grouped","DE123456788");
      for(auto v : {"DE123456789","DE12345678","FR123456788","DEABCDEFGHI","","DE12A456788"})
         r.Reject<adecc::VatId_DE>(std::string("German VAT ID invalid ")+v,v);
   }
}

int main() {
   Runner r;
   try {
      TestIban(r); TestDates(r); TestAmounts(r); TestStrings(r); TestSepaAndVat(r);
   } catch(std::exception const& e) {
      r.Check(false,std::format("Unhandled exception: {}",e.what()));
   }
   std::cout << std::format("[RESULT] {}/{} passed, {} failed\n",r.total-r.failed,r.total,r.failed);
   return r.failed==0 ? 0 : 1;
}
