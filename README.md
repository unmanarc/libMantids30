# libMantids 3.0

[![License: LGPLv3](https://img.shields.io/badge/License-LGPL%20v3-brightgreen.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/)

**Mini-Advanced C++ Network Toolkit for Internet Services Development**

**Author:** Aaron Mizrachi (unmanarc) <dev@unmanarc.com>  
**Main License:** LGPLv3  
**WIN32 License for Net_Interfaces:** GPLv2 (tap-windows.h is GPLv2)  

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Core Classes Reference](#core-classes-reference)
- [Quick Start](#quick-start)
- [Programming Examples](#programming-examples)
- [Advantages](#advantages)
- [Documentation](#documentation)
- [Dependencies](#dependencies)
- [Building](#building)
- [Compatibility](#compatibility)
- [C++ Standard](#c-standard)
- [Installation](#installation)

---

## Overview

`libMantids30` is a comprehensive C++17-based framework designed for developing high-performance network services, web APIs, and backend applications. It provides a modular architecture with abstracted components for database access, network programming, memory management, session handling, authentication, and logging.

The library follows a layered architecture pattern, allowing developers to build complex internet services by combining independent modules: from low-level socket handling to high-level RESTful API servers with WebSocket support.

---

## Features

- **Network Programming**
  - Sockets (UDP/TCP/UNIX/TLS)
  - Protocol implementations (HTTP/1.1, MIME, WebSocket, FastRPC)
  - Chained network layers (TLS over TLS, TLS over AES-XOR, etc.)
  - Network interface management

- **Web Servers**
  - HTTP/1.1 Client/Server full implementation
  - RESTful Web API Server with full JWT Support
  - Monolithic JSON Web API Server
  - Session management with client details tracking
  - WebSocket endpoints and connections

- **Database Abstraction Layer**
  - PostgreSQL backend
  - MariaDB/MySQL backend
  - SQLite3 backend (with in-memory and file-backed databases)
  - Unified query interface with parameter binding

- **Memory Management**
  - Abstract variable types (Var, String, Int32, Int64, Bool, Double, etc.)
  - Chunk-based memory allocation
  - Memory-mapped files support

- **Threading**
  - R/W Mutex locks
  - Thread-safe containers (Shared Map, Map, Queue)
  - Thread pools with semaphore-based control
  
- **Security & Authentication**
  - JWT token generation, validation, and revocation
  - Cryptographic functions (SHA Hashing, AES Encryption)
  - TOTP (Time-based One-Time Password) support
  - Session-based authentication

- **Scripting & Data**
  - JSON Expression Evaluation engine
  - VarsFile format for configuration
  - DataTables helper for server-side processing

- **Parsing Engine**
  - Strategy-based parsing system (delimiter, size, validator, multi-delimiter)
  - Composable protocol layering via SubParser chaining
  - Shared parsing core for all protocols (see [Parsing System](docs/PARSER.md))

- **Logging**
  - Structured logging with multiple log levels
  - File-based log storage with rotation
  - Web log
  - Application and module-specific loggers


---

## Core Classes Reference

For a comprehensive reference of all core classes in the `Mantids30` namespace, see [Core Classes Reference](docs/CLASSES.md).

---

## Advantages

| Advantage | Description |
|-----------|-------------|
| **Modular Architecture** | Each component (DB, Network, Memory, etc.) is independent and can be used separately |
| **Database Abstraction** | Write database-agnostic code; switch between SQLite3, PostgreSQL, and MariaDB without code changes |
| **Type-Safe Memory** | Abstract variable system provides type safety while allowing generic handling |
| **High Performance** | Chunk-based memory allocation and optimized network I/O for low-latency services |
| **Thread Safety** | Built-in thread-safe containers and synchronization primitives |
| **Multi-Protocol Support** | HTTP, WebSocket, FastRPC, and MIME protocols out of the box |
| **Parsing Engine** | Strategy-based parsing system with composable SubParser chaining for custom protocol development |
| **Security Features** | JWT, TOTP, cryptographic hashing, and session management included |
| **Cross-Platform** | Supports Linux, Windows (MSYS/MinGW), and various UNIX-like systems |
| **CMake Build System** | Modern CMake configuration with modular library builds |
| **Production Ready** | Used in production systems like uFastAuthD3 (Identity & Access Management) |

---

## Documentation

- **[Parsing System](docs/PARSER.md)** - Architecture, strategies, and guide for implementing custom protocols
- **[Core Classes Reference](docs/CLASSES.md)** - Complete reference of all core classes

---

## Dependencies

### Required
| Dependency | Version | Purpose |
|------------|---------|---------|
| C++ Compiler | C++17 compatible (GCC >= 9, Clang >= 10) | Compilation |
| CMake | >= 3.10 | Build system |
| Boost | >= 1.70 | String algorithms, filesystem |
| OpenSSL | >= 1.1.1 | TLS, Cryptographic functions |
| jsoncpp | >= 1.8.0 | JSON parsing and serialization |
| pthread | - | Threading support |

### Optional
| Dependency | Purpose |
|------------|---------|
| SQLite3 devel libs | SQLite3 database backend |
| MariaDB devel libs | MariaDB/MySQL database backend |
| PostgreSQL devel libs | PostgreSQL database backend |

---

## Building

### Quick Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Platform-specific Build Instructions

- **[Building on RHEL-Based Distros](docs/build/BUILD-RHEL.md)**
- **[Building on Fedora](docs/build/BUILD-FEDORA.md)**
- **[Building on Ubuntu 20.04/22.04/24.04/26.04 LTS](docs/build/BUILD-UBUNTU.md)**
- **[Building for Win32/64 via MSYS](docs/build/BUILD-WIN32.md)**

---

## Compatibility

This library has been tested on:

| Platform | Status |
|----------|--------|
| Fedora Linux 35+ | Tested |
| Ubuntu 20.04/22.04/24.04/26.04 | Tested |
| RHEL/CentOS 7/8/9 | Tested |
| Windows (MSYS2/MinGW64) | Supported |

---

## C++ Standard

This library uses **C++17** as its compilation standard. This decision is intentional and strategic:

- **Not older standards (C++14/C++11 or earlier):** We do not support legacy distributions that are no longer maintained, vulnerable, or without official security updates.
- **Not newer standards (C++20/C++23 or later):** We prioritize compatibility over bleeding-edge features to ensure broad support across stable production environments.
- **The goal:** Programs built with libMantids30 should compile and run on the widest possible range of currently supported, stable Linux distributions — the majority of the production Linux ecosystem.

C++17 strikes the right balance: it is fully supported by all currently maintained Linux distributions (RHEL 7+, Ubuntu 20.04+, Fedora, Debian 12+, etc.) while providing modern language features like structured bindings, `std::optional`, `std::variant`, class template arguments deduction, and improved filesystem support.

---

## Installation

### COPR Repository (Fedora/RHEL)

[![Copr build status](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids30/)

#### Fedora/RHEL 8+:
```bash
# Install EPEL Repo
dnf -y install 'dnf-command(config-manager)'
dnf config-manager --set-enabled powertools
dnf -y install epel-release

# Enable unmanarc's COPR repository
dnf copr enable amizrachi/unmanarc -y

# Install libMantids30 and dependencies
dnf -y install libMantids30-devel jsoncpp-devel boost-devel boost-static \
               openssl-devel sqlite-devel mariadb-devel postgresql-devel \
               gcc-c++ cmake
```

#### RHEL 7:
```bash
# Install EPEL Repo + COPR
yum -y install epel-release
yum -y install yum-plugin-copr

# Enable unmanarc's COPR repository
yum copr enable amizrachi/unmanarc -y

# Install libMantids30 and dependencies
yum -y install libMantids30-devel jsoncpp-devel boost-devel boost-static \
               openssl-devel sqlite-devel mariadb-devel postgresql-devel \
               gcc-c++ cmake3
```

---

## Real-World Examples

Real-world projects using libMantids30:

- **[uFastAuthD3](https://github.com/unmanarc/uFastAuthD3)** - Identity and Access Management daemon

---

## License

This library is licensed under **LGPLv3**, except for:
- `Net_Interfaces` on WIN32, which is licensed under **GPLv2** due to `tap-windows.h`

See [LICENSE](LICENSE) for details.

---

## Author

**Aaron Mizrachi** (unmanarc)
- Email: dev@unmanarc.com
- GitHub: [unmanarc](https://github.com/unmanarc)