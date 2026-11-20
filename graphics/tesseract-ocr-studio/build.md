# tesseract-ocr-studio — native BCC64X build

Configure using the normal BuildEngine test generator and its existing BCC64X toolchain. The package roots are resolved from `admin/build-libraries.xml`; nothing is fetched from a host SDK.

```bat
cmake --build ../build/Test/tesseract-ocr-studio/Release/cmake --parallel 4
ctest --test-dir ../build/Test/tesseract-ocr-studio/Release/cmake --output-on-failure
cmake --install ../build/Test/tesseract-ocr-studio/Release/cmake
```

The interactive application uses the installed managed DLL closure. Execute `--verify` on the generated runtime executable for native headless test evidence. Debug and Release must use their matching managed packages.

**[not verified]** These commands require the BCC64X Windows host.
