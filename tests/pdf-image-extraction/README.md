# PDF image extraction repro

This integration test inspects image XObjects in an existing PDF and writes the image streams to
an output directory.

The first implementation deliberately uses QPDF as the structural PDF layer. It does not render
the page and therefore does not reduce an embedded image to screen resolution.

## Reported information

For every image occurrence the program reports and writes to `images.tsv`:

- page number;
- resource name;
- PDF object/generation identifier;
- pixel width and height;
- bits per component;
- color space;
- PDF filter chain;
- presence of `/ImageMask`, `/Mask` and `/SMask`;
- size of the raw stream stored in the PDF;
- exported file name.

The same indirect image object may be referenced from several pages or form XObjects. It is
reported for every occurrence, but its stream is exported only once.

## Export semantics

| PDF filter | Export |
| --- | --- |
| `/DCTDecode` | `.jpg`, JPEG stream stored in the PDF |
| `/JPXDecode` | `.jp2`, JPEG 2000 stream stored in the PDF |
| other or multiple filters | `.bin`, unchanged raw PDF stream |

A `.bin` file is not claimed to be a directly viewable image. Reconstructing filtered sample
data into PNG/TIFF requires interpretation of color space, predictors, masks and possibly several
filter stages.

The test reports image dimensions actually stored in the PDF. It cannot recover a larger source
image that was downsampled before or during PDF creation unless the PDF producer stored that
information separately.

Inline images in page content streams are not extracted by this first version.

## Bootstrap

The repro deliberately does not assume that Ninja is installed globally. It follows the same
Stage-0 model used by DeckKernel.

Start a C++Builder Developer Command Prompt so that `BDS` is available, then run from the
BuildEngine-Tests repository root:

```bat
cmake -DBUILDENGINE_ROOT=D:/local/embarcadero/test_v3 -P bootstrap/Bootstrap.cmake
```

The bootstrap:

- obtains `bcc64x.exe` from the active `BDS` installation;
- validates the BuildEngine BCC64X toolchain;
- validates the published QPDF package;
- provisions pinned Ninja 1.13.2 below `Cache/tools` when required;
- writes machine-local evidence to `Cache/BootstrapTools.cmake`;
- generates ready-to-run configure and build commands.

## Configure

From the repository root:

```bat
Cache\configure-pdf-image-extraction.cmd
```

The generated command sets `CB_BDS` and `CB_BCC64X` and invokes the exact CMake and Ninja
recorded by the bootstrap. No global Ninja installation is required.

## Build

```bat
Cache\build-pdf-image-extraction.cmd
```

## Run

```bat
tests\pdf-image-extraction\build\pdf-image-extraction.exe <input.pdf> <output-directory>
```

Example result:

```text
PDF|version=1.7|pages=2
IMAGE|page=1|name=/Im1|object=17,0|width=1600|height=1200|bpc=8|color=/DeviceRGB|filter=/DCTDecode|raw=184321|file=image-17-0.jpg
RESULT|images=1|unique=1|output=C:\\temp\\pdf-images
```

For JPEG and JPEG 2000 this gives strong evidence for whether a higher-resolution image is still
present inside the PDF: the encoded image stream stored in the PDF is exported instead of a page
rendering.
