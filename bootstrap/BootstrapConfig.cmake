# BuildEngine-Tests bootstrap configuration.
#
# Keep the repro toolchain deterministic and independent of a globally
# installed Ninja.

if(NOT DEFINED ADECC_NINJA_VERSION)
   set(ADECC_NINJA_VERSION "1.13.2")
endif()

if(NOT DEFINED ADECC_NINJA_URL)
   set(ADECC_NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip")
endif()

if(NOT DEFINED ADECC_NINJA_ARCHIVE_SIZE)
   set(ADECC_NINJA_ARCHIVE_SIZE "291570")
endif()

if(NOT DEFINED ADECC_NINJA_SHA256)
   set(ADECC_NINJA_SHA256 "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65")
endif()
