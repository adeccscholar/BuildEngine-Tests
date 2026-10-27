# Project and dependency notices

The source files in this provided example are newly authored for the requested BuildEngine consumer. No distribution license is specified by this project package. The project owner should decide which license applies before external publication or redistribution.

Expected external software components (upstream license identifiers; check exact installed package notices before binary distribution):

| Component | Declared upstream license |
| --- | --- |
| OpenXLSX | BSD-3-Clause |
| pugixml | MIT |
| libzip | BSD-3-Clause |
| Boost | BSL-1.0 |
| zlib | Zlib |
| xz / liblzma | See installed upstream COPYING and license notices |

`OpenXLSX::OpenXLSX` is the directly linked installed dependency; the others are included in the known runtime package closure used by the sibling `OpenXlsxGrid` consumer. Actual redistribution dependencies must be established from the built import closure and their installed BuildEngine license/SBOM metadata. The BCC64X runtime and Windows SDK have separate licensing terms.
