# adecc-core-lib — separate library and licensing

BuildEngine-Tests is an educational C++Builder / BCC64X test repository.
Its **project-authored code** is distributed under PolyForm Noncommercial
License 1.0.0; see [LICENSE](LICENSE).

The reusable **adecc-core-lib** is a separate, header-only C++23 project:

- Source: https://github.com/adeccscholar/adecc-core-lib
- Library layout: `adecc/cpp_core`, `adecc/cpp_domain`, and optional
  database / OpenXLSX adapters
- Public licensing: PolyForm Noncommercial License 1.0.0
- Additional licensing: adecc Book License for eligible purchasers of
  *Rethinking C++ (C++ neu denken)*:
  https://github.com/adeccscholar/adecc-core-lib/blob/main/adecc/ADECC_BOOK_LICENSE.md

The adecc Book License is a **separate, custom MIT-like permission** covering
the adecc-owned library sources; it does **not** grant commercial rights to
BuildEngine-Tests as a whole. It is still marked as a draft for legal review.

The library is header-only, and clients may consume it from a pinned Git
revision or submodule without compiling a separate adecc library binary.
Its own copyright and license terms remain in force regardless of the
checkout path. Header inclusion does not automatically make unrelated
third-party components subject to the adecc Book License.

Both repositories are intended primarily for teaching, demonstration and
experimentation. The sources are provided **AS IS**, without warranty,
certification or any representation of fitness for production use.
