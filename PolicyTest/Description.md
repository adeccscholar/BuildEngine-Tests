# PolicyTest — adecc-core-lib policy acceptance

[TOC]

## Origin and modernization

Migrated from the supplied `PolicyTest.zip` (June 2026). The original Visual Studio sample executed only `Test_Value_based()`; most other calls were commented out. IBAN failures were printed but not counted as failures. It also contained older, local copies of SEPA/IBAN policies. This project instead uses **the current `adecc-core-lib` interface**, specifically:

- `policy_common.h`: `PolicyValue`, `ConvertedPolicyValue`, conversion chains, `AllOfPolicy`, `CaseConversionPolicy`.
- `policies_hawkers_tray.h`: validation and conversion for dates, localized numeric strings, regex, positive numbers and UTF-8-aware string length.
- `sepa_policies_hawkers_tray.h`: `Iban`, `CheckedIban`, German VAT ID, BIC, German postal code and UUID; SEPA text.
- `fixed_numeric.h`: rounding policy variants.

No local copy of any adecc implementation is compiled. The current SEPA character-set and German transliteration utilities remain under `adecc::old`, while the main validation and composition tests use the current `adecc` policy interfaces. These two legacy SEPA helpers are intentionally exercised without copying their implementation. The archive is historical reference material, not a dependency.

## Test groups

| Group | Accepted inputs | Rejected inputs |
| --- | --- | --- |
| IBAN | 50 synthetic IBANs; lowercase/whitespace normalization; grouped output | 8 supplied invalid IBANs plus malformed, empty, punctuation, checksum and length cases |
| Dates | ISO/German formats, 1900 lower boundary, leap day | impossible dates, invalid month/day, pre-1900, future and empty inputs |
| Amounts | German decimal, thousands formatting, positive typed values | zero, negative, malformed numbers, empty and trailing garbage |
| Rounding | Away-from-zero, half-to-even, toward-zero on positive and negative ties | Assertions fail if a rounding rule differs |
| Strings | Email, URL, E.164 phone, BIC, postal code, UUID | missing parts, wrong formats, wrong sizes, punctuation |
| SEPA | Allowed ASCII charset | forbidden characters, blank and overlength |
| VAT DE | Known synthetic valid checksum, normalized and grouped form | invalid check digits, country code, length and characters |

Tests are deterministic, independent of the network, clock-dependent only for rejecting the year 2999, and use no database server.

## Raw IBAN value versus formatted output

`adecc::Iban::Value()` contains the normalized, ungrouped IBAN.
`adecc::Iban::str()` and `std::format("{}", iban)` apply
`IbanGroupedStringPolicy`. Because the underlying `value_type` is also
`std::string`, `static_cast<std::string>(iban)` may resolve to the
raw-value conversion; it is not a reliable assertion of the presentation
contract. The acceptance test verifies `str()` and `std::format`
for both `Iban` and `CheckedIban`.

## Validation scope

IBAN checks validate **generic length 15–34, characters and modulo 97**. They do **not** implement country-specific BBAN lengths, an authoritative country registry or bank-account existence. BIC/URL/email/UUID regex policies validate their explicitly implemented **syntactic** contracts, not business deliverability. The positive IBAN dataset was independently checked with a reference modulo-97 calculation prior to migration.

Every check prints `[PASS]` or `[FAIL]`; final process status is nonzero if an assertion fails. A library behavior divergence is a test failure to investigate, not a reason to silently delete the negative case.

## Documentation

- [Build procedure](build.md)
- [License](LICENSE.md)
- [SBOM](SBOM.md)

The BuildEngine-Tests Markdown server serves this file under `/docs/PolicyTest/Description.md` and the associated evidence in this project folder.