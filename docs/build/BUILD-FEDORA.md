# Building on Fedora

Applies to Fedora 40 and newer.

## Install dependencies

```bash
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake pkgconf-pkg-config git \
    boost-devel boost-json-devel openssl-devel
```

Optional database backends:

```bash
sudo dnf install -y sqlite-devel libpq-devel mariadb-connector-c-devel
```

## Build and install

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

## Install from COPR instead

```bash
sudo dnf install -y 'dnf-command(config-manager)' epel-release
sudo dnf copr enable amizrachi/unmanarc -y
sudo dnf install -y libMantids30-devel
```

## Runtime packages

```bash
sudo dnf install -y boost-json openssl-libs boost-regex boost-thread
# optional
sudo dnf install -y sqlite-libs libpq mariadb-connector-c
```
