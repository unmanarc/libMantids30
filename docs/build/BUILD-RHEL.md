# Building on RHEL, AlmaLinux and Rocky Linux

Applies to RHEL/AlmaLinux/Rocky Linux 7, 8, 9 and 10. Versions 8, 9 and 10 are exercised
by CI; RHEL 7 requires the SCL toolchain and is built manually.

## RHEL / AlmaLinux / Rocky 8, 9, 10

```bash
sudo dnf install -y dnf-plugins-core
# RHEL 8 only:
sudo dnf config-manager --set-enabled powertools
# RHEL 9 and 10 only:
sudo dnf config-manager --set-enabled crb

sudo dnf install -y epel-release
sudo dnf install -y gcc-c++ make cmake pkgconf-pkg-config git \
    boost-devel openssl-devel jsoncpp-devel
```

Optional database backends:

```bash
sudo dnf install -y sqlite-devel libpq-devel mariadb-connector-c-devel
```

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

## RHEL / CentOS 7 (manual build)

RHEL 7 ships GCC 4.8 and OpenSSL 1.0.2, both too old. Use `devtoolset-9` and EPEL's
`openssl11`, which is what the `SSLRHEL7` CMake option expects.

```bash
sudo yum install -y epel-release centos-release-scl
sudo yum install -y devtoolset-9-gcc devtoolset-9-gcc-c++ cmake3 make git \
    boost-devel openssl11-devel jsoncpp-devel

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
sudo dnf install -y jsoncpp openssl boost-regex boost-thread
# optional
sudo dnf install -y sqlite-libs libpq mariadb-connector-c
```

On RHEL 7 use `yum` and add `openssl11-libs` when the library was built with
`-DSSLRHEL7=ON`.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `jsoncpp` not found | enable EPEL: `dnf install -y epel-release` |
| `libcrypto` not found on RHEL 7 | install `openssl11-devel` and configure with `-DSSLRHEL7=ON` |
| `pkg-config` not found | install `pkgconf-pkg-config` (RHEL 8+) |
| `cmake: command not found` on RHEL 7 | use `cmake3` from EPEL |
| Missing `libpq-fe.h` | install `libpq-devel` (RHEL 7: `postgresql-devel`) |
