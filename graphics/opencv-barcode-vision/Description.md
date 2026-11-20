# OpenCV Barcode Vision

## Experience

Use the **OpenCV 5.1** barcode detector to locate and decode EAN-13 symbols, or select the native OpenCV QR-code detector. The application displays actual decoded content, image overlays, measured processing time and optional exact round-trip verification. Load your own PNG/JPEG image, or generate a deterministic test image using the separately managed **ZXing-C++ 3.1.1** writer with its external **Zint 2.16.0** backend.

## Architecture

- **OpenCV**: `cv::barcode::BarcodeDetector::detectAndDecodeWithType` for EAN/UPC, `cv::QRCodeDetector::detectAndDecode` for QR, `cv::imread` for imported images and `cv::imgproc` for rendering diagnostic boundaries.
- **ZXing + Zint**: real test symbol generation only, never a hidden decoder fallback.
- **SDL2 + ImGui**: native interactive display; OpenCV image pixels and detector output provide every visible result.

The EAN/UPC test is a specific valid **EAN-13** sample; OpenCV does not promise to decode every linear symbology supported by ZXing. The QR detector is an independent OpenCV API.

## Verification

Run `--verify` to generate EAN-13 and QR pixels with ZXing/Zint and require OpenCV itself to decode the original expected content. Reported results use `SHOWCASE|CHECK` and `SHOWCASE|RESULT`. The test does not need camera access or an external sample file.

**[not verified]** Native Windows BCC64X compilation and EAN/QR recognition success are pending.

## Step-by-step image processing pipeline

The sample begins by creating a valid EAN-13 or QR symbol through ZXing-C++ and the separately linked Zint writer. The original generated pixels are transferred to an OpenCV image buffer, with quiet zones preserved and a neutral white canvas around the symbol. This is a genuine cross-library exercise: ZXing is responsible for encoding, while OpenCV is solely responsible for the detection and decoded result. The OpenCV barcode detector and QR detector are independent APIs selected by the UI; unsupported or undecodable symbols remain genuine unsuccessful detection outcomes.

For imported images, the program calls OpenCV `imread` and applies the selected detector directly. The application does not replace the input content with a generated fixture and does not silently switch to ZXing as a fallback decoder. A green quadrilateral is drawn only from actual detector points, and its coordinates come from OpenCV. The original source image, recognized strings, detected symbology and measured decoding time are displayed together, allowing real inspection of the recognition quality.

The `--verify` path uses deterministic data to exercise the complete writer-to-reader dependency chain without needing hardware, files outside the repository or network access. The exact comparison establishes that the result was decoded by OpenCV rather than merely drawn by a generator. This test is an integration assertion, not a guarantee of recognition quality for damaged, rotated or low-resolution real-world barcodes.

For full single-line configuration, CTest, installation and direct execution commands, see [build.md](build.md). Commands are intentionally separated into individually copyable lines.

## Native Windows camera discovery and live barcode scanning

The application now lists camera devices registered in the Windows **DirectShow video input category** using the Windows System Device Enumerator (`ICreateDevEnum` and `IEnumMoniker`). It retrieves the friendly name and, where provided, the device path of each device. Importantly, **listing a camera does not attempt to open it**: an offline, busy or disconnected camera can therefore still appear and be selected, provided that Windows exposes its DirectShow registration. Windows-disabled devices may be omitted by the operating system. The application never assumes that an index is online merely because it is shown in the list.

Choose the desired entry from **DirectShow device**, then press **CONNECT SELECTED** to request that exact source through the already managed OpenCV `cv::VideoCapture::open(index, cv::CAP_DSHOW)` implementation. Alternatively, enter a DirectShow index explicitly and use **CONNECT BY INDEX**, useful when comparing OpenCV's index mapping with the configured Windows camera inventory. The selected index is displayed during capture. **DISCONNECT** releases the device immediately, and choosing another device closes the previous stream. **REFRESH REGISTERED CAMERAS** rebuilds the list without probing each device and attempts to preserve a selection by its returned device path rather than by an assumed stable index.

Connected video frames are captured through native OpenCV `videoio`, optionally analyzed with the same OpenCV EAN-13 or QR algorithms as the file and generated-image modes, and displayed through a streaming **SDL2 texture**. The streaming texture preserves native image detail and avoids issuing tens of thousands of ImGui rectangles per video frame. The interface reports connection status, index, received frames, read failures, decoded content and timing. Detection can be disabled without disconnecting, and frame acquisition and recognition are rate-limited separately to avoid invoking the full detector on every GUI redraw.

The optional `--list-cameras` command enumerates registered devices in a textual diagnostic format, without opening any webcam. It is intentionally separate from the camera-independent `--verify` test, so a CI environment without a camera can still exercise the managed ZXing/Zint-to-OpenCV barcode pipeline. Discovery does not validate camera availability; real acquisition must be verified with a connected camera on Windows.

**[not verified]** The native BCC64X build, DirectShow enumeration order for the user's six configured devices, capture availability, and live recognition quality require validation on the Windows host.

## Recognition latch: read once, track until removal

The live capture now distinguishes **image acquisition**, **barcode recognition**, and **tracking of an already recognized symbol**. Once the OpenCV EAN/QR detector successfully decodes a barcode and supplies its quadrilateral, `LiveBarcodeTracker` captures a grayscale image template from the detected region. Subsequent frames are analyzed through native OpenCV `matchTemplate` within a bounded search window around the previously confirmed location. This is visual tracking, not a repeated invocation of either barcode or QR decoding. The tracked rectangle remains highlighted in green while the same symbol is present.

Recognition is **latched** during tracking: the detector is not called again on every frame or at the previous 180 ms scan cadence. Brief recognition or tracking variations cannot immediately clear the displayed text. If the template fails to match for **five consecutive captured frames**, the tracker releases the current symbol and re-arms decoding. The operator can explicitly override this using **RESET / SCAN NEXT**, or disconnect/change the camera or recognition mode. The last successfully decoded content remains visible in a dedicated **Last confirmed reading (retained)** section even after the tracked symbol disappears, enabling deliberate reading of a URL or a long encoded payload. The UI also reports how many live codes have been confirmed and whether a tracked code is still locked.

The tracking model assumes the symbol retains approximately the same scale and perspective across adjacent frames. Major motion, blur, lighting shifts, occlusion or camera format changes can interrupt a template match. This is not a promise of full six-degree-of-freedom marker tracking. More advanced feature tracking can be substituted in the tracking component without changing the OpenCV decoding interface.

The headless `--verify` now tests the latch with an actual ZXing-generated QR symbol: it must remain tracked through twelve identical camera-frame updates and re-arm only after several blank frames. This test does not require a connected camera and does not claim that live capture performance is verified. **[not verified]** until the local Windows BCC64X build and test execution pass.

## Selectable barcode symbologies and ISBN recognition

The **Detection format** combo box is populated at startup from the installed ZXing-C++ 3.1.1 `ZXing::BarcodeFormats::list(AllReadable)`. It lists the formats that this specific managed binary reports as readable rather than a hardcoded assortment of unsupported names. The initial setting is **Automatic (OpenCV + ZXing)**: the application first attempts its native OpenCV QR detector, then the OpenCV linear barcode detector, and finally uses the separately managed ZXing-C++ reader for additional supported barcode and matrix-code formats. Only an actual successful decode returns text and quadrilateral geometry; the program does not fabricate detections.

Choosing a particular format restricts the ZXing decoder using `ReaderOptions::setFormats`. This can be valuable when the automatic scan fails or when unrelated visible symbols should not be considered. Supported selections depend on the installed ZXing build and can include the EAN/UPC family, Code 39, Code 93, Code 128, Codabar, ITF, PDF417, Aztec, Data Matrix, QR variants, Micro QR and ISBN. A format being supported by the library does not guarantee detection under all camera lighting, resolution, motion or focus conditions. The new selection applies consistently to generated images, imported files and live camera frames; changing the selection explicitly clears the current visual tracking lock.

**ISBN-13** is normally carried by an EAN-13 book barcode with the `978` or `979` prefix. The application checks both the Bookland prefix and the EAN-13 check digit before labeling a decoded number as an ISBN. A valid `978`-prefixed ISBN-13 also yields the mathematically corresponding **ISBN-10**, including the possible `X` check digit; `979` ISBN-13 numbers are never incorrectly presented as convertible to ISBN-10. The combo box also includes ZXing's own readable `ISBN` format when the package exposes it. The retained last reading displays the book number and any valid derived ISBN-10. ISBN validation and conversion are metadata processing after library decoding, not an additional synthetic barcode detector.

The **Generate + Detect** fixture controls still create only EAN-13 and QR test images through the existing managed ZXing/Zint writer. The wider format combo box is a decoder selection, not an assertion that every readable format can also be generated. This distinction preserves the independent cross-library OpenCV tests while exposing the broader ZXing-C++ reading functionality.

The headless integration test checks runtime format enumeration, QR decoding with an explicitly selected ZXing format, a real `978` EAN-13 test image, ISBN-13 check-digit validation and the ISBN-10 conversion `9780306406157` → `0306406152`. **[not verified]** until native BCC64X execution.

## ISBN lookup: public catalog metadata and book cover

A recognized and checksum-validated `978`/`979` ISBN-13 can trigger an **asynchronous HTTPS REST lookup**. The initial option **Auto lookup after confirmed ISBN** is enabled; it starts one request on a newly confirmed ISBN event, never on every camera frame. The existing camera tracking keeps the decoded ISBN visible and avoids duplicate reads. Operators may disable automatic lookup or type a hyphenated ISBN in the field and press **LOOK UP BOOK**. Validated ISBNs alone are sent to the public services.

The application uses the installed **curl 8.23.0** shared library for transport, with TLS certificate and hostname verification, short network timeouts, bounded responses and an allowlist of catalog and cover endpoints. **nlohmann-json 3.12.0** parses the REST replies; OpenCV `imgcodecs` decodes available cover images into the existing native SDL2 preview component. The REST task runs asynchronously, without intentionally blocking the video or rendering loop. No extra network or JSON library is copied into the showcase.

The lookup queries **Open Library's ISBN Books API** using `/api/books?bibkeys=ISBN:<digits>&jscmd=data&format=json`. Only an exact ISBN record with title is accepted. On absence or failure, it queries the official **Google Books volumes API** with `q=isbn:<digits>`; Google results are checked against their explicit `industryIdentifiers`, rejecting unrelated search matches. Where supplied, it displays title, subtitle, authors, publisher, publication date and catalog source. A cover is downloaded from a returned thumbnail URL only when its host is accepted, decoded using OpenCV and shown next to the metadata; lack of a cover is explicitly signalled. A catalog not knowing a valid ISBN is **not** presented as an invalid book.

The concrete acceptance ISBN **978-3-6967-1009-5** normalizes to `9783696710095` and validates as a Bookland EAN/ISBN-13. Its derived ISBN-10 is `3696710091`. The official BoD publisher record independently confirms **Die Antwort klingt richtig**, by **Volker Hillmann**, published by **BoD – Books on Demand** on **26 August 2026**, 168 pages: https://buchshop.bod.de/die-antwort-klingt-richtig-volker-hillmann-9783696710095 . This publisher information is a manually verified external reference, **not** a claim that either Open Library or Google Books currently contains the same edition.

The camera-independent `--verify` checks the exact ISBN-13 validation and ISBN-10 conversion without any remote dependency. For a genuine live REST integration check, run `--lookup-isbn 978-3-6967-1009-5`. It prints structured publisher-independent catalog evidence and cover availability and returns a nonzero exit code when neither public catalog reports an exact match. Network lookup success and cover availability remain **[not verified]** until tested on the Windows BCC64X host.

## Windows TLS trust and remote-catalog failure diagnostics (20 November 2026)

In the first real ISBN lookup, Open Library timed out and Google Books rejected the TLS peer certificate. Neither condition establishes that the ISBN is absent. The managed curl 8.23.0 package is built against OpenSSL, so it must explicitly request the native Windows CA certificate store with `CURLOPT_SSL_OPTIONS = CURLSSLOPT_NATIVE_CA`. Strict `CURLOPT_SSL_VERIFYPEER=1` and `CURLOPT_SSL_VERIFYHOST=2` are retained. This is a legitimate curl/OpenSSL feature, not a certificate-check bypass. The certificate store must still contain a valid trust chain for each endpoint. An enterprise proxy with an untrusted interception certificate will continue to fail as intended.

Each provider now records its own response outcome. Open Library uses a bounded 1.5-second connection timeout and a 3-second total deadline before the Google Books fallback is attempted. Errors differentiate network timeout, TLS certificate validation failure, HTTP response failure, and absence of an exact matching record. The UI and `--lookup-isbn` diagnostic print independent **Open Library** and **Google Books** statuses. A timeout or certificate failure is not rendered as "book not listed". The network path remains asynchronous in the interactive application. **[not verified]** until BCC64X Windows execution.

No CA bundle is downloaded, no TLS verification is disabled, and no provider metadata is fabricated when both catalogs are unavailable.

## REST backoff, HTTP 429 and ISBN caching (20 November 2026)

The Google Books REST endpoint returned HTTP 429 after the native Windows CA trust correction, while Open Library continued timing out. Neither is evidence that the ISBN is absent. The shared HTTP client now parses a numeric HTTP `Retry-After` response header and reports 429 as a distinct `RateLimited` exception. The provider is **not retried immediately**. If no numeric `Retry-After` is available, the client uses a five-minute conservative backoff; when a numeric value is present, it is bounded to one day. Provider rate-limit state is shared in memory across ISBN requests.

After an Open Library timeout, new requests skip that provider for 60 seconds rather than spending three seconds repeatedly attempting unreachable requests. Results are cached by validated ISBN: exact book metadata for 24 hours, and no-result or provider-error outcomes for ten minutes. Cached outcomes are explicitly labelled in their status. The fallback logic never turns HTTP 429 or network timeout into a claim that a valid ISBN is unknown. Different HTTP providers retain their independent status messages.

The cache and throttling state are **per process**. Restarting the executable resets both; an API key, persistent library database or external cache service is not silently introduced. Rate limits imposed by a remote provider cannot be bypassed by code. A permanent quota restriction or denied unauthenticated access requires a supported provider access arrangement rather than faster repeated calls. Strict TLS verification and the existing managed libcurl/OpenSSL package boundaries remain in force.

**[not verified]** Native BCC64X runtime behavior and live provider responses still require testing on the Windows host.
