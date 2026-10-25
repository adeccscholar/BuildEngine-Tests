# Documentation server SBOM

[Declarative CycloneDX SBOM](SBOM-buildengine_tests_docu_server.cdx.json). The installed package versions for Boost and cmark-gfm must be resolved from the active BuildEngine library catalog. Browser JavaScript versions are declared by `PrepareDocumentationAssets.cmake`.

A verified binary SBOM requires PE/DLL imports, final package locks, component hashes and upstream license records. The checked-in declaration is **not yet a complete binary SBOM**.
