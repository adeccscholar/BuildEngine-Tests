# WeatherHist description

WeatherHist provides a minimal native VCL front-end for one-day Open-Meteo historical weather. It isolates API parsing and date validation from VCL in `HistoryModel.h` and reuses WeatherClient's HTTP and address lookup implementation rather than introducing another HTTP client. `HistoryModelTests.cpp` validates local-noon selection, day totals, optional values, UV provenance and invalid dates. The result view is a two-column VCL TListView; user interaction and UI updates run on the VCL thread. No files are downloaded or saved persistently.

The linked library is the BuildEngine curl DLL, with OpenSSL, zlib, Brotli and Zstandard runtime roots declared. nlohmann/json is an installed header-only package. Native VCL and BCC64X runtime packages come from RAD Studio.
