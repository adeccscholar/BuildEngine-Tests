# C++Builder Third-Party Evidence Tests

This repository is one component of the **C++Builder Third-Party Integration project**. The overall project investigates whether third-party C and C++ libraries can be built, packaged, and consumed reproducibly with **Embarcadero C++Builder / BCC64X**.

This repository does **not** contain the BuildEngine program and it does **not** contain the third-party libraries themselves. It contains independent smoke tests and, where useful, small demonstration programs that provide evidence for the packages produced by the project.

**License:** MIT

## Purpose

A successful library build is only part of the evidence. The produced package must also be usable by an independent C++Builder consumer.

The tests and demos in this repository are therefore designed to answer questions such as:

- can CMake discover the installed package?
- can BCC64X compile a real consumer against its public headers?
- do the installed import/static libraries link correctly?
- are required public compile definitions propagated?
- can the produced DLLs be loaded and used at runtime?
- do selected library features behave correctly in a minimal real program?

These programs are **evidence artifacts for third-party library integration**. They are not the main product and they are not a replacement for the upstream library's own test suite.

## Role in the overall project

```text
C++Builder Third-Party Integration Project
|
+-- BuildEngine program
|     Builds and orchestrates third-party packages
|
+-- BuildEngineAdmin repository
|     Declares tools, libraries, versions and test applicability
|
+-- Evidence Tests repository            <-- this repository
|     Independent consumers, smoke tests and focused demos
|
+-- Third-party upstream sources
|     Built with C++Builder/BCC64X
|
`-- Installed packages and logs
      Evidence produced by the workflow
```

The repository exists to strengthen the evidence for the larger project: **a library is not considered well integrated merely because its producer build completed**.

## Version-independent test sources

A test belongs to a library contract, but normally not to one exact library release. Test directories, CMake projects, and source files therefore do not contain a library version unless a genuinely different test implementation is technically necessary.

Example:

```text
BuildEngineSmokeTests/
|-- README.md
|-- LICENSE
|-- .gitattributes
|-- .gitignore
|-- SHA256SUMS.txt
|-- smoketests/
|   |-- README.md
|   `-- pugixml/
|       `-- installed-consumer-charconv/
|           |-- CMakeLists.txt
|           `-- main.cpp
`-- demos/
    `-- README.md
```

Additional independent checks are added beside the existing test:

```text
smoketests/
`-- pugixml/
    |-- installed-consumer-charconv/
    `-- xpath-basic/
```

Not as versioned source copies such as:

```text
pugixml-1.16/
pugixml-1.17/
```

## Version applicability belongs to the administration repository

Which library versions may use a test is declared in `BuildEngineAdmin/admin/smoke-tests.xml`, not in this repository.

Example:

```xml
<library id="pugixml">
   <smoke id="installed-consumer-charconv"
          source="pugixml\installed-consumer-charconv"
          enabled="true"
          minVersion="1.16">
      <variant name="Release"/>
      <variant name="Debug"/>
   </smoke>
</library>
```

The administration contract may define:

- no version limits: all versions;
- a minimum version: from that version onward;
- a maximum version: up to and including that version;
- both limits: a closed supported range.

This keeps evidence code stable while allowing the build administration to decide whether a specific test is valid for a specific library release.

## Synchronization into the BuildEngine workspace

The repository content is synchronized into the BuildEngine working directory independently of the administration repository:

```text
<EvidenceGitWorktree>\smoketests\
             |
             | compare by relative path and content hash
             v
<BuildWorkspace>\smoketests\
```

Only source and declarative project files belong in the synchronized tree. Build directories, binaries, generated files, and runtime logs are not repository content.

Later, GitHub can update the local worktree without changing the build/test architecture. A reproducible run should record the exact evidence-repository commit used.

## Tests use the normal BuildEngine workflow

Smoke tests do not have a second execution engine. Once selected by the administration contract, they are ordinary projects scheduled through the same technical workflow:

```text
Installed third-party package
        |
        v
Evidence-test source
        |
        v
CMake Configure
        |
        v
CMake Build with BCC64X
        |
        v
Execute
        |
        v
Exit code + evaluator + complete raw log
```

The important independence boundary is that the evidence program consumes the **installed package**, not private source files from the library build tree.

## Requirements for evidence tests

A test should normally:

- contain no absolute machine paths;
- contain no hard-coded library release number;
- consume the installed library through its normal public package/include/link contract;
- build with C++Builder/BCC64X through the project's normal toolchain;
- avoid private source-tree dependencies;
- return `0` only when the intended check succeeds;
- emit concise and stable diagnostic text suitable for raw evidence logs;
- keep any required test data inside this repository unless another source is explicitly declared;
- test one clear integration claim rather than duplicate the upstream project's complete test suite.

## Demos as evidence

Small demos may be added when they demonstrate something that is more meaningful than a minimal pass/fail smoke test, for example:

- a visible rendering path for a graphics library;
- an HTTPS request through a networking/TLS stack;
- a CORBA service/client interaction;
- a data-processing example that exercises several public library features together.

A demo is still an **evidence program**, not a product application. It should remain focused, reproducible, and tied to a documented integration claim. A demo does not become an acceptance gate automatically; the administration contract decides which evidence programs are required for a library/profile.

If demos are added as a separate tree, the intended convention is:

```text
demos/<library>/<stable-demo-id>/
```

with the same rule as smoke tests: no release number in the path unless a genuinely incompatible implementation requires a separate source set.

## First evidence test: pugiXML `installed-consumer-charconv`

The initial test deliberately verifies the **installed pugiXML consumer contract**, not the pugiXML source tree.

It checks:

1. `find_package(pugixml CONFIG REQUIRED)`;
2. linking against `pugixml::pugixml`;
3. compile-time visibility of `PUGIXML_CHARCONV_FLOAT`;
4. basic DOM construction and XPath use;
5. `set_value(double)` / `as_double()` round-trip behavior;
6. exit code `0` only after all checks succeed.

This provides evidence that the package produced by the third-party build can actually be consumed by an independent C++Builder project.

## Adding a new evidence test

Add a stable source directory, for example:

```text
smoketests/zlib/compress-roundtrip/
```

Then register it separately in the administration repository:

```xml
<library id="zlib">
   <smoke id="compress-roundtrip"
          source="zlib\compress-roundtrip"
          enabled="true"/>
</library>
```

This separation is intentional:

```text
this repository
   = evidence source code

BuildEngineAdmin
   = library/version applicability and workflow contract

BuildEngine
   = execution infrastructure
```

## What this repository is not

This repository is not:

- the BuildEngine implementation;
- the third-party library source repository;
- a fork of upstream tests;
- a general collection of unrelated examples;
- proof by itself that a library is fully supported.

It is one evidence layer in the broader effort to build and validate third-party libraries with C++Builder/BCC64X.

## License

Project-authored tests, demos, CMake files, and documentation in this repository are licensed under the MIT License. See `LICENSE`.

Third-party libraries remain under their own upstream licenses and are not relicensed by this repository.
