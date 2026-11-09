# libMantids30

[![CI](https://github.com/unmanarc/libMantids30/actions/workflows/ci.yml/badge.svg)](https://github.com/unmanarc/libMantids30/actions/workflows/ci.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-brightgreen.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/)

**Mini-Advanced C++ Network Toolkit for Internet Services Development**

`libMantids30` is a C++17 framework for building network services, REST/JSON APIs and
backend daemons. It ships modular libraries that can be used independently: sockets and
chained transports, HTTP/1.1, WebSocket, MIME and FastRPC protocol engines, RESTful and
monolithic web API servers, database abstraction (SQLite3 / MariaDB / PostgreSQL), a typed
memory model, thread-safe containers, JWT/TOTP security helpers and structured logging.

| Property | Value |
|----------|-------|
| Version | 3.0.0 |
| Language standard | C++17 |
| Build system | CMake >= 3.10 |
| License | LGPLv3 (`Net_Interfaces` on WIN32 is GPLv2) |
| Platforms | Linux (RHEL/Alma/Rocky 8-10, Ubuntu 24.04/26.04, Debian 12/13, Fedora), Windows (MSYS2/MinGW64) |

---

## Table of Contents

- [Features](#features)
- [Supported Platforms](#supported-platforms)
- [Requirements](#requirements)
- [Build from Source](#build-from-source)
- [Using libMantids30](#using-libmantids30)
- [Packages (COPR)](#packages-copr)
- [Documentation](#documentation)
- [Real-World Users](#real-world-users)
- [Contributing](#contributing)
- [Security](#security)
- [License](#license)

---

## Features

- **Network programming** - TCP/UDP/UNIX/TLS sockets, acceptors (thread, pool,
  multi-threaded), chained protocols (TLS over TLS, AES, XOR) and interface management.
- **Protocol engines** - HTTP/1.1 client and server, WebSocket, MIME, FastRPC1/3,
  LineRecv and APISync, all built on the shared parsing engine.
- **Web API servers** - RESTful and monolithic JSON API servers with JWT
  authentication, sessions, endpoint requirement maps and WebSocket endpoints.
- **Database abstraction** - unified `SQLConnector` / `Query` / `Transaction` API with
  SQLite3, MariaDB and PostgreSQL backends (each backend is optional at build time).
- **Memory model** - typed abstract variables (`STRING`, `INT64`, `IPV4`, ...),
  chunk-based buffers (`B_Chunks`, `B_MEM`, `B_MMAP`, `B_Ref`) and streamables.
- **Threading** - `Threaded`, `ThreadPool`, `GarbageCollector` and thread-safe
  `Safe::Map` / `Safe::Queue` containers.
- **Security** - JWT tokens with revocation cache, TOTP, SHA/HMAC helpers and AES.
- **Scripting and data** - JSON expression evaluator, MantidsLang, VarsFile
  configuration format and DataTables helper.
- **Logging** - `AppLog`, `WebLog` and `RPCLog` with levels and file rotation.

---

## Supported Platforms

| Platform | Support |
|----------|---------|
| Ubuntu 24.04 / 26.04 LTS | CI-tested |
| Debian 12 / 13 | CI-tested |
| RHEL / AlmaLinux / Rocky Linux 8, 9, 10 | CI-tested |
| RHEL / CentOS 7 (`devtoolset-9`, `cmake3`, `openssl11`) | Deprecated (EOL November 2024), manual build only |
| Fedora 40+ | Packages in COPR |
| Windows 10/11 (MSYS2 / MinGW64) | Supported, documented |

---

## Requirements

### Required

| Dependency | Minimum | Notes |
|------------|---------|-------|
| C++ compiler | GCC >= 9 / Clang >= 10 | |
| CMake | >= 3.10 | |
| pkg-config | any | used to locate OpenSSL |
| Boost | >= 1.71 | `regex`, `thread` and `json` components; RHEL 8 uses EPEL `boost1.78-devel`, RHEL 9/10 `boost-devel` |
| OpenSSL | >= 1.1.1 | `libssl`, `libcrypto` (`libssl11` with `-DSSLRHEL7=ON`) |

### Optional (database backends)

| Dependency | Backend library |
|------------|-----------------|
| SQLite3 | `Mantids30_DB_SQLite3` |
| MariaDB / MySQL client | `Mantids30_DB_MariaDB` |
| PostgreSQL (`libpq`) | `Mantids30_DB_PostgreSQL` |

When an optional library is not found, its module is skipped with a warning and the rest
of the framework still builds.

---

## Build from Source

```bash
git clone https://github.com/unmanarc/libMantids30.git
cd libMantids30

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --parallel "$(nproc)"
sudo cmake --install build
```

### Static and shared libraries

Build each variant in its own build directory:

```bash
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/osslibs
cmake --build build-shared --parallel "$(nproc)"
sudo cmake --install build-shared

cmake -S . -B build-static -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/osslibs
cmake --build build-static --parallel "$(nproc)"
sudo cmake --install build-static
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | `ON` | Build shared libraries instead of static ones |
| `SSLRHEL7` | `OFF` | Use EPEL `openssl11` (`libssl11` / `libcrypto11`) on RHEL 7 |
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Installation prefix |
| `CMAKE_INSTALL_LIBDIR` | distro default | Use `lib` for RHEL-style layouts |
| `CMAKE_BUILD_TYPE` | empty | `Release`, `MinSizeRel`, `Debug`, ... |

Distribution specific instructions:

- [Debian / Ubuntu](docs/build/BUILD-UBUNTU.md)
- [RHEL / AlmaLinux / Rocky Linux](docs/build/BUILD-RHEL.md)
- [Fedora](docs/build/BUILD-FEDORA.md)
- [Windows (MSYS2 / MinGW64)](docs/build/BUILD-WIN32.md)

---

## Using libMantids30

### Installed layout

```
<prefix>/include/Mantids30/<Component>/...        public headers
<prefix>/lib/libMantids30_<Component>.so.3.0.0    shared libraries
<prefix>/lib/libMantids30_<Component>.a           static libraries
<prefix>/share/pkgconfig/Mantids30_<Component>.pc pkg-config metadata
```

Each component is a standalone library (`Mantids30_Helpers`, `Mantids30_Memory`,
`Mantids30_Net_Sockets`, `Mantids30_Protocol_HTTP`, `Mantids30_Server_RESTfulWebAPI`, ...).
Include headers using the installed prefix:

```cpp
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Net_Sockets/socket_tcp.h>
#include <Mantids30/Helpers/random.h>
```

### With CMake and pkg-config

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(MANTIDS_HTTP REQUIRED Mantids30_Protocol_HTTP)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS json regex thread)

add_executable(my_service main.cpp)
target_include_directories(my_service PRIVATE
    ${MANTIDS_HTTP_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(my_service PRIVATE
    ${MANTIDS_HTTP_LIBRARIES}
    Boost::json Boost::regex Boost::thread
    OpenSSL::SSL OpenSSL::Crypto
    pthread)
```

> The generated `.pc` files declare a `Requires: boost_json >= 1.71` entry for the
> components that use JSON. Link every `Mantids30_*` component you use, plus
> OpenSSL and Boost explicitly.

### With a plain Makefile

```bash
pkg-config --cflags --libs Mantids30_Protocol_HTTP Mantids30_Net_Sockets \
                           Mantids30_Helpers Mantids30_Threads Mantids30_Memory \
                           boost_json libssl libcrypto
```

Add `-lboost_regex -lboost_thread -lpthread` if Boost's own `pkg-config` files are not
available on your distribution.

---

## Packages (COPR)

Fedora and RHEL 8+ users can install prebuilt packages:

```bash
sudo dnf install -y 'dnf-command(config-manager)'
sudo dnf install -y epel-release
sudo dnf copr enable amizrachi/unmanarc -y
sudo dnf install -y libMantids30-devel
```

RHEL 7 (deprecated, EOL November 2024):

```bash
sudo yum install -y epel-release yum-plugin-copr
sudo yum copr enable amizrachi/unmanarc -y
sudo yum install -y libMantids30-devel
```

---

## Documentation

- [Documentation index](docs/README.md)
- [Parsing system](docs/PARSER.md) - architecture, strategies and custom protocols
- [Core classes reference](docs/CLASSES.md) - modules, namespaces and classes
- Build guides: [Debian/Ubuntu](docs/build/BUILD-UBUNTU.md),
  [RHEL family](docs/build/BUILD-RHEL.md), [Fedora](docs/build/BUILD-FEDORA.md),
  [Windows](docs/build/BUILD-WIN32.md)

---

## Real-World Users

- [uFastAuthD3](https://github.com/unmanarc/uFastAuthD3) - identity and access management daemon

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). All contributions are expected to follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

---

## Security

Please report vulnerabilities privately as described in [SECURITY.md](SECURITY.md).

---

## License

LGPLv3 - see [LICENSE](LICENSE). The `Net_Interfaces` module is GPLv2 on WIN32 because it
links `tap-windows.h`.

---

## Author

**Aaron Mizrachi** (unmanarc)

- Email: <dev@unmanarc.com>
- GitHub: [unmanarc](https://github.com/unmanarc)
