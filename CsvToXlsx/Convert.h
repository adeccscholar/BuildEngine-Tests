#pragma once

#include <cstdint>
#include <filesystem>

namespace adecc::csv_to_xlsx {

/**
\brief Successful conversion metrics.
\details The maximum column count covers the widest logical record.
*/
struct ConversionResult {
   std::uint32_t uRows{};
   std::uint16_t uMaxColumns{};
   char chDelimiter{};
};

/**
\brief Convert one CSV file into one Excel workbook.
\details Writes every value as std::string, never overwrites an existing XLSX,
and commits the XLSX through a temporary file in the destination directory.
\param aInput Full path to the UTF-8 CSV source file.
\param aOutput Full path to the desired .xlsx workbook.
\returns Counts and the automatically detected CSV delimiter.
\throw std::exception When parsing, validation, writing or publishing fails.
\pre Both paths are distinct; the output parent directory exists.
\post A successful call leaves an XLSX containing all input records.
*/
ConversionResult ConvertFile(std::filesystem::path const& aInput,
                             std::filesystem::path const& aOutput);

} // namespace adecc::csv_to_xlsx
