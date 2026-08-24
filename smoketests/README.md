# Evidence Test Sources

This directory contains version-independent consumer tests used as **evidence for third-party libraries built with Embarcadero C++Builder/BCC64X**.

The tests are only one component of the wider C++Builder Third-Party Integration project. They are not the BuildEngine program and they do not replace upstream library test suites.

Test paths remain stable across library releases. Version applicability is declared separately in `BuildEngineAdmin/admin/smoke-tests.xml`.

Each test should consume the installed package through its public interface and produce a clear pass/fail result suitable for reproducible evidence logs.
