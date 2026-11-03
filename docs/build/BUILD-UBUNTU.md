# Building on Debian and Ubuntu

Applies to Debian 12/13 and Ubuntu 20.04/22.04/24.04/26.04 LTS. Ubuntu 22.04, 24.04,
26.04 and Debian 12/13 are exercised by CI.

## Install dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config git \
    libboost-dev libboost-regex-dev libboost-thread-dev libboost-system-dev \
    libssl-dev libboost-json-dev
```

Optional database backends (each module is skipped with a warning when the library is
not present):

```bash
sudo apt install -y libsqlite3-dev libpq-dev libmariadb-dev
```

## Build shared libraries

```bash
cmake -S . -B build-shared \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-shared --parallel "$(nproc)"
sudo cmake --install build-shared
sudo ldconfig
```

## Build static libraries

```bash
cmake -S . -B build-static \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-static --parallel "$(nproc)"
sudo cmake --install build-static
```

Use separate build directories for each variant; both can be installed into the same
prefix. For a self contained deployment prefix use `-DCMAKE_INSTALL_PREFIX=/opt/osslibs`.

## Verify the installation

```bash
pkg-config --modversion Mantids30_Helpers
pkg-config --cflags --libs Mantids30_Protocol_HTTP
ls /usr/local/include/Mantids30
```

## Runtime packages

When copying binaries to another machine you only need:

```bash
sudo apt install -y libssl3 libboost-json1.74.0 libboost-regex1.74.0
```

Adjust the Boost soname to the distribution version, or build statically with
`-DBUILD_SHARED_LIBS=OFF`.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Could NOT find PkgConfig` | `sudo apt install pkg-config` |
| `libcrypto` / `libssl` not found | `sudo apt install libssl-dev` |
| `boost-json` not found | `sudo apt install libboost-json-dev` |
| Undefined Boost symbols | install `libboost-regex-dev libboost-thread-dev` |
| `cmake: command not found` | `sudo apt install cmake` (>= 3.10 required) |
