// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
/** \file test_mariadb_connection.cpp
\brief Native C++23 MariaDB backend contract and optional live server test.
*/
#include <adecc/mariadb/mariadb_database.h>
#include <mysql.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc,char** argv) {
   using adecc::db::mariadb::mariadb_database;
   using adecc::db::mariadb::mariadb_credentials;
   if(argc==2 && std::string_view(argv[1])=="--offline") {
      mariadb_database db;
      bool const ok=!db.Connected() && mysql_get_client_version()>=30400UL;
      std::cout << "SMOKE|RESULT|" << (ok?"PASS":"FAIL") << "|MariaDB C API and database concept\n";
      return ok?0:1;
   }
   auto GetEnv=[](char const* name,char const* fallback)->std::string {
      char const* p=std::getenv(name);
      return p&&*p?p:fallback;
   };
   mariadb_credentials credentials{
      .strHost=GetEnv("BUILDENGINE_MARIADB_HOST","localhost"),
      .uPort=static_cast<std::uint16_t>(std::stoi(GetEnv("BUILDENGINE_MARIADB_PORT","3306"))),
      .strDatabase=GetEnv("BUILDENGINE_MARIADB_DATABASE","BuildEngineTest"),
      .strUser=GetEnv("BUILDENGINE_MARIADB_USER","buildenginetest_user"),
      .strPassword=GetEnv("BUILDENGINE_MARIADB_PASSWORD",""),
      .strCaFile=GetEnv("BUILDENGINE_MARIADB_CA",""),
      .boRequireTls=true
   };
   mariadb_database db{credentials};
   auto connected=db.Connect();
   if(!connected || !*connected) {
      std::cerr << "MariaDB connection failed: "
         << (connected?"not connected":std::get<2>(connected.error())) << '\n';
      return 1;
   }
   auto transaction=db.BeginTransaction();
   if(!transaction||!*transaction) return 2;
   if(auto rollback=db.Rollback();!rollback||!*rollback)return 3;
   std::cout<<"SMOKE|RESULT|PASS|MariaDB connection and rollback\n";
   return 0;
}
