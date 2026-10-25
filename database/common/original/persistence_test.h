#pragma once

#include "database.h"
#include "system_data_persistent.h"
#include "stream_tools.h"

#include <string>
#include <string_view>
#include <optional>
#include <algorithm>
#include <numeric>

#include <exception>

using testdbneu_list = adecc::defined_type_list<
   int,                  // Id
   std::string,          // Name
   std::string,          // Kommentar
   int,                  // Wert
   std::optional<int>    // WertDoppelt, berechnetes readonly Feld
   >;


inline adecc::vecCaptions<adecc::AnsiStreamPolicy> testdbneu_captions = {
	 { "Id",            80, adecc::EAlignmentType::right },
	 { "Name",         180, adecc::EAlignmentType::left  },
	 { "Kommentar",    250, adecc::EAlignmentType::left  },
	 { "Wert",         100, adecc::EAlignmentType::right },
	 { "WertDoppelt",  120, adecc::EAlignmentType::right }
  };


struct TestDbNeuMetaData {
   static constexpr std::string_view svTableName = "dbo.TestDbNeu";

   static constexpr std::array<std::string_view, 5> arrAttributeNames {
		  "Id", "Name", "Kommentar", "Wert", "WertDoppelt"
	  };

   static constexpr std::array<std::size_t, 1> arrKeyIndices { 0 };
   static constexpr std::optional<std::size_t> optIdentityIndex { 0 };
   static constexpr std::array<std::size_t, 1> arrReadOnlyIndices { 4 };
};


class TTestDbNeu : public adecc::db::PersistentSystemData<testdbneu_list, TestDbNeuMetaData> {
public:
   using Base = adecc::db::PersistentSystemData<testdbneu_list, TestDbNeuMetaData>;
   using Base::Base;
   using Base::operator=;

   explicit TTestDbNeu(int const iId) {
	  Set<0>(iId);
	  }

   auto operator <=> (TTestDbNeu const& rhs) const {
	  return CompareSelected<0>(rhs.Data());
	  }

   bool operator == (TTestDbNeu const& rhs) const {
	  return EqualSelected<0>(rhs);
	  }

   auto const& Id() const { return Get<0>(); }
   auto const& Name() const { return Get<1>(); }
   auto const& Kommentar() const { return Get<2>(); }
   auto const& Wert() const { return Get<3>(); }
   auto const& WertDoppelt() const { return Get<4>(); }

   template <typename val_ty> requires std::assignable_from<element_ty<0>&, val_ty>
   void Id(val_ty&& theValue) { Set<0>(std::forward<val_ty>(theValue)); }

   template <typename val_ty> requires std::assignable_from<element_ty<1>&, val_ty>
   void Name(val_ty&& theValue) { Set<1>(std::forward<val_ty>(theValue)); }

   template <typename val_ty> requires std::assignable_from<element_ty<2>&, val_ty>
   void Kommentar(val_ty&& theValue) { Set<2>(std::forward<val_ty>(theValue)); }

   template <typename val_ty> requires std::assignable_from<element_ty<3>&, val_ty>
   void Wert(val_ty&& theValue) { Set<3>(std::forward<val_ty>(theValue)); }

   template <typename val_ty> requires std::assignable_from<element_ty<4>&, val_ty>
   void WertDoppelt(val_ty&& theValue) { Set<4>(std::forward<val_ty>(theValue)); }
};


using testdbneu_tuple_ty = typename TTestDbNeu::data_ty;


/*!
\brief Prüft die von PersistentSystemData erzeugten SQL-Anweisungen und Output-Parameter
\details
   Der Test stellt sicher, dass Identity und readonly Felder nicht in den
   schreibenden SQL-Teilen erscheinen, die Datenstruktur aber vollständig bleibt.

\param os Ausgabestrom für Testmeldungen
*/
void TestPersistentMetadata(std::ostringstream& os) {
   std::string const strInsertSql = TTestDbNeu::CreateInsertSql();
   std::string const strUpdateSql = TTestDbNeu::CreateUpdateSql();

   std::string const strExpectedInsertSql {
	  "INSERT INTO dbo.TestDbNeu (Name, Kommentar, Wert) "
	  "VALUES (:Name, :Kommentar, :Wert)"
	  };

   std::string const strExpectedUpdateSql {
	  "UPDATE dbo.TestDbNeu "
	  "SET Name = :Name, Kommentar = :Kommentar, Wert = :Wert "
	  "WHERE Id = :keyId"
	  };

   if (strInsertSql != strExpectedInsertSql) {
      throw std::runtime_error {
         std::format(
            "TestPersistentMetadata: Insert SQL passt nicht.\nIst: {}\nSoll: {}",
            strInsertSql,
            strExpectedInsertSql
			)
         };
      }

   if (strUpdateSql != strExpectedUpdateSql) {
      throw std::runtime_error {
         std::format(
            "TestPersistentMetadata: Update SQL passt nicht.\nIst: {}\nSoll: {}",
            strUpdateSql,
            strExpectedUpdateSql
            )
         };
      }

   auto vecInsertParams = TTestDbNeu::CreateInsertOutputParameters();
   auto vecUpdateParams = TTestDbNeu::CreateUpdateOutputParameters();

   if (vecInsertParams.size() != std::tuple_size_v<testdbneu_tuple_ty>) {
      throw std::runtime_error {
         "TestPersistentMetadata: Insert Output Parameteranzahl passt nicht"
         };
      }

   if (vecUpdateParams.size() != std::tuple_size_v<testdbneu_tuple_ty>) {
      throw std::runtime_error {
         "TestPersistentMetadata: Update Output Parameteranzahl passt nicht"
         };
      }

   auto const& [strInsertIdName, aInsertIdRole] = vecInsertParams[0];
   auto const& [strInsertReadonlyName, aInsertReadonlyRole] = vecInsertParams[4];

   if (strInsertIdName != "Id" ||
       aInsertIdRole != adecc::db_output_param_role::identity) {
      throw std::runtime_error {
         "TestPersistentMetadata: Insert Identity Parameter ist falsch"
         };
      }

   if (strInsertReadonlyName != "WertDoppelt" ||
       aInsertReadonlyRole != adecc::db_output_param_role::may_be_missing) {
      throw std::runtime_error {
         "TestPersistentMetadata: Insert readonly Parameter ist falsch"
         };
      }

   auto const& [strUpdateKeyName, aUpdateKeyRole] = vecUpdateParams[0];
   auto const& [strUpdateReadonlyName, aUpdateReadonlyRole] = vecUpdateParams[4];

   if (strUpdateKeyName != "keyId" ||
       aUpdateKeyRole != adecc::db_output_param_role::key) {
      throw std::runtime_error {
         "TestPersistentMetadata: Update Key Parameter ist falsch"
         };
      }

   if (strUpdateReadonlyName != "WertDoppelt" ||
       aUpdateReadonlyRole != adecc::db_output_param_role::may_be_missing) {
      throw std::runtime_error {
         "TestPersistentMetadata: Update readonly Parameter ist falsch"
         };
      }

   std::println(os, "PersistentSystemData Metadata Test erfolgreich.");
   std::println(os, "Insert SQL: {}", strInsertSql);
   std::println(os, "Update SQL: {}", strUpdateSql);
   }


/*!
\brief Löscht und erzeugt die Testtabelle neu
\details
   Die Tabelle enthält eine Identity-Spalte und ein berechnetes readonly Feld.

\tparam app_db Logischer Datenbanktyp
\param db Datenbank
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void CreatePersistentTestTable(app_db& db, std::ostringstream& os) {
   db.ExecuteCommand(
      "if object_id('dbo.TestDbNeu', 'U') is not null "
      "drop table dbo.TestDbNeu; "
      "create table dbo.TestDbNeu "
      "( "
      "   Id int identity(1, 1) not null primary key, "
      "   Name nvarchar(100) not null, "
      "   Kommentar nvarchar(200) null, "
      "   Wert int not null, "
      "   WertDoppelt as (Wert * 2) "
      ")"
   );

   std::println(os, "Tabelle dbo.TestDbNeu wurde neu erzeugt.");
   }


/*!
\brief Erzeugt Testobjekte nach dem PersistentSystemData Muster
\details
   Die Identity ist initial 0. Das readonly Feld bleibt \c std::nullopt,
   weil es nicht gesetzt werden darf.

\returns Testdaten
*/
std::vector<TTestDbNeu> CreatePersistentInputRows() {
   std::vector<TTestDbNeu> vecRows;

   TTestDbNeu aAlpha;
   aAlpha.Id(0);
   aAlpha.Name("Alpha");
   aAlpha.Kommentar("Kommentar Alpha");
   aAlpha.Wert(10);
   aAlpha.WertDoppelt(std::nullopt);
   vecRows.emplace_back(aAlpha);

   TTestDbNeu aBeta;
   aBeta.Id(0);
   aBeta.Name("Beta");
   aBeta.Kommentar("Kommentar Beta");
   aBeta.Wert(20);
   aBeta.WertDoppelt(std::nullopt);
   vecRows.emplace_back(aBeta);

   TTestDbNeu aGamma;
   aGamma.Id(0);
   aGamma.Name("Gamma");
   aGamma.Kommentar("Kommentar Gamma");
   aGamma.Wert(30);
   aGamma.WertDoppelt(std::nullopt);
   vecRows.emplace_back(aGamma);

   return vecRows;
   }


/*!
\brief Wandelt PersistentSystemData Objekte in vollständige Tupel um
\details
   Die Tupel bleiben vollständig und positionsstabil. Es wird kein reduziertes
   Insert- oder Update-Tupel erzeugt.

\param vecRows Fachliche Testobjekte
\returns Vollständiger Tuple-Range
*/
std::vector<testdbneu_tuple_ty> ToTupleRange(
   std::vector<TTestDbNeu> const& vecRows
) {
   std::vector<testdbneu_tuple_ty> vecTuples;
   vecTuples.reserve(vecRows.size());

   for (auto const& aRow : vecRows) {
      vecTuples.emplace_back(aRow.Data());
      }

   return vecTuples;
   }


/*!
\brief Fügt Testdaten über die PersistentSystemData Metadaten ein
\details
   Der Insert nutzt OutputRange, damit die Identity in das vollständige Tuple
   zurückgeschrieben wird.

\tparam app_db Logischer Datenbanktyp
\param db Datenbank
\param os Ausgabestrom für Testmeldungen
\returns Eingefügte Tupel inklusive Identity
*/
template <adecc::db::logical_database_type app_db>
std::vector<testdbneu_tuple_ty> InsertPersistentRows(
   app_db& db,
   std::ostringstream& os
) {
   std::vector<TTestDbNeu> vecRows = CreatePersistentInputRows();
   std::vector<testdbneu_tuple_ty> vecInput = ToTupleRange(vecRows);

   auto aRange = db.template MakeOutputRange<
      int,
      std::string,
      std::string,
      int,
      std::optional<int>
   >(
      TTestDbNeu::CreateInsertSql(),
      TTestDbNeu::CreateInsertOutputParameters()
   );

   std::vector<testdbneu_tuple_ty> vecInserted;

   for (auto const& tupRow : aRange(vecInput)) {
      vecInserted.emplace_back(tupRow);
      }

   if (vecInserted.size() != vecInput.size()) {
      throw std::runtime_error {
         "InsertPersistentRows: Anzahl der eingefügten Zeilen stimmt nicht"
         };
      }

   for (std::size_t uIndex{}; uIndex < vecInserted.size(); ++uIndex) {
      testdbneu_tuple_ty const& tupInput = vecInput[uIndex];
      testdbneu_tuple_ty const& tupOutput = vecInserted[uIndex];

      if (std::get<0>(tupOutput) <= 0) {
         throw std::runtime_error {
            "InsertPersistentRows: Identity wurde nicht zurückgeschrieben"
            };
         }

      if (std::get<1>(tupOutput) != std::get<1>(tupInput) ||
          std::get<2>(tupOutput) != std::get<2>(tupInput) ||
          std::get<3>(tupOutput) != std::get<3>(tupInput)) {
         throw std::runtime_error {
            "InsertPersistentRows: Nicht-Identity Werte wurden unerwartet verändert"
            };
         }

      std::println(
         os,
         "Inserted: Id={}, Name={}, Kommentar={}, Wert={}",
         std::get<0>(tupOutput),
         std::get<1>(tupOutput),
         std::get<2>(tupOutput),
         std::get<3>(tupOutput)
      );
      }

   return vecInserted;
   }


/*!
\brief Aktualisiert Testdaten über die PersistentSystemData Metadaten
\details
   Der Update nutzt OutputSink. Schlüssel werden über \c key Präfix gebunden,
   readonly Felder bleiben vollständig im Tuple, fehlen aber im SQL.

\tparam app_db Logischer Datenbanktyp
\param db Datenbank
\param vecInserted Eingefügte Tupel inklusive Identity
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void UpdatePersistentRows(app_db& db, std::vector<testdbneu_tuple_ty> const& vecInserted,
						  std::ostringstream& os) {
   if (vecInserted.size() < 3) {
	  throw std::runtime_error { "UpdatePersistentRows: Zu wenige eingefügte Zeilen" };
	  }

   TTestDbNeu aAlpha;
   aAlpha = vecInserted[0];
   aAlpha.Name("Alpha update");
   aAlpha.Wert(110);

   TTestDbNeu aGamma;
   aGamma = vecInserted[2];
   aGamma.Name("Gamma update");
   aGamma.Wert(330);

   std::vector<TTestDbNeu> vecRows {  aAlpha, aGamma };

   std::vector<testdbneu_tuple_ty> vecUpdate = ToTupleRange(vecRows);

   auto aSink = db.template MakeOutputSink<int, std::string, std::string, int, std::optional<int>>(
											TTestDbNeu::CreateUpdateSql(),
											TTestDbNeu::CreateUpdateOutputParameters() );

   aSink = vecUpdate;

   for (auto const& tupRow : vecUpdate) {
	  std::println(
		 os,
		 "Updated: Id={}, Name={}, Wert={}",
		 std::get<0>(tupRow),
		 std::get<1>(tupRow),
		 std::get<3>(tupRow)
	  );
	  }
   }


/*!
\brief Prüft die persistierten Daten
\details
   Es wird geprüft, dass die Updates korrekt durchgeführt wurden und das
   berechnete readonly Feld aus der Datenbank gelesen werden kann.

\tparam app_db Logischer Datenbanktyp
\param db Datenbank
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void VerifyPersistentRows(app_db& db, std::ostringstream& os) {
   auto rngRows = db.template Execute<
      int,
      std::string,
      std::optional<std::string>,
      int,
      std::optional<int>
   >(
      TTestDbNeu::CreateSelectFromSql() + " order by Id"
   );

   std::vector<std::tuple<
      int,
      std::string,
      std::optional<std::string>,
      int,
      std::optional<int>
   >> vecRows;

   for (auto const& tupRow : rngRows) {
      vecRows.emplace_back(tupRow);
      }

   if (vecRows.size() != 3) {
      throw std::runtime_error {
         "VerifyPersistentRows: Es wurden nicht exakt drei Zeilen gefunden"
         };
      }

   if (std::get<1>(vecRows[0]) != "Alpha update" ||
       std::get<3>(vecRows[0]) != 110 ||
       !std::get<4>(vecRows[0]) ||
       *std::get<4>(vecRows[0]) != 220) {
      throw std::runtime_error {
         "VerifyPersistentRows: Erste Zeile passt nicht"
         };
      }

   if (std::get<1>(vecRows[1]) != "Beta" ||
       std::get<3>(vecRows[1]) != 20 ||
       !std::get<4>(vecRows[1]) ||
       *std::get<4>(vecRows[1]) != 40) {
      throw std::runtime_error {
         "VerifyPersistentRows: Zweite Zeile passt nicht"
         };
      }

   if (std::get<1>(vecRows[2]) != "Gamma update" ||
       std::get<3>(vecRows[2]) != 330 ||
       !std::get<4>(vecRows[2]) ||
       *std::get<4>(vecRows[2]) != 660) {
      throw std::runtime_error {
         "VerifyPersistentRows: Dritte Zeile passt nicht"
         };
      }

   std::println(os, "VerifyPersistentRows: Alle Prüfungen erfolgreich.");

   for (auto const& [iId, strName, optKommentar, iWert, optWertDoppelt] : vecRows) {
      std::println(
         os,
         "Row: Id={}, Name={}, Kommentar={}, Wert={}, WertDoppelt={}",
         iId,
         strName,
         optKommentar.value_or("<NULL>"),
         iWert,
         optWertDoppelt ? std::to_string(*optWertDoppelt) : "<NULL>"
      );
      }
   }


/*!
\brief Führt den vollständigen PersistentSystemData Test aus
\details
   Der Test folgt dem Muster der fachlichen PersistentSystemData Klassen:
   Metadaten, Fachklasse, vollständige Tupel und Persistenz über OutputRange
   beziehungsweise OutputSink.

\tparam app_db Logischer Datenbanktyp
\param db Datenbank
\returns Testprotokoll
*/
template <adecc::db::logical_database_type app_db>
std::string RunPersistentSystemDataTests(app_db& db) {
   std::ostringstream os;

   auto aTransaction = db.Transaction();

   TestPersistentMetadata(os);

   CreatePersistentTestTable(db, os);

   std::vector<testdbneu_tuple_ty> vecInserted =
	  InsertPersistentRows(db, os);

   UpdatePersistentRows(db, vecInserted, os);

   VerifyPersistentRows(db, os);

   aTransaction.Commit();

   std::println(os, "");
   std::println(os, "PersistentSystemData Tests erfolgreich abgeschlossen.");

   return os.str();
   }
