// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
/** \file test_mariadb_database.cpp
\brief Execute the existing cross-database logical tests with the MariaDB backend.
*/
#include "mariadb_arguments.h"
#include "additional_dialect.h"
#include "additional_tests.h"
#include <adecc/mariadb/mariadb_query.h>
#include <adecc/cpp_core/database.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

using database_ty=adecc::db::logical_database<
   adecc::db::mariadb::mariadb_database,adecc::db::mariadb::query>;

int main(int argc,char** argv) {
   try {
      auto args=mariadb_test::Parse(argc,argv);
      if(args.offline) {
         auto p=adecc::db::mariadb::detail::Plan(
            "SELECT ':ignored', :Id, :Id, "+std::string(1,static_cast<char>(96))+"x:y"+std::string(1,static_cast<char>(96))+" -- :ignored\n");
         if(p.names.size()!=2||p.sql.find("?, ?")==std::string::npos)
            throw std::runtime_error{"Named parameter translation failed"};
         std::cout<<"SMOKE|RESULT|PASS|MariaDB offline SQL parameter test\n";
         return 0;
      }
      if(args.integrated)
         throw std::runtime_error{"GSSAPI mode is not yet verified; do not silently fall back to password authentication"};
      adecc::db::mariadb::mariadb_credentials credentials{
         .strHost=args.host,.uPort=args.port,.strDatabase=args.database,
         .strUser=args.user,.strPassword=args.password,.strCaFile=args.ca,
         .boRequireTls=args.requireTls
      };
      database_ty db{credentials};
      auto connected=db.Connect();
      if(!connected||!*connected)
         throw std::runtime_error{connected?"MariaDB not connected":std::get<2>(connected.error())};
      std::cout<<"[TEST] MariaDB "<<args.host<<":"<<args.port
               <<" database="<<args.database<<" user="<<args.user<<"\n";
      int const count=buildengine_tests::database::additional::Run(db,Dialect{});
      std::cout<<"SMOKE|RESULT|PASS|logical MariaDB shared cases: "<<count<<"\n";
      return 0;
   }
   catch(std::exception const& e){
      std::cerr<<"SMOKE|RESULT|FAIL|"<<e.what()<<"\n";
      return 1;
   }
}
