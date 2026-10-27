#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

namespace adecc::csv_to_xlsx {

/**
\brief Streaming CSV reader with a variable number of columns.
\details Supports RFC 4180 quoting, doubled quotation marks, CRLF/LF/CR record
separators and quoted fields containing line breaks. Every field is a std::string.
*/
class CsvReader final {
public:
   /**
   \brief Construct a reader for an already opened stream.
   \details Does not own the stream. The caller must remove any UTF-8 BOM first.
   \param aInput Input stream opened in binary mode.
   \param chDelimiter Delimiter character, normally comma, semicolon or TAB.
   \returns A constructed reader.
   \throw std::invalid_argument For an invalid delimiter.
   */
   explicit CsvReader(std::istream& aInput, char chDelimiter);

   /**
   \brief Read one complete logical CSV record.
   \details Quoted fields may include multiple physical lines. Trailing empty
   fields and fully empty lines are retained.
   \param vecFields Receives the fields of the next record.
   \returns true when a record is available, false only at clean EOF.
   \throw std::runtime_error For malformed quoting or an I/O error.
   */
   bool ReadRow(std::vector<std::string>& vecFields);

private:
   std::istream& m_aInput;
   char m_chDelimiter;
};

/**
\brief Skip an optional UTF-8 BOM at the beginning of the stream.
\details Leaves the stream at offset zero when there is no complete BOM.
\param aInput A seekable input stream.
\returns Nothing.
\throw std::runtime_error On an I/O or seek failure.
*/
void SkipUtf8Bom(std::istream& aInput);

/**
\brief Guess a CSV delimiter from records outside quoted fields.
\details Examines at most 128 KiB or 32 records and chooses among comma,
semicolon and TAB. In case of a tie, comma wins.
\param aInput Input stream opened in binary mode at the start of data.
\returns The detected delimiter, or comma when no separator is found.
\throw std::runtime_error On an I/O or seek failure.
\note Resets the stream to its original position before returning.
*/
char DetectDelimiter(std::istream& aInput);

/**
\brief Validate a UTF-8 value for Excel's string limits.
\details Rejects malformed UTF-8, XML 1.0 forbidden characters and more than
32767 UTF-16 code units in one cell, without converting it to a numeric type.
\param strValue Field text to be validated.
\returns Nothing.
\throw std::runtime_error For text that Excel cannot safely represent.
*/
void ValidateExcelText(std::string const& strValue);

} // namespace adecc::csv_to_xlsx
