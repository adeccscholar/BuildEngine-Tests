#include "CsvReader.h"
#include "Convert.h"

#include <OpenXLSX.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
using namespace adecc::csv_to_xlsx;

namespace {
void Check(bool bCondition, std::string_view svMessage) {
   if(!bCondition) {
      throw std::runtime_error(std::string{svMessage});
      }
   }
} // namespace

#define CHECK(expression) Check((expression), #expression)

int main() {
   {
      std::istringstream aInput{"a;b;c\r\n1;\r\n\"x;y\";\"z\"\"z\"\r\n"};
      CHECK(DetectDelimiter(aInput) == ';');
      CsvReader aReader(aInput, ';');
      std::vector<std::string> vecFields;
      CHECK(aReader.ReadRow(vecFields) && (vecFields == std::vector<std::string>{"a", "b", "c"}));
      CHECK(aReader.ReadRow(vecFields) && (vecFields == std::vector<std::string>{"1", ""}));
      CHECK(aReader.ReadRow(vecFields) && (vecFields == std::vector<std::string>{"x;y", "z\"z"}));
      CHECK(!aReader.ReadRow(vecFields));
      }
   {
      std::istringstream aInput{"\"line1\r\nline2\",=SUM(1;2),00123\n\n"};
      CsvReader aReader(aInput, ',');
      std::vector<std::string> vecFields;
      CHECK(aReader.ReadRow(vecFields));
      CHECK((vecFields == std::vector<std::string>{"line1\r\nline2", "=SUM(1;2)", "00123"}));
      CHECK(aReader.ReadRow(vecFields) && (vecFields == std::vector<std::string>{""}));
      CHECK(!aReader.ReadRow(vecFields));
      }
   {
      std::istringstream aInput{"\"unterminated"};
      CsvReader aReader(aInput, ',');
      std::vector<std::string> vecFields;
      bool bThrown = false;
      try {
         aReader.ReadRow(vecFields);
         }
      catch(std::runtime_error const&) {
         bThrown = true;
         }
      CHECK(bThrown);
      }
   {
      std::istringstream aInput{"\xEF\xBB\xBF" "a,b\n"};
      SkipUtf8Bom(aInput);
      CsvReader aReader(aInput, ',');
      std::vector<std::string> vecFields;
      CHECK(aReader.ReadRow(vecFields) && vecFields[0] == "a");
      }
   {
      std::istringstream aInput{"Column;Amount\nFirst;12,50\nSecond;100,00\n"};
      CHECK(DetectDelimiter(aInput) == ';');
      }
   ValidateExcelText("00123");
   ValidateExcelText("=1+2");
   ValidateExcelText("\xC3\xBC");
   {
      bool bThrown = false;
      try {
         ValidateExcelText("\xC3\x28");
         }
      catch(std::runtime_error const&) {
         bThrown = true;
         }
      CHECK(bThrown);
      }
   {
      bool bThrown = false;
      try {
         ValidateExcelText(std::string(32768, 'x'));
         }
      catch(std::runtime_error const&) {
         bThrown = true;
         }
      CHECK(bThrown);
      }
   fs::path aDirectory = fs::temp_directory_path() / "adecc_csv_to_xlsx_integration";
   fs::create_directories(aDirectory);
   fs::path aInput = aDirectory / "sample.csv";
   fs::path aOutput = aDirectory / "sample.xlsx";
   fs::remove(aOutput);
   {
      std::ofstream aFile(aInput, std::ios::binary);
      aFile << "Name;Number;Formula\r\n\"text;quoted\";00123;=1+2\r\nonlyone\n";
      }
   auto const aResult = ConvertFile(aInput, aOutput);
   CHECK(aResult.uRows == 3);
   CHECK(aResult.uMaxColumns == 3);
   CHECK(aResult.chDelimiter == ';');
   {
      OpenXLSX::XLDocument aDocument;
      aDocument.open(aOutput.string());
      auto aSheet = aDocument.workbook().worksheet("Sheet1");
      CHECK(aSheet.cell(2, 1).value().get<std::string>() == "text;quoted");
      CHECK(aSheet.cell(2, 2).value().get<std::string>() == "00123");
      CHECK(aSheet.cell(2, 3).value().get<std::string>() == "=1+2");
      CHECK(aSheet.cell(3, 1).value().get<std::string>() == "onlyone");
      aDocument.close();
      }
   bool bOverwriteRefused = false;
   try {
      ConvertFile(aInput, aOutput);
      }
   catch(std::runtime_error const&) {
      bOverwriteRefused = true;
      }
   CHECK(bOverwriteRefused);
   fs::remove(aOutput);
   fs::remove(aInput);
   fs::remove(aDirectory);
   return 0;
   }
