# BuildEngine-Tests Markdown documentation server

This is a standalone **test-support application**, not a production-facing API. It renders repository Markdown directly with a cmark-gfm DLL and serves independently prepared JavaScript/CSS and image resources. Navigation groups the library-consumer tests, including Description, build, licenses and SBOM declarations. Feature-dependent syntax highlighting, Mermaid diagrams and MathJax are loaded only when needed.

The server runs on loopback by default. It exposes no build execution endpoint.

**Libraries exercised:** Boost.Asio/Beast (HTTP), cmark-gfm and cmark-gfm extensions (Markdown), Windows socket runtime. Browser assets: highlight.js, Mermaid, MathJax.

[Build instructions](build.md) · [License](LICENSE.md) · [SBOM](SBOM.md).

## Database navigation

The server recursively indexes `database/`, including `database/common`, MSSQL and PostgreSQL. Parent sections do not generate non-existent license/SBOM links. The shared C++ and original Test.zip headers as well as backend SQL creation scripts can be viewed in the read-only syntax-highlighted source viewer; they cannot be executed by the server.
