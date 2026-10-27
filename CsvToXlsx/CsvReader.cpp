#include "CsvReader.h"

#include <array>
#include <cstdint>
#include <format>
#include <ios>
#include <stdexcept>

namespace adecc::csv_to_xlsx {

CsvReader::CsvReader(std::istream& aInput, char chDelimiter)
   : m_aInput(aInput), m_chDelimiter(chDelimiter) {
   if(chDelimiter == '"' || chDelimiter == '\r' || chDelimiter == '\n' || chDelimiter == '\0') {
      throw std::invalid_argument("Invalid CSV delimiter");
      }
   }

bool CsvReader::ReadRow(std::vector<std::string>& vecFields) {
   enum class EState { Start, Plain, Quoted, AfterQuote };
   EState eState = EState::Start;
   std::string strField;
   vecFields.clear();
   bool bReadAnything = false;

   auto FinishField = [&]() -> void {
      vecFields.push_back(std::move(strField));
      strField.clear();
      };

   while(true) {
      int const iNext = m_aInput.get();
      if(iNext == std::char_traits<char>::eof()) {
         if(m_aInput.bad()) {
            throw std::runtime_error("Error while reading CSV input");
            }
         if(eState == EState::Quoted) {
            throw std::runtime_error("Unterminated quoted CSV field");
            }
         if(!bReadAnything) {
            return false;
            }
         FinishField();
         return true;
         }
      bReadAnything = true;
      char const ch = static_cast<char>(iNext);
      if(eState == EState::Quoted) {
         if(ch == '"') {
            eState = EState::AfterQuote;
            }
         else {
            strField.push_back(ch);
            }
         continue;
         }
      if(ch == m_chDelimiter) {
         FinishField();
         eState = EState::Start;
         continue;
         }
      if(ch == '\n' || ch == '\r') {
         if(ch == '\r' && m_aInput.peek() == '\n') {
            m_aInput.get();
            }
         FinishField();
         return true;
         }
      if(eState == EState::AfterQuote) {
         if(ch == '"') {
            strField.push_back('"');
            eState = EState::Quoted;
            continue;
            }
         throw std::runtime_error("Invalid character after closing CSV quotation mark");
         }
      if(ch == '"') {
         if(eState != EState::Start) {
            throw std::runtime_error("Unquoted quotation mark in CSV field");
            }
         eState = EState::Quoted;
         continue;
         }
      strField.push_back(ch);
      eState = EState::Plain;
      }
   }

void SkipUtf8Bom(std::istream& aInput) {
   std::istream::pos_type const aStart = aInput.tellg();
   if(aStart == std::istream::pos_type(-1)) {
      throw std::runtime_error("Cannot seek in input stream");
      }
   std::array<unsigned char, 3> aBytes{};
   aInput.read(reinterpret_cast<char*>(aBytes.data()), static_cast<std::streamsize>(aBytes.size()));
   if(aInput.gcount() != 3 || aBytes != std::array<unsigned char, 3>{0xEF, 0xBB, 0xBF}) {
      aInput.clear();
      aInput.seekg(aStart);
      if(!aInput) {
         throw std::runtime_error("Cannot restore input stream after BOM detection");
         }
      }
   }

char DetectDelimiter(std::istream& aInput) {
   std::istream::pos_type const aStart = aInput.tellg();
   if(aStart == std::istream::pos_type(-1)) {
      throw std::runtime_error("Cannot seek in input stream");
      }
   std::array<std::size_t, 3> aCounts{};
   std::array<std::size_t, 3> aFirstCounts{};
   constexpr std::array<char, 3> aCandidates{',', ';', '\t'};
   bool bQuoted = false;
   bool bFirstComplete = false;
   bool bPreviousCR = false;
   std::size_t uRows = 0;
   std::size_t uBytes = 0;
   while(uRows < 32 && uBytes < 128 * 1024) {
      int const iNext = aInput.get();
      if(iNext == std::char_traits<char>::eof()) {
         break;
         }
      ++uBytes;
      char const ch = static_cast<char>(iNext);
      if(ch == '"') {
         bQuoted = !bQuoted;
         }
      else if(!bQuoted) {
         for(std::size_t uIndex = 0; uIndex < aCandidates.size(); ++uIndex) {
            if(ch == aCandidates[uIndex]) {
               ++aCounts[uIndex];
               if(!bFirstComplete) {
                  ++aFirstCounts[uIndex];
                  }
               }
            }
         if(ch == '\r') {
            ++uRows;
            bFirstComplete = true;
            bPreviousCR = true;
            }
         else if(ch == '\n') {
            if(!bPreviousCR) {
               ++uRows;
               }
            bFirstComplete = true;
            bPreviousCR = false;
            }
         else {
            bPreviousCR = false;
            }
         }
      }
   if(aInput.bad()) {
      throw std::runtime_error("Error while detecting CSV delimiter");
      }
   aInput.clear();
   aInput.seekg(aStart);
   if(!aInput) {
      throw std::runtime_error("Cannot restore stream after CSV delimiter detection");
      }
   // A unique delimiter on the first record is a stronger signal than
   // decimal commas on subsequent rows of a German semicolon-separated file.
   std::size_t uHeaderNonzero = 0;
   std::size_t uHeaderIndex = 0;
   for(std::size_t uIndex = 0; uIndex < aFirstCounts.size(); ++uIndex) {
      if(aFirstCounts[uIndex] > 0) {
         ++uHeaderNonzero;
         uHeaderIndex = uIndex;
         }
      }
   if(uHeaderNonzero == 1) {
      return aCandidates[uHeaderIndex];
      }
   std::size_t uBest = 0;
   for(std::size_t uIndex = 1; uIndex < aCounts.size(); ++uIndex) {
      if(aCounts[uIndex] > aCounts[uBest]) {
         uBest = uIndex;
         }
      }
   return aCandidates[uBest];
   }

void ValidateExcelText(std::string const& strValue) {
   std::size_t uUnits = 0;
   for(std::size_t uIndex = 0; uIndex < strValue.size();) {
      unsigned char const ch = static_cast<unsigned char>(strValue[uIndex]);
      std::uint32_t uPoint = 0;
      std::size_t uLength = 0;
      if(ch <= 0x7f) {
         uPoint = ch;
         uLength = 1;
         }
      else if(ch >= 0xc2 && ch <= 0xdf) {
         uPoint = ch & 0x1f;
         uLength = 2;
         }
      else if(ch >= 0xe0 && ch <= 0xef) {
         uPoint = ch & 0x0f;
         uLength = 3;
         }
      else if(ch >= 0xf0 && ch <= 0xf4) {
         uPoint = ch & 0x07;
         uLength = 4;
         }
      else {
         throw std::runtime_error("Invalid UTF-8 leading byte");
         }
      if(uIndex + uLength > strValue.size()) {
         throw std::runtime_error("Incomplete UTF-8 character");
         }
      for(std::size_t uOffset = 1; uOffset < uLength; ++uOffset) {
         unsigned char const chNext = static_cast<unsigned char>(strValue[uIndex + uOffset]);
         if((chNext & 0xc0) != 0x80) {
            throw std::runtime_error("Invalid UTF-8 continuation byte");
            }
         uPoint = (uPoint << 6) | (chNext & 0x3f);
         }
      if((uLength == 2 && uPoint < 0x80) ||
         (uLength == 3 && uPoint < 0x800) ||
         (uLength == 4 && uPoint < 0x10000) ||
         (uPoint >= 0xd800 && uPoint <= 0xdfff) || uPoint > 0x10ffff) {
         throw std::runtime_error("Invalid UTF-8 scalar value");
         }
      bool const bAllowed = (uPoint == 9 || uPoint == 10 || uPoint == 13 ||
         (uPoint >= 0x20 && uPoint <= 0xd7ff) ||
         (uPoint >= 0xe000 && uPoint <= 0xfffd) ||
         (uPoint >= 0x10000 && uPoint <= 0x10ffff));
      if(!bAllowed) {
         throw std::runtime_error("XML-incompatible character in CSV field");
         }
      uUnits += (uPoint > 0xffff ? 2 : 1);
      if(uUnits > 32767) {
         throw std::runtime_error("Excel cell text exceeds 32767 UTF-16 code units");
         }
      uIndex += uLength;
      }
   }

} // namespace adecc::csv_to_xlsx
