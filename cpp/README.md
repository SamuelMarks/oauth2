C++ Version of OAuth2 authentication
====================================
![C++](https://img.shields.io/badge/C%2B%2B-17%20|%2020-blue.svg)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![CMake](https://img.shields.io/badge/builder-CMake-blue.svg)]((https://cmake.org))

## Build

To compile and run the main oauth2 example you can use this command:

```
cmake -DCRYPTO_LIB='OpenSSL' ..
cmake --build .
```

### Configuration

Additional flags you can provide are found in the [`CMakeLists.txt`](CMakeLists.txt). For example:
```
-DSTACK_SIZE=16384
-DSERVER_ADDR='127.0.0.1'
-DSERVER_HOST='localhost'
-DPORT_TO_BIND=3000
-DMSG_BACKLOG=5
-DEXPECTED_PATH='/ibm/cloud/appid/callback'
```

## Dependencies

  - [CMake](https://cmake.org)
    - macOS: `brew install openssl` (with [brew](https://brew.sh))
    - Linux: Install `libssl-dev` or `openssl-devel` with your package manager, e.g., `apt`, `dnf`, `yum`, `apk`
    - Windows: various ways. E.g., [vcpkg](https://github.com/microsoft/vcpkg) or [Conan](https://conan.io). Note: this option is also cross-platform.
  - [OpenSSL](https://www.openssl.org) (or a derivative/fork with the same namespace, like [LibreSSL](https://www.libressl.org) or [BoringSSL](https://boringssl.googlesource.com/boringssl))
    - macOS: `brew install openssl`
    - Linux: Install `libssl-dev` or `openssl-devel` with your package manager, e.g., `apt`, `dnf`, `yum`, `apk`
    - Windows: various ways. E.g., [vcpkg](https://github.com/microsoft/vcpkg) or [Conan](https://conan.io). 
  - C++—and C—compiler toolchain. Tested with: [clang](https://clang.llvm.org), [g++](https://gcc.gnu.org), and [MSVC](https://docs.microsoft.com/en-us/cpp).
    - macOS: `xcode-select --install`
    - Linux: `apt install build-essential` on Debian based distributions like Ubuntu; or just install `gcc g++ make` with your favourite package manager
    - Windows: Install [MinGW](http://mingw-w64.org) or [MSVC](https://docs.microsoft.com/en-us/cpp). You'll need the [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk) also.

## Tested operating systems (OSs)

  - Linux
  - macOS
  - Windows

## Libraries

Normally you can use libraries to do things such as JSON parsing and a small HTTP client and server.
Here we have implemented basic functionality as a teaching aid.
Not all the code is written in the same style - this is on purpose to demonstrate different readability styles.

## References

All references used to create these examples are quotes in the source files.
Most have been significantly altered to improve readability or to carry out the required functionality.
