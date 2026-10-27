# SBOM status

This source package does **not** claim to contain a verified binary CycloneDX SBOM. The dependency declarations are listed in [LICENSE.md](LICENSE.md) and resolved by the installed BuildEngine at configure time.

For release, generate per-executable CycloneDX SBOMs from the exact installed packages and staged DLL closure for `adecc_csv_to_xlsx` and `adecc_csv_to_xlsx_test`. Do not pre-populate an SBOM with unverified package hashes, versions, or transitive dependencies.
