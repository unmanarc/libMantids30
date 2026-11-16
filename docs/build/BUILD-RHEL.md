# Building on RHEL

Applies to RHEL 8, 9 and 10. Versions 8, 9 and 10 are exercised
by CI; RHEL 7 is deprecated (EOL November 2024) and remains buildable via the manual
path below for in-tree builds only.

> **RHEL 7 is deprecated.** It ships no new packages; the `SSLRHEL7` / devtoolset-9
> path is kept for existing in-tree builds only.

## RHEL 8, 9, 10

```bash
sudo dnf install -y dnf-plugins-core
# RHEL 8 only:
sudo dnf config-manager --set-enabled powertools
# RHEL 9 and 10 only:
sudo dnf config-manager --set-enabled crb

sudo dnf install -y epel-release
sudo dnf install -y gcc-c++ make cmake pkgconf-pkg-config git \
    openssl-devel
# Boost.JSON headers:
#   Versions 8/9: use EPEL Boost 1.78 for a consistent JSON API.
#   Version 10: use the native Boost packages.
if [ "$(rpm --eval '%{rhel}')" = "8" ] || [ "$(rpm --eval '%{rhel}')" = "9" ]; then
    sudo dnf install -y boost1.78-devel
else
    sudo dnf install -y boost-devel
fi
```

Optional database backends:

```bash
sudo dnf install -y sqlite-devel libpq-devel mariadb-connector-c-devel
```

For versions 8/9, append `-DBoost_DIR=/usr/lib64/cmake/Boost-1.78.0` to the
CMake configure command (shared or static). This selects the versioned EPEL
package, including its `/usr/include/boost1.78` headers.

Build and install:

```bash
cmake -S . -B build-shared \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_INSTALL_LIBDIR=lib \
      -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-shared --parallel "$(nproc)"
sudo cmake --install build-shared
sudo ldconfig
```

Static variant:

```bash
cmake -S . -B build-static -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-static --parallel "$(nproc)"
sudo cmake --install build-static
```

## RHEL / CentOS 7 (deprecated, manual build)

> **RHEL 7 is deprecated (EOL November 2024).** No new packages are built for it;
> this path is kept for existing in-tree builds only.

RHEL 7 ships GCC 4.8 and OpenSSL 1.0.2, both too old. Use `devtoolset-9` and EPEL's
`openssl11`, which is what the `SSLRHEL7` CMake option expects.

```bash
sudo yum install -y epel-release centos-release-scl
sudo yum install -y devtoolset-9-gcc devtoolset-9-gcc-c++ cmake3 make git \
    boost-devel openssl11-devel

# Enable the C++17 capable toolchain in the current shell
scl enable devtoolset-9 bash
```

```bash
cmake3 -S . -B build-shared \
       -DCMAKE_BUILD_TYPE=Release \
       -DBUILD_SHARED_LIBS=ON \
       -DSSLRHEL7=ON \
       -DCMAKE_INSTALL_LIBDIR=lib \
       -DCMAKE_INSTALL_PREFIX=/opt/osslibs
cmake3 --build build-shared --parallel "$(nproc)"
sudo cmake3 --install build-shared
```

`-DSSLRHEL7=ON` makes the build look for `libssl11` / `libcrypto11` (EPEL) instead of
`libssl` / `libcrypto`. Without it the build will fail on RHEL 7 because OpenSSL 1.0.2 is
too old for the JWT and TLS code.

## Runtime packages

```bash
if [ "$(rpm --eval '%{rhel}')" = "8" ] || [ "$(rpm --eval '%{rhel}')" = "9" ]; then
    sudo dnf install -y boost1.78 openssl
else
    sudo dnf install -y boost openssl boost-json boost-regex boost-thread
fi
# optional
sudo dnf install -y sqlite-libs libpq mariadb-connector-c
```

On RHEL 7 use `yum` and add `openssl11-libs` when the library was built with
`-DSSLRHEL7=ON`.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `boost-json` not found | install `boost1.78-devel` (EPEL) on versions 8/9, or `boost-devel` on version 10 |
| `libcrypto` not found on RHEL 7 | install `openssl11-devel` and configure with `-DSSLRHEL7=ON` |
| `pkg-config` not found | install `pkgconf-pkg-config` (RHEL 8+) |
| `cmake: command not found` on RHEL 7 | use `cmake3` from EPEL |
| Missing `libpq-fe.h` | install `libpq-devel` (RHEL 7: `postgresql-devel`) |
