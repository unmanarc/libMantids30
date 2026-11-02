# Building on Windows with MSYS2 / MinGW64

libMantids30 builds with the MinGW64 toolchain from [MSYS2](https://www.msys2.org/).

## Install the toolchain and dependencies

Open an **MSYS2 MSYS** shell and update the base system:

```bash
pacman -Syu
```

Close and reopen the shell, then install the compiler, build tools and libraries:

```bash
pacman -S --needed base-devel \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-pkgconf \
    mingw-w64-x86_64-boost \
    mingw-w64-x86_64-openssl \
    mingw-w64-x86_64-jsoncpp
```

Optional database backends:

```bash
pacman -S --needed mingw-w64-x86_64-sqlite3 \
    mingw-w64-x86_64-postgresql \
    mingw-w64-x86_64-mariadb-connector-c
```

## Build from the MINGW64 shell

```bash
cmake -S . -B ../libMantids30-BuildWin64 -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_INSTALL_PREFIX=C:/osslibs \
      -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe \
      -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
cmake --build ../libMantids30-BuildWin64 --parallel
cmake --install ../libMantids30-BuildWin64
```

Adjust the MSYS2 installation path if it is not `C:/msys64`, and replace `C:/osslibs` with
your preferred installation prefix.

## Notes

- `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` is enabled by the top level `CMakeLists.txt`, so no
  `.def` files are required.
- The `Net_Interfaces` component uses `tap-windows.h` and is therefore licensed under
  **GPLv2** on WIN32. Installing a TAP adapter requires administrator privileges.
- Static builds (`-DBUILD_SHARED_LIBS=OFF`) work but require the static variants of Boost
  and OpenSSL, which MSYS2 ships as separate `*-static` packages.
