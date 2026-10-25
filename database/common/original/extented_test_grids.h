#pragma once

#include "stream_tools.h"

using namespace std::literals;



inline adecc::vecCaptions<adecc::AnsiStreamPolicy> vecAccountClassesCaps = {
      { "ID",            8, adecc::EAlignmentType::right  },
      { "Name",         20, adecc::EAlignmentType::left  },
      { "Description",  70, adecc::EAlignmentType::left  },
      { "Type Code",    15, adecc::EAlignmentType::left  },
      { "Balance Side", 15, adecc::EAlignmentType::left  }
   };

inline adecc::vecCaptions<adecc::AnsiStreamPolicy> vecAccountCaps = {
      { "ID"s,           8, adecc::EAlignmentType::right  },
      { "Konto"s,       10, adecc::EAlignmentType::right },
      { "Kontoname"s,   50, adecc::EAlignmentType::left },
      { "Kontotyp"s,    10, adecc::EAlignmentType::left },
      { "Kontoklasse"s, 12, adecc::EAlignmentType::left },
      { "Währung"s,     10, adecc::EAlignmentType::center },
      { "Aktiv"s,       10, adecc::EAlignmentType::center }
   };

inline adecc::vecCaptions<adecc::AnsiStreamPolicy> vecPostingCaps = {
      { "ID"s,              9, adecc::EAlignmentType::right },
      { "Buchungsdatum"s,  14, adecc::EAlignmentType::left },
      { "Beleg-Nr."s,      11, adecc::EAlignmentType::left },
      { "Sollkonto"s,      10, adecc::EAlignmentType::right },
      { "Habenkonto"s,     10, adecc::EAlignmentType::right },
      { "Betrag"s,         12, adecc::EAlignmentType::right },
      { "Steuerklasse",    12, adecc::EAlignmentType::left },
      { "Buchungstext"s,   55, adecc::EAlignmentType::left }
   };

inline adecc::vecCaptions<adecc::AnsiStreamPolicy> vecBalAcctByYearCaps = {
      { "Konto"s,          10, adecc::EAlignmentType::right },
      { "Kontoname",       50, adecc::EAlignmentType::left  },
      { "Kontotyp",        20, adecc::EAlignmentType::left  },
      { "Betrag"s,         12, adecc::EAlignmentType::right }
   };