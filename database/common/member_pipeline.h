// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/**
\file member_pipeline.h
\brief Database-neutral membership / contribution integration workload.
\details The same typed CSV sources, transformations, persistent metadata,
logical query operations and console grid run with any backend implementing
adecc::db::logical_database. Only DDL and server bootstrap are driver-specific.
The input files contain fabricated test records, never real member data.
*/
#include <adecc/cpp_core/database.h>
#include <adecc/cpp_core/type_lists.h>
#include <adecc/cpp_core/type_transformations.h>
#include <adecc/cpp_core/file_as_tuple.h>
#include <adecc/cpp_core/system_data_persistent.h>
#include <adecc/cpp_core/text_grid_wrapper.h>
#include <adecc/cpp_domain/sepa_policies_hawkers_tray.h>
#include "member_field_policies.h"
#include <pugixml.hpp>
#include <array>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace buildengine_tests::database::members {

using Iban_ty = adecc::Iban;
using Tax_ty = adecc::VatId_DE; // German validated tax/VAT identification policy

using anreden_ty = adecc::defined_type_list<
   int,std::string,std::string,std::string>;
using mitglied_ty = adecc::defined_type_list<
   int,std::string,std::string,std::string,int,adecc::date_ty,
   std::string,std::string,std::string,std::string,
   adecc::date_ty,adecc::money_ty,std::string,std::string>;
using buchung_ty = adecc::defined_type_list<
   int,int,adecc::date_ty,adecc::money_ty,std::string>;

using person_csv_ty = adecc::defined_type_list<
   std::string,std::string,std::string,std::string,Geburtstag_ty,
   std::string,std::string,std::string,std::string,
   adecc::date_ty,Beitrag_ty,Iban_ty,Tax_ty>;
using person_source_ty = adecc::defined_type_list<
   int,std::string,std::string,std::string,std::string,Geburtstag_ty,
   std::string,std::string,std::string,std::string,
   adecc::date_ty,Beitrag_ty,Iban_ty,Tax_ty>;
using booking_csv_ty = adecc::defined_type_list<
   std::string,adecc::date_ty,Beitrag_ty,Iban_ty>;
using booking_source_ty = adecc::defined_type_list<
   int,std::string,adecc::date_ty,Beitrag_ty,Iban_ty>;

struct AnredeMeta {
   static constexpr std::string_view svTableName="Anreden";
   static constexpr std::array<std::string_view,4> arrAttributeNames{
      "ID","Bezeichnung","Kurztext","Anrede"};
   static constexpr std::array<std::size_t,1> arrKeyIndices{0};
   static constexpr std::optional<std::size_t> optIdentityIndex{0};
   static constexpr std::array<std::size_t,0> arrReadOnlyIndices{};
};
struct MitgliedMeta {
   static constexpr std::string_view svTableName="Mitglied";
   static constexpr std::array<std::string_view,14> arrAttributeNames{
      "ID","MitgliedNr","Name","Vorname","Anrede","Geburtstag",
      "Strasse","Hausnummer","PLZ","Ort","Beginn","Beitrag","IBAN","Steuer"};
   static constexpr std::array<std::size_t,1> arrKeyIndices{0};
   static constexpr std::optional<std::size_t> optIdentityIndex{0};
   static constexpr std::array<std::size_t,0> arrReadOnlyIndices{};
};
struct BuchungMeta {
   static constexpr std::string_view svTableName="Buchungen";
   static constexpr std::array<std::string_view,5> arrAttributeNames{
      "ID","MitgliedID","PerDat","Betrag","IBAN"};
   static constexpr std::array<std::size_t,1> arrKeyIndices{0};
   static constexpr std::optional<std::size_t> optIdentityIndex{0};
   static constexpr std::array<std::size_t,0> arrReadOnlyIndices{};
};
using TAnrede=adecc::db::PersistentSystemData<anreden_ty,AnredeMeta>;
using TMitglied=adecc::db::PersistentSystemData<mitglied_ty,MitgliedMeta>;
using TBuchung=adecc::db::PersistentSystemData<buchung_ty,BuchungMeta>;

using salutation_ids_ty=std::map<std::string,int>;
using member_ids_ty=std::map<std::string,int>;
using member_begins_ty=std::map<std::string,adecc::date_ty>;

inline void Require(bool condition,std::string_view reason) {
   if(!condition)throw std::runtime_error{std::string{reason}};
}

[[nodiscard]] inline std::vector<anreden_ty::type_list> ReadSalutations(
   std::filesystem::path const& file) {
   pugi::xml_document xml;
   std::string path=file.string();
   auto status=xml.load_file(path.c_str());
   if(!status)throw std::runtime_error{std::format("Cannot read {}: {}",path,status.description())};
   auto root=xml.child("Anreden");
   if(!root)throw std::runtime_error{"Anreden.xml: missing Anreden element"};
   std::vector<anreden_ty::type_list> rows;
   for(auto node:root.children("Eintrag")) {
      auto read=[&](char const* name)->std::string {
         auto child=node.child(name);
         if(!child)throw std::runtime_error{std::format("Anreden.xml missing {}",name)};
         return child.text().as_string();
      };
      rows.emplace_back(0,read("Bezeichnung"),read("Kurztext"),read("Anrede"));
   }
   Require(rows.size()>=3,"Anreden.xml needs at least three entries");
   return rows;
}

template<adecc::db::logical_database_type DB>
[[nodiscard]] salutation_ids_ty LoadSalutations(DB& db,
   std::filesystem::path const& xmlFile) {
   auto source=ReadSalutations(xmlFile);
   auto output=anreden_ty::invoke([&]<class... T>() {
      return db.template MakeOutputRange<T...>(
         TAnrede::CreateInsertSql(),TAnrede::CreateInsertOutputParameters());
   });
   salutation_ids_ty ids;
   for(auto const& row:output(source)) {
      Require(std::get<0>(row)>0,"Anreden generated identity absent");
      Require(ids.emplace(std::get<1>(row),std::get<0>(row)).second,
         "Duplicate salutation");
   }
   Require(ids.size()==source.size(),"Salutations not completely inserted");
   return ids;
}

template<adecc::db::logical_database_type DB>
[[nodiscard]] std::tuple<member_ids_ty,member_begins_ty,std::size_t>
LoadMembers(DB& db,std::filesystem::path const& csvFile,
   salutation_ids_ty const& salutations) {
   auto source=adecc::FromCsvFileTyped<false,true,person_csv_ty>(
      csvFile,";",true)
      |std::views::transform([](auto&& row){
         return std::tuple_cat(std::tuple{0},std::move(row));
      })
      |std::views::transform([&](auto&& row) {
         return adecc::Transform<person_source_ty,mitglied_ty>(
            std::move(row),
            adecc::BindMapping<4,person_source_ty,mitglied_ty>(salutations),
            adecc::BindMapping<12,person_source_ty,mitglied_ty>(
               [](Iban_ty const& iban){return iban.Value();}),
            adecc::BindMapping<13,person_source_ty,mitglied_ty>(
               [](Tax_ty const& tax){return tax.Value();}));
      });
   auto output=mitglied_ty::invoke([&]<class... T>() {
      return db.template MakeOutputRange<T...>(
         TMitglied::CreateInsertSql(),TMitglied::CreateInsertOutputParameters());
   });
   member_ids_ty ids;
   member_begins_ty begins;
   std::size_t count{};
   for(auto const& row:output(source)) {
      Require(std::get<0>(row)>0,"Member generated identity absent");
      auto const& number=std::get<1>(row);
      Require(ids.emplace(number,std::get<0>(row)).second,
         "Duplicate member number");
      begins.emplace(number,std::get<10>(row));
      ++count;
   }
   Require(count==50,"Exactly 50 synthetic member records expected");
   return {std::move(ids),std::move(begins),count};
}

template<adecc::db::logical_database_type DB>
[[nodiscard]] std::size_t LoadBookings(DB& db,
   std::filesystem::path const& csvFile,
   member_ids_ty const& memberIds,member_begins_ty const& begins) {
   // Validate the source first, then present the same lazy transformed rows
   // to the generic SQL output sink.
   auto csv=adecc::FromCsvFileTyped<false,true,booking_csv_ty>(csvFile,";",true)
      |std::views::transform([&](auto&& row){
         auto const& number=std::get<0>(row);
         auto pos=begins.find(number);
         if(pos==begins.end())throw std::runtime_error{"Booking references unknown member"};
         Require(!(std::get<1>(row)<pos->second),"Booking precedes membership start");
         return std::tuple_cat(std::tuple{0},std::move(row));
      })
      |std::views::transform([&](auto&& row){
         return adecc::Transform<booking_source_ty,buchung_ty>(
            std::move(row),
            adecc::BindMapping<1,booking_source_ty,buchung_ty>(memberIds),
            adecc::BindMapping<4,booking_source_ty,buchung_ty>(
               [](Iban_ty const& iban){return iban.Value();}));
      });
   auto output=buchung_ty::invoke([&]<class... T>(){
      return db.template MakeOutputRange<T...>(
         TBuchung::CreateInsertSql(),TBuchung::CreateInsertOutputParameters());
   });
   std::size_t count{};
   for(auto const& row:output(csv)) {
      Require(std::get<0>(row)>0,"Booking generated identity absent");
      ++count;
   }
   Require(count>=150,"Booking fixture unexpectedly short");
   return count;
}

template<adecc::db::logical_database_type DB>
void PrintContributions(DB& db,std::size_t memberCount,std::size_t bookingCount) {
   auto memberRows=db.template ExecuteOne<int>("SELECT COUNT(*) FROM Mitglied");
   auto bookingRows=db.template ExecuteOne<int>("SELECT COUNT(*) FROM Buchungen");
   Require(memberRows&&std::get<0>(*memberRows)==static_cast<int>(memberCount),
      "Member count mismatch");
   Require(bookingRows&&std::get<0>(*bookingRows)==static_cast<int>(bookingCount),
      "Booking count mismatch");

   // The IBAN on a booking is deliberately a historical snapshot.
   auto changed=db.template ExecuteOne<int>(
      "SELECT COUNT(*) FROM Buchungen b JOIN Mitglied m ON m.ID=b.MitgliedID "
      "WHERE b.IBAN<>m.IBAN");
   Require(changed&&std::get<0>(*changed)>0,
      "Expected at least one historical IBAN change");

   using report_ty=adecc::defined_type_list<
      std::string,std::string,std::string,adecc::date_ty,adecc::money_ty>;
   auto result=db.template Execute<
      std::string,std::string,std::string,adecc::date_ty,adecc::money_ty>(
      "SELECT a.Bezeichnung, m.Vorname, m.Name, m.Geburtstag, "
      "COALESCE(SUM(b.Betrag),0) AS Beitragssumme "
      "FROM Mitglied m JOIN Anreden a ON a.ID=m.Anrede "
      "LEFT JOIN Buchungen b ON b.MitgliedID=m.ID "
      "GROUP BY m.ID,a.Bezeichnung,m.Vorname,m.Name,m.Geburtstag "
      "ORDER BY m.MitgliedNr");

   // Consume the logical DB generator as a typed range into the same
   // adecc Grid abstraction also used for Excel or GUI backends.
   using backend_ty=adecc::grid::OStreamGridBackend<>;
   using captions_ty=std::vector<std::tuple<
      std::string,int,adecc::EAlignmentType>>;
   captions_ty captions{
      {"Anrede",12,adecc::EAlignmentType::left},
      {"Vorname",18,adecc::EAlignmentType::left},
      {"Name",20,adecc::EAlignmentType::left},
      {"Geburtstag",12,adecc::EAlignmentType::left},
      {"Beitragssumme",15,adecc::EAlignmentType::right}
   };
   std::cout<<"\n[REPORT] Contributions per member (EUR)\n";
   {
      adecc::grid::WriteGridModel<backend_ty> grid{
         backend_ty{std::cout,backend_ty::TableSeparators()},captions};
      for(auto const& row:result)grid+=row;
      Require(grid.rows()==memberCount,"Grid row count does not match members");
   }
   std::cout<<"\n[PASS] members="<<memberCount
      <<" bookings="<<bookingCount
      <<" historical-IBAN="<<std::get<0>(*changed)<<"\n";
}

/**
\brief Database-neutral import + report sequence; dialect handles SQL DDL.
\details Schema changes are made before DML transactions because MariaDB DDL
implicitly commits. A failure never reports PASS, and the next run drops
the dependent tables in a safe order.
*/
template<adecc::db::logical_database_type DB,class Dialect>
void Run(DB& db,Dialect const& dialect,std::filesystem::path const& files) {
   for(auto const& name:{"Anreden.xml","Mitglieder.csv","Buchungen.csv"}) {
      Require(std::filesystem::is_regular_file(files/name),
         std::format("Missing fixture: {}",(files/name).string()));
   }
   dialect.RecreateTables(db);
   auto salutations=LoadSalutations(db,files/"Anreden.xml");
   auto [ids,begins,memberCount]=LoadMembers(db,files/"Mitglieder.csv",salutations);
   auto bookingCount=LoadBookings(db,files/"Buchungen.csv",ids,begins);
   PrintContributions(db,memberCount,bookingCount);
}
} // namespace buildengine_tests::database::members
