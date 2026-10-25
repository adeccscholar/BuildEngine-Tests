# CycloneDX SBOM — PolicyTest

- [SBOM-adecc_policy_test.cdx.json](SBOM-adecc_policy_test.cdx.json)

The SBOM represents **declared source dependencies only**: the standalone adecc-core-lib and the C++Builder/Windows runtime. At install time compare the real imports and installed DLL hashes with the declaration before marking a release as verified. The test does not redistribute a database driver or a third-party parsing library.