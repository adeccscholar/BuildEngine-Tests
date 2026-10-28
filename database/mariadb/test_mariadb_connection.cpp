// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
/** \file test_mariadb_connection.cpp
\brief Native C++23 MariaDB connection and transaction smoke with CLI overrides.
*/
#include "mariadb_arguments.h"
#include <adecc/mariadb/mariadb_database.h>
#include <mysql.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

int main(int argc,char** argv) {
   try {
      mariadb_test::config args=mariadb_test::Parse(argc,argv);
      if(args.offline) {
         adecc::db::mariadb::mariadb_database db;
         bool ok=!db.Connected()&&mysql_get_client_version()>=30400UL;
         std::cout<<"SMOKE|RESULT|"<<(ok?"PASS":"FAIL")<<"|MariaDB native client and concept\n";
         return ok?0:1;
      }
      if(args.integrated)
         throw std::runtime_error{"GSSAPI mode not verified; refusing password fallback"};
      adecc::db::mariadb::mariadb_credentials credentials{
         .strHost=args.host,
         .uPort=args.port,
         .strDatabase=args.database,
         .strUser=args.user,
         .strPassword=args.password,
         .strCaFile=args.ca,
         .boRequireTls=args.requireTls
      };
      adecc::db::mariadb::mariadb_database db{credentials};
      auto connected=db.Connect();
      if(!connected||!*connected)
         throw std::runtime_error{connected?"Not connected":std::get<2>(connected.error())};
      if(auto r=db.BeginTransaction();!r||!*r)
         throw std::runtime_error{"MariaDB BeginTransaction failed"};
      if(auto r=db.Rollback();!r||!*r)
         throw std::runtime_error{"MariaDB Rollback failed"};
      std::cout<<"SMOKE|RESULT|PASS|MariaDB connection and rollback\n";
      return 0;
   }
   catch(std::exception const& ex) {
      std::cerr<<"SMOKE|RESULT|FAIL|"<<ex.what()<<"\n";
      return 1;
   }
}
