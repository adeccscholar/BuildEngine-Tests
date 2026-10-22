# Legacy project migration

Existing real Financial C++ example in Demo.zip includes JSON, HTTPS/ECB and subtests; it replaces the prior placeholder but sources are not yet in GitHub. Legacy vendored CMake/third-party scripts must not be used.

Original source: user-supplied Demo.zip. C++ sources are not yet committed.

## Additional supplied resources

`Zusätze.zip` supplies 17 scenario JSON datasets under `financial/Tests/`, including IRR/XIRR numerical stress cases, multicurrency, long-running, multiple-root and leap-year scenarios. These are test fixtures, not C++ assert source code. The JSON test fixtures are NOT yet committed to Git.
