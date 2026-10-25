#pragma once
struct Dialect {
 template<class DB> void Create(DB& db) const {
   db.ExecuteCommand("DROP TABLE IF EXISTS Test_Error_TupleInsert");
   db.ExecuteCommand("CREATE TEMP TABLE Test_Error_TupleInsert ("
      "Id integer NOT NULL PRIMARY KEY,"
      "Name varchar(50) NOT NULL,"
      "Amount integer NOT NULL CHECK (Amount>=0),"
      "OptionalValue integer NULL) ON COMMIT PRESERVE ROWS");
 }
 template<class DB> void Drop(DB& db) const {
   db.ExecuteCommand("DROP TABLE IF EXISTS Test_Error_TupleInsert");
 }
 template<class DB> void ResetAfterExpectedError(DB&) const {}
};
