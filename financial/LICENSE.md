# Licenses — financial

## Project-authored sources

Copyright (c) 2026 adecc Systemhaus GmbH. The project-authored source code and documentation in this directory are distributed under the complete **PolyForm Noncommercial License 1.0.0** reproduced below (including the Required Notice).

Earlier distributions under MIT remain under their original grants; licensing of this revision does not revoke earlier grants. The project license never changes the upstream terms of third-party software.

## Components and their upstream licenses

The following register covers the source-declared and known BuildEngine package dependencies for this project. They retain their own terms. The published source paths are references; for distributing binaries, preserve the **exact unmodified license and attribution files** from the installed BuildEngine package and its source/SBOM evidence.

| Component | Upstream license / SPDX expression | Authoritative source |
| --- | --- | --- |
| Boost (`boost`) | `BSL-1.0` | [Upstream terms](https://www.boost.org/LICENSE_1_0.txt) |
| pugixml (`pugixml`) | `MIT` | [Upstream terms](https://github.com/zeux/pugixml/blob/master/LICENSE.md) |
| OpenSSL (`openssl`) | `Apache-2.0` | [Upstream terms](https://github.com/openssl/openssl/blob/master/LICENSE.txt) |
| zlib (`zlib`) | `Zlib` | [Upstream terms](https://github.com/madler/zlib/blob/master/LICENSE) |
| Brotli (`brotli`) | `MIT` | [Upstream terms](https://github.com/google/brotli/blob/master/LICENSE) |
| Zstandard (`zstd`) | `BSD-3-Clause` | [Upstream terms](https://github.com/facebook/zstd/blob/dev/LICENSE) |

**Component scope:** This is a source-contract inventory, not proof of a complete transitive binary dependency closure. Its actual versions are selected by the active `admin/build-libraries.xml` catalog or are currently pinned in the project's build definition. Build and installed PE import lists must be reconciled with the CycloneDX SBOM before a distribution is declared complete. Do not guess absent licenses or mark an unverified expression as verified.

For libjpeg-turbo, the IJG and BSD terms apply to different parts/APIs; preserve upstream README.ijg and LICENSE.md. For Skia and PDFium, embedded third-party notices must be included. GLEW's generator scripts may have separate GPL terms; the runtime licensing entry does not relicense those scripts. XZ's liblzma terms differ from its auxiliary tools. An unknown term is intentionally marked **[nicht verifiziert]**, not silently assigned a permissive license.

**System/toolchain:** Windows/OpenGL platform DLLs, C++Builder/VCL and Embarcadero runtime DLLs are not automatically covered by an open-source license. Consult their vendor terms. Browser JS assets count as redistributed dependencies if shipped.

## Complete project license text

---

# PolyForm Noncommercial License 1.0.0

<https://polyformproject.org/licenses/noncommercial/1.0.0>

## Acceptance

In order to get any license under these terms, you must agree
to them as both strict obligations and conditions to all
your licenses.

## Copyright License

The licensor grants you a copyright license for the
software to do everything you might do with the software
that would otherwise infringe the licensor's copyright
in it for any permitted purpose.  However, you may
only distribute the software according to [Distribution
License](#distribution-license) and make changes or new works
based on the software according to [Changes and New Works
License](#changes-and-new-works-license).

## Distribution License

The licensor grants you an additional copyright license
to distribute copies of the software.  Your license
to distribute covers distributing the software with
changes and new works permitted by [Changes and New Works
License](#changes-and-new-works-license).

## Notices

You must ensure that anyone who gets a copy of any part of
the software from you also gets a copy of these terms or the
URL for them above, as well as copies of any plain-text lines
beginning with `Required Notice:` that the licensor provided
with the software.  For example:

> Required Notice: Copyright Yoyodyne, Inc. (http://example.com)

## Changes and New Works License

The licensor grants you an additional copyright license to
make changes and new works based on the software for any
permitted purpose.

## Patent License

The licensor grants you a patent license for the software that
covers patent claims the licensor can license, or becomes able
to license, that you would infringe by using the software.

## Noncommercial Purposes

Any noncommercial purpose is a permitted purpose.

## Personal Uses

Personal use for research, experiment, and testing for
the benefit of public knowledge, personal study, private
entertainment, hobby projects, amateur pursuits, or religious
observance, without any anticipated commercial application,
is use for a permitted purpose.

## Noncommercial Organizations

Use by any charitable organization, educational institution,
public research organization, public safety or health
organization, environmental protection organization,
or government institution is use for a permitted purpose
regardless of the source of funding or obligations resulting
from the funding.

## Fair Use

You may have "fair use" rights for the software under the
law. These terms do not limit them.

## No Other Rights

These terms do not allow you to sublicense or transfer any of
your licenses to anyone else, or prevent the licensor from
granting licenses to anyone else.  These terms do not imply
any other licenses.

## Patent Defense

If you make any written claim that the software infringes or
contributes to infringement of any patent, your patent license
for the software granted under these terms ends immediately. If
your company makes such a claim, your patent license ends
immediately for work on behalf of your company.

## Violations

The first time you are notified in writing that you have
violated any of these terms, or done anything with the software
not covered by your licenses, your licenses can nonetheless
continue if you come into full compliance with these terms,
and take practical steps to correct past violations, within
32 days of receiving notice.  Otherwise, all your licenses
end immediately.

## No Liability

***As far as the law allows, the software comes as is, without
any warranty or condition, and the licensor will not be liable
to you for any damages arising out of these terms or the use
or nature of the software, under any kind of legal claim.***

## Definitions

The **licensor** is the individual or entity offering these
terms, and the **software** is the software the licensor makes
available under these terms.

**You** refers to the individual or entity agreeing to these
terms.

**Your company** is any legal entity, sole proprietorship,
or other kind of organization that you work for, plus all
organizations that have control over, are under the control of,
or are under common control with that organization.  **Control**
means ownership of substantially all the assets of an entity,
or the power to direct its management and policies by vote,
contract, or otherwise.  Control can be direct or indirect.

**Your licenses** are all the licenses granted to you for the
software under these terms.

**Use** means anything you do with the software requiring one
of your licenses.

Required Notice: Copyright (c) 2026 adecc Systemhaus GmbH

---

## Distribution checklist

1. Preserve the project PolyForm license and its Required Notice.
2. Ship and document every linked or bundled dependency, including browser assets, with the actual version, its exact upstream license/NOTICE texts and transitive dependencies.
3. Reconcile the built executable and staged DLL imports with the per-target CycloneDX SBOM; record checksums and the provenance of each shipped component.
4. Do not describe a declared-only SBOM as a verified release SBOM or infer licenses from filenames.
