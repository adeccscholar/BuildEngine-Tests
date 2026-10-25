#pragma once

#include "database.h"
#include "system_data_persistent.h"

#include <algorithm>
#include <numeric>
#include <exception>
#include <iostream>
#include <optional>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <array>

using test_row_ty = std::tuple<int, std::string, std::string, int>;


/*!
\brief Löscht und erzeugt die Testtabelle neu
\details
   Die Tabelle enthält eine Identity-Spalte als Primary Key und drei Nutzfelder.

\param db Logische Datenbank
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void CreateTestTable(app_db& db, std::ostream& os) {
   db.ExecuteCommand(
	  "if object_id('dbo.TestDbNeu', 'U') is not null "
	  "drop table dbo.TestDbNeu; "
	  "create table dbo.TestDbNeu "
	  "( "
	  "   Id int identity(1, 1) not null primary key, "
	  "   Name nvarchar(100) not null, "
	  "   Kommentar nvarchar(200) null, "
	  "   Wert int not null "
	  ")"
   );

   std::println(os, "Tabelle dbo.TestDbNeu wurde neu erzeugt.");
   }


/*!
\brief Fügt Testdaten mit OutputRange ein
\details
   Die Identity-Spalte \c Id wird im Insert nicht gebunden. Nach jedem Insert
   wird sie aus dem Backend gelesen und in das Output-Tuple geschrieben.

\param db Logische Datenbank
\param os Ausgabestrom für Testmeldungen
\returns Eingefügte Datensätze inklusive Identity
\throw std::runtime_error wenn eine Testbedingung fehlschlägt
*/
template <adecc::db::logical_database_type app_db>
std::vector<test_row_ty> InsertRows(app_db& db, std::ostream& os) {
   std::vector<test_row_ty> vecInput {
	  { 0, "Alpha", "Kommentar Alpha", 10 },
	  { 0, "Beta",  "Kommentar Beta",  20 },
	  { 0, "Gamma", "Kommentar Gamma", 30 }
	  };

   auto aRange = db.template MakeOutputRange<int, std::string, std::string, int>(
	  "insert into dbo.TestDbNeu "
	  "(Name, Kommentar, Wert) "
	  "values "
	  "(:Name, :Kommentar, :Wert)",
	  {
		 { "Id",        adecc::db_output_param_role::identity },
		 { "Name",      adecc::db_output_param_role::needed_value },
		 { "Kommentar", adecc::db_output_param_role::needed_value },
		 { "Wert",      adecc::db_output_param_role::needed_value }
	  }
   );

   std::vector<test_row_ty> vecInserted;

   for (auto const& tupRow : aRange(vecInput)) {
	  vecInserted.emplace_back(tupRow);
	  }

   if (vecInserted.size() != vecInput.size()) {
	  throw std::runtime_error {
		 "InsertRows: Anzahl der eingefügten Zeilen stimmt nicht"
		 };
	  }

   for (std::size_t uIndex{}; uIndex < vecInserted.size(); ++uIndex) {
	  test_row_ty const& tupInput = vecInput[uIndex];
	  test_row_ty const& tupOutput = vecInserted[uIndex];

	  if (std::get<0>(tupOutput) <= 0) {
		 throw std::runtime_error {
			"InsertRows: Identity wurde nicht zurückgeschrieben"
			};
		 }

	  if (std::get<1>(tupOutput) != std::get<1>(tupInput)) {
		 throw std::runtime_error {
			"InsertRows: Name wurde unerwartet verändert"
			};
		 }

	  if (std::get<2>(tupOutput) != std::get<2>(tupInput)) {
		 throw std::runtime_error {
			"InsertRows: Kommentar wurde unerwartet verändert"
			};
		 }

	  if (std::get<3>(tupOutput) != std::get<3>(tupInput)) {
		 throw std::runtime_error {
			"InsertRows: Wert wurde unerwartet verändert"
			};
		 }

	  std::println(os, "Inserted: Id={}, Name={}, Kommentar={}, Wert={}",
		 std::get<0>(tupOutput), std::get<1>(tupOutput), std::get<2>(tupOutput),
		 std::get<3>(tupOutput)
	    );
	  }

   return vecInserted;
   }


/*!
\brief Aktualisiert ausgewählte Zeilen mit direktem Range-Aufruf
\details
   Der vollständige Tuple-Range wird verwendet. Das SQL nutzt aber nur
   \c Id und \c Wert. Die Felder \c Name und \c Kommentar sind als
   \c may_be_missing beschrieben und dürfen daher im SQL fehlen.

\param db Logische Datenbank
\param vecInserted Datensätze inklusive Identity
\param os Ausgabestrom für Testmeldungen
\throw std::runtime_error wenn eine Testbedingung fehlschlägt
*/
template <adecc::db::logical_database_type app_db>
void UpdateRowsByCall(
   app_db& db,
   std::vector<test_row_ty> const& vecInserted,
   std::ostringstream& os
) {
   if (vecInserted.size() < 3) {
	  throw std::runtime_error {
		 "UpdateRowsByCall: Zu wenige eingefügte Datensätze"
		 };
	  }

   std::vector<test_row_ty> vecUpdate {
	  {
		 std::get<0>(vecInserted[0]),
		 std::get<1>(vecInserted[0]),
		 std::get<2>(vecInserted[0]),
		 110
	  },
	  {
		 std::get<0>(vecInserted[2]),
		 std::get<1>(vecInserted[2]),
		 std::get<2>(vecInserted[2]),
		 330
	  }
	  };

   auto aSink = db.template MakeOutputSink<int, std::string, std::string, int>(
	  "update dbo.TestDbNeu "
	  "set Wert = :Wert "
	  "where Id = :Id",
	  {
		 { "Id",        adecc::db_output_param_role::key },
		 { "Name",      adecc::db_output_param_role::may_be_missing },
		 { "Kommentar", adecc::db_output_param_role::may_be_missing },
		 { "Wert",      adecc::db_output_param_role::needed_value }
	  }
   );

   aSink(vecUpdate);

   for (auto const& tupRow : vecUpdate) {
	  std::println(
		 os,
		 "Updated by call: Id={}, Wert={}",
		 std::get<0>(tupRow),
		 std::get<3>(tupRow)
	  );
	  }
   }


/*!
\brief Aktualisiert eine Zeile über \c OutputSink::operator=
\details
   Diese Methode testet die Zuweisung eines einzelnen Tuple-Werts an den Sink.

\param db Logische Datenbank
\param tupInserted Datensatz inklusive Identity
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void UpdateOneRowByAssignment(app_db& db, test_row_ty const& tupInserted,
                              std::ostream& out) {
   test_row_ty tupUpdate { std::get<0>(tupInserted),
	                        std::get<1>(tupInserted),
	                        std::get<2>(tupInserted),
	                        111
	                      };

   auto aSink = db.template MakeOutputSink<int, std::string, std::string, int>(
	  "update dbo.TestDbNeu "
	  "set Wert = :Wert "
	  "where Id = :Id",
	  {
		 { "Id",        adecc::db_output_param_role::key },
		 { "Name",      adecc::db_output_param_role::may_be_missing },
		 { "Kommentar", adecc::db_output_param_role::may_be_missing },
		 { "Wert",      adecc::db_output_param_role::needed_value }
	  }
   );

   aSink = tupUpdate;

   std::println(out, "Updated by tuple assignment: Id={}, Wert={}",
	             std::get<0>(tupUpdate), std::get<3>(tupUpdate));
   }


/*!
\brief Aktualisiert mehrere Zeilen über \c OutputSink::operator=
\details
   Diese Methode testet die Zuweisung eines vollständigen Ranges an den Sink.

\param db Logische Datenbank
\param vecInserted Datensätze inklusive Identity
\param os Ausgabestrom für Testmeldungen
\throw std::runtime_error wenn eine Testbedingung fehlschlägt
*/
template <adecc::db::logical_database_type app_db>
void UpdateRowsByRangeAssignment(app_db& db, std::vector<test_row_ty> const& vecInserted,
                                 std::ostream& out) {
   if (vecInserted.size() < 3) {
	  throw std::runtime_error { 
		   "UpdateRowsByRangeAssignment: The number of inserted records is too low."
		 };
	  }

   std::vector<test_row_ty> vecUpdate {	 
		{
		     std::get<0>(vecInserted[0]),
		     std::get<1>(vecInserted[0]),
		     std::get<2>(vecInserted[0]),
		     112
	     },
	     {
		     std::get<0>(vecInserted[1]),
		     std::get<1>(vecInserted[1]),
		     std::get<2>(vecInserted[1]),
		     222
	     }
	   };

   auto aSink = db.template MakeOutputSink<int, std::string, std::string, int>(
	  "update dbo.TestDbNeu "
	  "set Wert = :Wert "
	  "where Id = :Id",
	  {
		 { "Id",        adecc::db_output_param_role::key },
		 { "Name",      adecc::db_output_param_role::may_be_missing },
		 { "Kommentar", adecc::db_output_param_role::may_be_missing },
		 { "Wert",      adecc::db_output_param_role::needed_value }
	  }
   );

   aSink = vecUpdate;

   for (auto const& tupRow : vecUpdate) {
	  std::println(out, "Updated by range assignment: Id={}, Wert={}",
		            std::get<0>(tupRow), std::get<3>(tupRow));
	  }
   }


/*!
\brief Aktualisiert mehrere Zeilen über \c std::ranges::copy
\details
   Diese Methode testet den Output Iterator des Sinks.

\param db Logische Datenbank
\param vecInserted Datensätze inklusive Identity
\param os Ausgabestrom für Testmeldungen
\throw std::runtime_error wenn eine Testbedingung fehlschlägt
*/
template <adecc::db::logical_database_type app_db>
void UpdateRowsByCopy(app_db& db, std::vector<test_row_ty> const& vecInserted,
                      std::ostream& out) {
   if (vecInserted.size() < 3) {
	  throw std::runtime_error {
		 "UpdateRowsByCopy: The number of inserted records is too low."
		 };
	  }

   std::vector<test_row_ty> vecUpdate {
	    {
		   std::get<0>(vecInserted[1]),
		   std::get<1>(vecInserted[1]),
		   std::get<2>(vecInserted[1]),
		   223
	    },
	    {
		   std::get<0>(vecInserted[2]),
		   std::get<1>(vecInserted[2]),
		   std::get<2>(vecInserted[2]),
		   333
	    }
	  };

   auto aSink = db.template MakeOutputSink<int, std::string, std::string, int>(
	  "update dbo.TestDbNeu "
	  "set Wert = :Wert "
	  "where Id = :Id",
	    {
		   { "Id",        adecc::db_output_param_role::key },
		   { "Name",      adecc::db_output_param_role::may_be_missing },
		   { "Kommentar", adecc::db_output_param_role::may_be_missing },
		   { "Wert",      adecc::db_output_param_role::needed_value }
	    }
     );

   std::ranges::copy(vecUpdate, aSink.OutputIterator());

   for (auto const& tupRow : vecUpdate) {
	  std::println(out, "Updated by std::ranges::copy: Id={}, Wert={}",
		            std::get<0>(tupRow), std::get<3>(tupRow));
	  }
   }


/*!
\brief Liest die Testtabelle zur Kontrolle aus
\details
   Die Methode schreibt alle Datensätze sortiert nach \c Id in den Ausgabestrom.

\param db Logische Datenbank
\param os Ausgabestrom für Testmeldungen
*/
template <adecc::db::logical_database_type app_db>
void PrintRows(app_db& db, std::ostream& out) {
   auto rngRows = db.template Execute<int, std::string, std::optional<std::string>, int>(
	  "select Id, Name, Kommentar, Wert "
	  "from dbo.TestDbNeu "
	  "order by Id"
   );

   std::println(out, "");
   std::println(out, "Current rows:");

   for (auto const& [iId, strName, optKommentar, iWert] : rngRows) {
	  std::println(out, "Id={}, Name={}, Kommentar={}, Wert={}",
		            iId, strName, optKommentar.value_or("<NULL>"), iWert);
	  }
   }


/*!
\brief Prüft das finale Ergebnis der Insert- und Update-Operationen
\details
   Erwartet werden drei Datensätze. Nach allen Update-Varianten müssen die
   finalen Werte 112, 223 und 333 lauten.

\param db Logische Datenbank
\param os Ausgabestrom für Testmeldungen
\throw std::runtime_error wenn eine Testbedingung fehlschlägt
*/
template <adecc::db::logical_database_type app_db>
void VerifyRows(app_db& db, std::ostream& out) {
   auto rngRows = db.template Execute<int, std::string, std::optional<std::string>, int>(
	  "select Id, Name, Kommentar, Wert "
	  "from dbo.TestDbNeu "
	  "order by Id"
	  );

   std::vector<std::tuple<int, std::string, std::optional<std::string>, int>> vecRows;

   for (auto const& tupRow : rngRows) {
	  vecRows.emplace_back(tupRow);
	  }

   if (vecRows.size() != 3) {
	  throw std::runtime_error {
		 "VerifyRows: Exactly three records were not found"
		 };
	  }

   if (std::get<3>(vecRows[0]) != 112) {
	  throw std::runtime_error {
		 "VerifyRows: The first line does not have the expected value of 112"
		 };
	  }

   if (std::get<3>(vecRows[1]) != 223) {
	  throw std::runtime_error {
		 "VerifyRows: The second row does not contain the expected value of 223"
		 };
	  }

   if (std::get<3>(vecRows[2]) != 333) {
	  throw std::runtime_error {
		 "VerifyRows: The third row does not have the expected value of 333"
		 };
	  }

   std::println(out, "VerifyRows: Passed all exams.");
   }

void TestErrorMessageWithParams(auto& db, std::ostream& out) {
	using database_ty = std::remove_cvref_t<decltype(db)>;

	auto const strSql = "SELECT CAST(:Wert AS int) AS Wert "
		"FROM TestDbNeu "
		"WHERE Id = :keyId";

	adecc::db_params vecParams{
		database_ty::Param("Wert", std::string { "kein-int" }),
		database_ty::Param("keyId", 1),
		database_ty::Param("AddValue", std::optional<int> {}, false)
	};

	try {
		for (auto const& aRow : db.template Execute<int>(strSql, vecParams)) {
			(void)aRow;
		}

		std::println(std::cout, "FEHLER: Query hätte fehlschlagen müssen.");
	}
	catch (std::exception const& ex) {
		std::println(out, "Erwarteter Fehler wurde ausgelöst:");
		std::println(out, "{}", ex.what());
	}
}


/*!
\brief Führt den vollständigen Test für ExecuteCommand, OutputRange und OutputSink aus
\details
   Der Test erzeugt eine echte Tabelle \c dbo.TestDbNeu, fügt mehrere Zeilen
   mit Identity-Rückschreibung ein und aktualisiert danach ausgewählte Zeilen
   über mehrere Sink-Nutzungsformen.

\param db Logische Datenbank
\returns Textprotokoll des Tests
*/
template <adecc::db::logical_database_type app_db>
std::string RunOutputTests(app_db& db) {
   std::ostringstream os;

   auto aTransaction = db.Transaction();

   CreateTestTable(db, os);

   std::vector<test_row_ty> vecInserted = InsertRows(db, os);

   UpdateRowsByCall(db, vecInserted, os);
   UpdateOneRowByAssignment(db, vecInserted[0], os);
   UpdateRowsByRangeAssignment(db, vecInserted, os);
   UpdateRowsByCopy(db, vecInserted, os);

   VerifyRows(db, os);

   PrintRows(db, os);

	//TestErrorMessageWithParams(db, os);

   aTransaction.Commit();

   std::println(os, "");
   std::println(os, "Output Tests erfolgreich abgeschlossen.");

   return os.str();
   }
