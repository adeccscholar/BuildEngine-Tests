// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
/** \file member_pipeline_mariadb.cpp
\brief MariaDB adapter for the reusable member contribution workload.
*/
#include "mariadb_arguments.h"
#include "member_pipeline.h"
#include "member_dialect.h"
#include <adecc/mariadb/mariadb_query.h>
#include <adecc/cpp_core/database.h>
#include <mysql.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using database_ty=adecc::db::logical_database<
   adecc::db::mariadb::mariadb_database,adecc::db::mariadb::query>;

int main(int argc,char** argv) {
   try {
      auto args=mariadb_test::Parse(argc,argv);
      if(args.offline) {
         auto plan=adecc::db::mariadb::detail::Plan(
            "SELECT :Member, ':Literal', :Member");
         if(plan.names.size()!=2)
            throw std::runtime_error{"Parameter scanner failed"};
         std::cout<<"SMOKE|RESULT|PASS|Member pipeline offline scanner\n";
         return 0;
      }
      if(args.integrated)
         throw std::runtime_error{"GSSAPI login not verified: no password fallback"};
      if(args.database!="BuildEngineTest"&&
         args.database.rfind("BuildEngineTest_",0)!=0)
         throw std::runtime_error{"Destructive fixture only permits BuildEngineTest or BuildEngineTest_*"};
      for(char c:args.database)
         if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
              (c>='0'&&c<='9')||c=='_'))
            throw std::runtime_error{"Unsafe database identifier"};
      auto credentials=adecc::db::mariadb::mariadb_credentials{
         .strHost=args.host,.uPort=args.port,.strDatabase=args.database,
         .strUser=args.user,.strPassword=args.password,.strCaFile=args.ca,
         .boRequireTls=args.requireTls};
      // Bootstrap with an empty default schema; CREATE DATABASE has implicit commit.
      auto bootstrap=credentials;
      bootstrap.strDatabase.clear();
      adecc::db::mariadb::mariadb_database initial{bootstrap};
      auto ok=initial.Connect();
      if(!ok||!*ok)
         throw std::runtime_error{"Bootstrap connection failed: "+
            (ok?std::string{"disconnected"}:std::get<2>(ok.error()))};
      MYSQL* native=initial.NativeHandle();
      std::string sql="CREATE DATABASE IF NOT EXISTS "+args.database+
         " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
      if(mysql_real_query(native,sql.data(),static_cast<unsigned long>(sql.size()))!=0)
         throw std::runtime_error{"Cannot create test database: "+std::string{mysql_error(native)}};
      initial=adecc::db::mariadb::mariadb_database{};
      database_ty db{credentials};
      ok=db.Connect();
      if(!ok||!*ok)
         throw std::runtime_error{"Test database connection failed: "+
            (ok?std::string{"disconnected"}:std::get<2>(ok.error()))};
      std::filesystem::path fixtures=
         std::filesystem::path{__FILE__}.parent_path()/"fixtures";
      // Source checkout is expected to remain at production tests/database/mariadb.
      if(!std::filesystem::is_directory(fixtures))
         throw std::runtime_error{"Missing fixtures directory: "+fixtures.string()};
      buildengine_tests::database::members::Run(db,MemberDialect{},fixtures);
      std::cout<<"SMOKE|RESULT|PASS|MariaDB members and contributions\n";
      return 0;
   }catch(std::exception const& e){
      std::cerr<<"SMOKE|RESULT|FAIL|"<<e.what()<<"\n";return 1;
   }
}
