C++ Version of OAuth2 authentication
====================================

## Build

To compile and run the main oauth2 example you can use this command:

```
cmake -DCRYPTO_LIB='OpenSSL' ..
cmake --build .
```

## Dependencies

  - [CMake](https://cmake.org)
  - [OpenSSL](https://www.openssl.org) (or a derivative/fork with the same namespace, like [LibreSSL](https://www.libressl.org) or [BoringSSL](https://boringssl.googlesource.com/boringssl))
  - C++—and C—compiler toolchain. Tested with: [clang](https://clang.llvm.org), [g++](https://gcc.gnu.org), and [MSVC](https://docs.microsoft.com/en-us/cpp).

## Tested operating systems (OSs)

  - Linux
  - macOS
  - Windows (WiP)

## Libraries

Normally you can use libraries to do things such as JSON parsing and a small HTTP client and server.
Here we have implemented basic functionality as a teaching aid.
Not all the code is written in the same style - this is on purpose to demonstrate different readability styles.

## References

All references used to create these examples are quotes in the source files.
Most have been significantly altered to improve readability or to carry out the required functionality.
