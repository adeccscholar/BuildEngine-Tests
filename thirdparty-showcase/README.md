# BCC64X Third-Party Integration Showcase

Visible VCL demonstration for the verified Studio 37 / BCC64X third-party stack.

## What the demo proves

One button starts one integrated workflow:

1. **Boost.Beast + OpenSSL** use the existing `adecc::net::HttpsClient` to request
   `https://api.github.com/repos/madler/zlib`.
2. **libcurl** independently requests
   `https://api.github.com/repos/nih-at/libzip`.
3. **nlohmann/json** parses both real responses into typed C++ data and builds one combined JSON summary.
4. **zlib** compresses the generated summary and immediately verifies the memory roundtrip.
5. **libzip** creates `showcase-output/thirdparty-showcase-result.zip` containing both raw responses,
   the generated summary, its zlib representation and a README. The archive is reopened and verified.

The VCL dashboard shows every stage, the HTTP status and byte counts, runtime library versions,
compression ratio, generated JSON and the final ZIP path.

## Build

From the initialized `studio_37` root:

```bat
scripts\58-build-thirdparty-showcase.cmd Release
```

or:

```bat
scripts\58-build-thirdparty-showcase.cmd Debug
```

## Run

```bat
scripts\59-run-thirdparty-showcase.cmd Release
```

The demo intentionally requires Internet access for the two GitHub REST requests. It does not require a token;
for repeated classroom demonstrations, avoid rapid repeated runs because GitHub applies rate limits to unauthenticated requests.

## Source policy

`HttpsClient.cpp` and `HttpsClient.h` are copied unchanged from the existing financial demo / user-supplied client.
The showcase does not modify upstream zlib, libzip, curl, nlohmann/json, Boost or OpenSSL sources.
