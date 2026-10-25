-- Administrator session inside BuildEngineTest only.
DO $block$
BEGIN
   IF current_database() <> 'BuildEngineTest' THEN
      RAISE EXCEPTION 'Expected BuildEngineTest, found %', current_database();
   END IF;
   IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'buildenginetest_user') THEN
      CREATE ROLE buildenginetest_user LOGIN;
   END IF;
END
$block$;
GRANT CONNECT ON DATABASE "BuildEngineTest" TO buildenginetest_user;
GRANT TEMPORARY ON DATABASE "BuildEngineTest" TO buildenginetest_user;
-- This backend test uses CREATE TEMP TABLE and does not require public-schema DDL.
-- For Windows integrated login, configure pg_hba.conf/pg_ident.conf mapping
-- to buildenginetest_user and enable SSPI/GSSAPI independently.
-- For password authentication, set the role password outside source control.
