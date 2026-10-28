#pragma once
/** MariaDB 11 concrete DDL for shared membership fixture. */
struct MemberDialect {
 template<class DB> void RecreateTables(DB& db)const {
  db.ExecuteCommand("DROP TABLE IF EXISTS Buchungen");
  db.ExecuteCommand("DROP TABLE IF EXISTS Mitglied");
  db.ExecuteCommand("DROP TABLE IF EXISTS Anreden");
  db.ExecuteCommand("CREATE TABLE Anreden (ID int AUTO_INCREMENT PRIMARY KEY, Bezeichnung varchar(30) NOT NULL UNIQUE, Kurztext varchar(10) NOT NULL, Anrede varchar(50) NOT NULL) ENGINE=InnoDB");
  db.ExecuteCommand("CREATE TABLE Mitglied (ID int AUTO_INCREMENT PRIMARY KEY, MitgliedNr varchar(12) NOT NULL UNIQUE, Name varchar(50) NOT NULL, Vorname varchar(50) NOT NULL, Anrede int NOT NULL, Geburtstag date NOT NULL, Strasse varchar(50) NOT NULL, Hausnummer varchar(10) NOT NULL, PLZ varchar(5) NOT NULL, Ort varchar(50) NOT NULL, Beginn date NOT NULL, Beitrag decimal(10,2) NOT NULL, IBAN varchar(34) NOT NULL, Steuer varchar(20) NOT NULL, FOREIGN KEY (Anrede) REFERENCES Anreden(ID)) ENGINE=InnoDB");
  db.ExecuteCommand("CREATE TABLE Buchungen (ID int AUTO_INCREMENT PRIMARY KEY, MitgliedID int NOT NULL, PerDat date NOT NULL, Betrag decimal(10,2) NOT NULL, IBAN varchar(34) NOT NULL, FOREIGN KEY (MitgliedID) REFERENCES Mitglied(ID)) ENGINE=InnoDB");
 }
};
