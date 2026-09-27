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

The export is intentionally conservative:

| PDF filter | Export |
| --- | --- |
| `/DCTDecode` | `.jpg`, original JPEG stream stored in the PDF |
| `/JPXDecode` | `.jp2`, original JPEG 2000 stream stored in the PDF |
| other or multiple filters | `.bin`, unchanged raw PDF stream |

A `.bin` file is not claimed to be a directly viewable image. Reconstructing filtered sample
data into PNG/TIFF requires interpretation of color space, predictors, masks and possibly several
filter stages and should be implemented as a separate decoding step.

The test reports the image dimensions that are actually stored in the PDF. It cannot recover a
larger source image that was downsampled before or during PDF creation unless the PDF producer
stored that information separately as metadata.

Inline images in page content streams are not extracted by this first version. The target use case
is inserted photographs and graphics represented as image XObjects.

## Build

The test consumes the published BuildEngine `Win64x` SDK view. Configure it with the same
BCC64X toolchain and qpdf package location used for the normal consumers.

Example:

```text
cmake -G Ninja \
   -DCMAKE_TOOLCHAIN_FILE=<BuildEngine Admin>/admin/cmake/toolchains/bcc64x-buildengine-cxx.cmake \
   -Dqpdf_DIR=<InstallRoot>/Win64x/lib/win64/Release/cmake/qpdf \
   -S . -B build

cmake --build build
```

## Run

```text
pdf-image-extraction <input.pdf> <output-directory>
```

Example result:

```text
PDF|version=1.7|pages=2
IMAGE|page=1|name=/Im1|object=17,0|width=1600|height=1200|bpc=8|color=/DeviceRGB|filter=/DCTDecode|raw=184321|file=image-17-0.jpg
RESULT|images=1|unique=1|output=C:\\temp\\pdf-images
```

For JPEG and JPEG 2000 this gives us the strongest possible evidence for the question whether a
higher resolution image is still present inside the PDF: the program exports the encoded image
stream itself instead of a page rendering.
