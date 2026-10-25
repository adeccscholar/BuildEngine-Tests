-- Microsoft SQL Server (run as a sufficiently privileged administrator in sqlcmd/SSMS).
-- Database is created only when absent. Never drops an existing database.
USE [master];
GO
IF DB_ID(N'BuildEngineTest') IS NULL
BEGIN
   CREATE DATABASE [BuildEngineTest];
END;
GO
USE [BuildEngineTest];
GO
-- Integrated authentication: grant the actual developer/test account
-- CONNECT and DDL permissions in this isolated TEST database.
-- Replace DOMAIN\Account only after reviewing the intended Windows account:
-- CREATE USER [DOMAIN\Account] FOR LOGIN [DOMAIN\Account];
-- ALTER ROLE db_ddladmin ADD MEMBER [DOMAIN\Account];
-- ALTER ROLE db_datareader ADD MEMBER [DOMAIN\Account];
-- ALTER ROLE db_datawriter ADD MEMBER [DOMAIN\Account];
SELECT DB_NAME() AS ActiveTestDatabase;
GO
