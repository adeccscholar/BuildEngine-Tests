// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/** \file mariadb_arguments.h \brief Command-line configuration for MariaDB integration tests. */
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mariadb_test {
struct config {
   std::string host{"localhost"};
   std::uint16_t port{3306};
   std::string database{"BuildEngineTest"};
   std::string user{"root"};
   std::string password;
   std::string ca;
   bool offline{};
   bool requireTls{true};
   bool integrated{};
};
[[nodiscard]] inline std::string Env(char const* name,std::string fallback={}) {
   char const* value=std::getenv(name);
   return value&&*value?std::string{value}:std::move(fallback);
}
[[nodiscard]] inline std::uint16_t Port(std::string_view value) {
   unsigned int number{};
   auto [pos,err]=std::from_chars(value.data(),value.data()+value.size(),number);
   if(err!=std::errc{}||pos!=value.data()+value.size()||number==0||number>65535)
      throw std::invalid_argument{"Invalid MariaDB port"};
   return static_cast<std::uint16_t>(number);
}
/** \brief Parse command-line overrides, honoring defaults and environment variables.
\details An explicit --password is accepted as requested, but environment
variables are recommended to avoid exposing passwords in process listings.
*/
[[nodiscard]] inline config Parse(int argc,char** argv) {
   config c;
   c.host=Env("BUILDENGINE_MARIADB_HOST","localhost");
   c.port=Port(Env("BUILDENGINE_MARIADB_PORT","3306"));
   c.database=Env("BUILDENGINE_MARIADB_DATABASE","BuildEngineTest");
   c.user=Env("BUILDENGINE_MARIADB_USER","root");
   c.password=Env("BUILDENGINE_MARIADB_PASSWORD");
   c.ca=Env("BUILDENGINE_MARIADB_CA");
   for(int i=1;i<argc;++i) {
      std::string_view option=argv[i];
      auto value=[&]()->std::string {
         if(i+1>=argc)throw std::invalid_argument{"Missing value for "+std::string{option}};
         return argv[++i];
      };
      if(option=="--host")c.host=value();
      else if(option=="--port")c.port=Port(value());
      else if(option=="--database")c.database=value();
      else if(option=="--user")c.user=value();
      else if(option=="--password")c.password=value();
      else if(option=="--ca")c.ca=value();
      else if(option=="--offline")c.offline=true;
      else if(option=="--integrated")c.integrated=true;
      else if(option=="--no-tls")c.requireTls=false;
      else if(option=="--help"||option=="-h")
         throw std::invalid_argument{"Usage: mariadb-test [--host HOST] [--port 3306] [--database DB] [--user root] [--password PASSWORD] [--ca CERT] [--integrated] [--offline] [--no-tls]"};
      else throw std::invalid_argument{"Unknown option: "+std::string{option}};
   }
   if(c.host.empty()||c.database.empty()||c.user.empty())
      throw std::invalid_argument{"Host, database and user must not be empty"};
   if(c.integrated&&!c.password.empty())
      throw std::invalid_argument{"Integrated authentication must not specify a password"};
   return c;
}
}
