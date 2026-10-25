#pragma once

#include "database.h"
#include <iostream>
#include <string>
#include <print>

namespace extented_test {

   // -----------------------------------------------------------------------------
   //    Methode zum Erzeugen der Tabellen für den Test, löschen wenn sie extieren
   // -----------------------------------------------------------------------------
   template <adecc::db::logical_database_type app_db>

   void CreateTables(app_db& db, std::ostream& os) {
      db.ExecuteCommand(
         "DROP TABLE IF EXISTS dbo.Test_Posting;\n"
         "DROP TABLE IF EXISTS dbo.Test_TaxClass;\n"
         "DROP TABLE IF EXISTS dbo.Test_NominalAcct;\n"
         "DROP TABLE IF EXISTS dbo.Test_Account;\n"
         "DROP TABLE IF EXISTS dbo.Test_AccountClass;\n"
         "DROP TABLE IF EXISTS dbo.Test_AccountType;\n"
         "DROP TABLE IF EXISTS dbo.Test_AddressType;\n"
         );

      db.ExecuteCommand(
         "CREATE TABLE Test_AddressType(\n"
         "   ID            INTEGER NOT NULL,\n"
         "   Denotation    VARCHAR(50) NOT NULL,"
         "   Abbreviation  VARCHAR(10) NOT NULL,"
         "   Description   VARCHAR(MAX) NULL,"

         "   CONSTRAINT pk_Test_AddressType PRIMARY KEY CLUSTERED (ID),\n"
         "   CONSTRAINT uk_Test_AddressType_Denotation UNIQUE (Denotation),\n"
         "   CONSTRAINT uk_Test_AddressType_Abbreviation UNIQUE (Abbreviation),\n"
         ")"
         );

      std::println(os, "Tabelle \"Test_AddressType\" wurde neu erzeugt.");

      db.ExecuteCommand(
         "CREATE TABLE Test_AccountType(\n"
         "   ID                   INTEGER IDENTITY(1, 1) NOT NULL,\n"
         "   Denotation           VARCHAR(100) NOT NULL,\n"
         "   Abbreviation         VARCHAR(10)  NOT NULL,\n"
         "   Description          VARCHAR(MAX) NULL,\n"

         "   CONSTRAINT pk_Test_AccountType PRIMARY KEY CLUSTERED (ID),\n"
         "   CONSTRAINT uk_Test_AccountType_Denotation UNIQUE (Denotation)\n"
         ")"
         );

      std::println(os, "Tabelle \"Test_AccountType\" wurde neu erzeugt.");

      db.ExecuteCommand(
         "CREATE TABLE dbo.Test_AccountClass(\n"
         "   ID                     INTEGER IDENTITY(1, 1) NOT NULL,\n"
         "   Denotation             VARCHAR(100) NOT NULL,\n"
         "   Description            VARCHAR(MAX) NULL,\n"
         "   Type_Code              VARCHAR(30)  NOT NULL,\n"
         "   Balance_Side           VARCHAR(10)  NOT NULL,\n"
         "   Pnl_Relevant           SMALLINT     NOT NULL,\n"
         "   Balance_Sheet_Relevant SMALLINT     NOT NULL,\n"
         "   Natural_Debit          SMALLINT     NOT NULL,\n"
         "   Natural_Credit         SMALLINT     NOT NULL,\n"
         "   Sort_Order             INTEGER      NOT NULL,\n"

         "   CONSTRAINT pk_Test_AccountClass PRIMARY KEY CLUSTERED (ID),\n"
         "   CONSTRAINT uk_Test_AccountClass_Denotation UNIQUE (Denotation),\n"
         "   CONSTRAINT ck_Test_AccountClass_BalanceSide CHECK(Balance_Side IN ('Debit', 'Credit', 'None')),\n"
         "   CONSTRAINT ck_Test_AccountClass_NaturalSide CHECK ((Natural_Debit = 1 AND Natural_Credit = 0) OR\n"
         "                                                      (Natural_Debit = 0 AND Natural_Credit = 1) OR\n"
         "                                                      (Natural_Debit = 0 AND Natural_Credit = 0))\n"
         ")"
         );

      std::println(os, "Tabelle \"Test_AccountClass\" wurde neu erzeugt.");

      /*
      db.ExecuteCommand(
         "CREATE TABLE dbo.Test_Account ( "
         "   ID            INTEGER IDENTITY(1, 1) NOT NULL, "
         "   Account_No    INTEGER NOT NULL, "
         "   Denotation    VARCHAR(100) NOT NULL, "
         "   Account_Type  INTEGER NOT NULL, "
         "   Account_Class INTEGER NOT NULL, "
         "   Currency      CHAR(3) NOT NULL CONSTRAINT df_Test_Account_Currency DEFAULT 'EUR', "
         "   IsActive      SMALLINT NOT NULL CONSTRAINT df_Test_Account_IsActive DEFAULT 1, "
         "   Description   VARCHAR(MAX) NULL, "

         "   CONSTRAINT pk_Test_Account PRIMARY KEY CLUSTERED (ID), "
         "   CONSTRAINT uk_Test_Account_No UNIQUE (Account_No), "
         "   CONSTRAINT uk_Test_Account_Description UNIQUE (Denotation), "
         "   CONSTRAINT fk_Test_Account_AcctType FOREIGN KEY (Account_Type) REFERENCES Test_AccountType(ID), "
         "   CONSTRAINT fk_Test_Account_AcctClass FOREIGN KEY (Account_Class) REFERENCES Test_AccountClass(ID)"
         ")"
         );

      std::println(os, "Tabelle \"Test_Account\" wurde neu erzeugt.");
      */

      db.ExecuteCommand(
         "CREATE TABLE dbo.Test_Account (\n"
         "   ID            INTEGER IDENTITY(1, 1) NOT NULL,\n"
         "   Account_No    INTEGER NOT NULL,\n"
         "   Denotation    VARCHAR(100) NOT NULL,\n"
         "   Account_Type  INTEGER NOT NULL,\n"

         "   CONSTRAINT pk_Test_Account PRIMARY KEY CLUSTERED (ID),\n"
         "   CONSTRAINT uk_Test_Account_No UNIQUE (Account_No),\n"
         "   CONSTRAINT uk_Test_Account_Description UNIQUE (Denotation),\n"
         "   CONSTRAINT fk_Test_Account_AcctType FOREIGN KEY (Account_Type) REFERENCES Test_AccountType(ID)\n"
         ")"
      );

      std::println(os, "Tabelle \"Test_Account\" wurde neu erzeugt.");

      db.ExecuteCommand(
         "CREATE TABLE dbo.Test_NominalAcct  ( "
         "   ID            INTEGER NOT NULL,\n"
         "   Account_Class INTEGER NOT NULL,\n"
         "   Currency      CHAR(3) NOT NULL CONSTRAINT df_Test_Account_Currency DEFAULT 'EUR',\n"
         "   IsActive      SMALLINT NOT NULL CONSTRAINT df_Test_Account_IsActive DEFAULT 1,\n"
         "   Description   VARCHAR(MAX) NULL,\n"

         "   CONSTRAINT pk_Test_NominalAcct  PRIMARY KEY CLUSTERED (ID),\n"
         "   CONSTRAINT fk_Test_NominalAcct_Account FOREIGN KEY (ID) REFERENCES Test_Account(ID),\n"
         "   CONSTRAINT fk_Test_NominalAcct_AcctClass FOREIGN KEY (Account_Class) REFERENCES Test_AccountClass(ID)\n"
         ")"
      );

      std::println(os, "Tabelle \"Test_NominalAcct \" wurde neu erzeugt.");



      db.ExecuteCommand(
         "CREATE TABLE Test_TaxClass ( "
         "   ID               INTEGER IDENTITY(1, 1) NOT NULL, "
         "   Tax_Class        VARCHAR(15) NOT NULL, "
         "   Description      VARCHAR(50) NOT NULL, "
         "   Tax_Percent      DECIMAL(5,2) NOT NULL, "
         "   Tax_Acct_Debit   INTEGER NULL, "
         "   Tax_Acct_Credit  INTEGER NULL, "
         "   Eligible_for_Tax SMALLINT NOT NULL, "
         "   Subject_to_Tax   SMALLINT NOT NULL, "

         "   CONSTRAINT pk_Test_TaxClass PRIMARY KEY CLUSTERED (ID), "
         "   CONSTRAINT uk_Test_TaxClass_Class UNIQUE (Tax_Class), "
         "   CONSTRAINT uk_Test_TaxClass_Description UNIQUE (Description), "
         "   CONSTRAINT fk_Test_TaxClass_Tax_Acct_Debit FOREIGN KEY (Tax_Acct_Debit) REFERENCES Test_Account(ID), "
         "   CONSTRAINT fk_Test_TaxClass_Tax_Acct_Credit FOREIGN KEY (Tax_Acct_Credit) REFERENCES Test_Account(ID) "
         ")"
        );

      std::println(os, "Tabelle \"Test_TaxClass\" wurde neu erzeugt.");

      db.ExecuteCommand(
         "CREATE TABLE Test_Posting ( "
         "  ID              INTEGER IDENTITY(1, 1) NOT NULL, "
         "  Booking_Date    DATE NOT NULL, "
         "  Booking_No      VARCHAR(30) NOT NULL, "
         "  Debit_Acct_ID   INTEGER NOT NULL, "
         "  Credit_Acct_ID  INTEGER NOT NULL, "
         "  Amount          DECIMAL(15, 2) NOT NULL, "
         "  Tax_Class       INTEGER NOT NULL, "
         "  Posting_Text    VARCHAR(255) NOT NULL, "

         "  CONSTRAINT pk_Test_Booking PRIMARY KEY CLUSTERED (ID), "
         "  CONSTRAINT fk_Test_Booking_Debit_Acct FOREIGN KEY (Debit_Acct_ID) REFERENCES Test_Account(ID), "
         "  CONSTRAINT fk_Test_Booking_Credit_Acct FOREIGN KEY (Credit_Acct_ID) REFERENCES Test_Account(ID), "
         "  CONSTRAINT fk_Test_Booking_Tax_Class FOREIGN KEY (Tax_Class) REFERENCES Test_TaxClass(ID), "
         "  CONSTRAINT ck_Test_Booking_Different_Acct CHECK(Debit_Acct_ID <> Credit_Acct_ID) "
         ")"
         );
      std::println(os, "Tabelle \"Test_Posting\" wurde neu erzeugt.");
      std::println(os);
      }

} // end of namespace extented_test