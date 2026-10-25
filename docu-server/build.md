# Build the Markdown documentation server

See the central [build/start commands](../Docs/start/BUILD.md). Configure independently using the C++Builder 13 BCC64X toolchain and BuildEngine-installed packages, then run `cmake -P docu-server/PrepareDocumentationAssets.cmake` to prepare the test-owned browser JavaScript files.

The resulting executable is `buildengine_tests_docu_server.exe`. Start with `--root . --config Docs/Documentation.xml` and browse `http://127.0.0.1:8781/docs/`.
