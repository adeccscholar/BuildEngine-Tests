#pragma once
/** \file additional_dialect.h
\brief MariaDB variant for the existing shared logical database test.
*/
struct Dialect {
   template<class DB> void Create(DB& db) const {
      db.ExecuteCommand("DROP TABLE IF EXISTS Test_Error_TupleInsert");
      db.ExecuteCommand("CREATE TABLE Test_Error_TupleInsert ("
         "Id int NOT NULL PRIMARY KEY,"
         "Name varchar(50) NOT NULL,"
         "Amount int NOT NULL CHECK (Amount>=0),"
         "OptionalValue int NULL) ENGINE=InnoDB");
   }
   template<class DB> void Drop(DB& db) const {
      db.ExecuteCommand("DROP TABLE IF EXISTS Test_Error_TupleInsert");
   }
   template<class DB> void ResetAfterExpectedError(DB&) const {}
};
