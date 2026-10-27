#include "Convert.h"
#include "CsvReader.h"

#include <OpenXLSX.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace adecc::csv_to_xlsx {
namespace fs = std::filesystem;

namespace {

/**
\brief Clean up a temporary XLSX path on every exit path.
\details The final workbook is committed by rename, leaving the input intact
when conversion fails.
*/
class TemporaryFileGuard final {
public:
   /**
   \brief Construct cleanup ownership for one temporary file.
   \details Deletes only the exact path passed to the constructor.
   \param aPath Temporary workbook path.
   \returns A cleanup guard.
   \throw No explicit exception; path construction may throw.
   */
   explicit TemporaryFileGuard(fs::path aPath) : m_aPath(std::move(aPath)) {
      }
   /**
   \brief Remove the temporary file if it still exists.
   \details Uses the nonthrowing filesystem overload.
   \returns Nothing.
   \throw None.
   */
   ~TemporaryFileGuard() noexcept {
      std::error_code aError;
      fs::remove(m_aPath, aError);
      }
private:
   fs::path m_aPath;
};

/**
\brief Choose a currently unused temporary XLSX filename.
\details Keeps the temporary file in the same directory as the final file.
\param aOutput Intended final workbook path.
\returns An available temporary path.
\throw std::runtime_error When all candidate names are occupied.
*/
fs::path TemporaryPath(fs::path const& aOutput) {
   for(unsigned uSuffix = 1; uSuffix <= 10000; ++uSuffix) {
      fs::path aCandidate = aOutput;
      aCandidate += std::format(".pending-{}.xlsx", uSuffix);
      if(!fs::exists(aCandidate)) {
         return aCandidate;
         }
      }
   throw std::runtime_error("No available temporary XLSX filename");
   }

} // namespace

ConversionResult ConvertFile(fs::path const& aInput, fs::path const& aOutput) {
   if(fs::exists(aOutput)) {
      throw std::runtime_error("XLSX output already exists; refusing to overwrite");
      }
   std::ifstream aStream(aInput, std::ios::in | std::ios::binary);
   if(!aStream) {
      throw std::runtime_error("Cannot open CSV input");
      }
   SkipUtf8Bom(aStream);
   char const chDelimiter = DetectDelimiter(aStream);
   CsvReader aReader(aStream, chDelimiter);

   fs::path const aTemporary = TemporaryPath(aOutput);
   TemporaryFileGuard aCleanup(aTemporary);
   ConversionResult aResult{.chDelimiter = chDelimiter};
   std::vector<std::string> vecFields;
   {
      OpenXLSX::XLDocument aDocument;
      aDocument.create(aTemporary.string(), OpenXLSX::XLDoNotOverwrite);
      auto aSheet = aDocument.workbook().worksheet("Sheet1");
      while(aReader.ReadRow(vecFields)) {
         if(aResult.uRows >= 1'048'576) {
            throw std::runtime_error("Excel worksheet exceeds 1048576 rows");
            }
         if(vecFields.size() > 16'384) {
            throw std::runtime_error("CSV record exceeds 16384 Excel columns");
            }
         ++aResult.uRows;
         aResult.uMaxColumns = static_cast<std::uint16_t>(
            std::max<std::size_t>(aResult.uMaxColumns, vecFields.size()));
         for(std::size_t uColumn = 0; uColumn < vecFields.size(); ++uColumn) {
            try {
               ValidateExcelText(vecFields[uColumn]);
               aSheet.cell(aResult.uRows, static_cast<std::uint16_t>(uColumn + 1))
                  .value() = vecFields[uColumn];
               }
            catch(std::exception const& aError) {
               throw std::runtime_error(std::format("Row {}, column {}: {}",
                  aResult.uRows, uColumn + 1, aError.what()));
               }
            }
         }
      aDocument.save();
      aDocument.close();
      }
   if(!fs::is_regular_file(aTemporary) || fs::file_size(aTemporary) == 0) {
      throw std::runtime_error("XLSX output is missing or empty");
      }
   if(fs::exists(aOutput)) {
      throw std::runtime_error("XLSX destination was created by another process");
      }
   fs::rename(aTemporary, aOutput);
   return aResult;
   }

} // namespace adecc::csv_to_xlsx
