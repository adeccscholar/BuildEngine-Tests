-- Phase 1: execute as PostgreSQL administrator connected to postgres.
-- psql -h localhost -U postgres -d postgres -f create-database.sql
SELECT 'CREATE DATABASE "BuildEngineTest"'
WHERE NOT EXISTS (SELECT 1 FROM pg_database WHERE datname='BuildEngineTest')
\gexec
-- Phase 2: execute against BuildEngineTest:
-- psql -h localhost -U postgres -d BuildEngineTest -f configure-access.sql
