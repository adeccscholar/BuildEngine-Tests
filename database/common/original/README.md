# Original Test.zip — reference source archive

The supplied C++ header collection was imported without intentional changes.
It remains readable through this documentation server for source comparison,
but **the archived headers are not all compiled into the current test executables**.

## Original tests and support headers

| File | Topic |
| --- | --- |
| [db_test.h](db_test.h) | OutputRange, identity, OutputSink and update verification |
| [db_test_detailed.h](db_test_detailed.h) | Original detailed database/error scenarios |
| [db_test_detailed(1).h](db_test_detailed%281%29.h) | Alternate detailed scenario/test-run interface |
| [test_framework.h](test_framework.h) | Test results, diagnostics and assertions |
| [test_framework(1).h](test_framework%281%29.h) | Alternate test framework |
| [persistence_test.h](persistence_test.h) | Persistence exercises |
| [persistent_insert_complete.h](persistent_insert_complete.h) | Complete persistent insertion helpers |
| [extented_test.h](extented_test.h) | Extended persistence and grid scenario |
| [extented_test_create.h](extented_test_create.h) | Original backend-specific DDL |
| [extented_test_types.h](extented_test_types.h) | Persistent model types |
| [extented_test_data.h](extented_test_data.h) | Original test input data |
| [extented_test_details.h](extented_test_details.h) | Detail and output-range helpers |
| [extented_test_details_bwa.h](extented_test_details_bwa.h) | BWA detail calculations |
| [extented_test_helper.h](extented_test_helper.h) | Test helper operations |
| [extented_test_grids.h](extented_test_grids.h) | Grid output |

See the active, backend-neutral [additional test implementation](../additional_tests.h)
and the [database overview](../../README.md).

The server renders these source files as **read-only HTML with syntax highlighting**;
it does not expose compilation, database execution or arbitrary files.
