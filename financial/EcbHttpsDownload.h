// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbHttpsDownload.h
\brief Public interface for downloading ECB exchange-rate history.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#pragma once

#include <string>

namespace adecc::financial::ecb {

[[nodiscard]]
std::string DownloadEcbHistoryXml();

}