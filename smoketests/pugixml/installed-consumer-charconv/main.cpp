// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

#include <pugixml.hpp>

#include <cmath>
#include <print>

#ifndef PUGIXML_CHARCONV_FLOAT
#error PUGIXML_CHARCONV_FLOAT must be propagated by the installed pugixml target.
#endif

int main() {
   constexpr double flExpectedValue = 1234.125;

   pugi::xml_document theDocument;
   auto theRoot = theDocument.append_child("root");
   auto theValue = theRoot.append_child("value");

   if (!theValue.text().set(flExpectedValue)) {
      std::println(stderr, "PUGIXML_SMOKE_ERROR: unable to set double value");
      return 1;
      }

   auto const theSelected = theDocument.select_node("/root/value");
   if (!theSelected) {
      std::println(stderr, "PUGIXML_SMOKE_ERROR: XPath selection failed");
      return 2;
      }

   auto const flValue = theSelected.node().text().as_double();
   if (std::abs(flValue - flExpectedValue) > 1.0e-12) {
      std::println(stderr, "PUGIXML_SMOKE_ERROR: double roundtrip mismatch");
      return 3;
      }

   std::println("PUGIXML_SMOKE_PASS: installed consumer charconv roundtrip");
   return 0;
   }
