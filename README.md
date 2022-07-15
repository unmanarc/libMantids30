# libMantids 

Mini-Advanced C++ Network Toolkit for Internet Services Development
  
Author: Aaron Mizrachi (unmanarc) <aaron@unmanarc.com>   
Main License: LGPLv3   
WIN32 License for mdz_net_interfaces: GPLv2 (tap-windows.h is GPLv2)  


***
## Functionality

This framework provides C++11 based enhancing libraries for console and network based projects

### Components:

* Program User Authentication
* Network Programming
  * Sockets (UDP/TCP/UNIX/TLS)
  * Protocols (HTTP/MIME/URL/LINE2LINE)
  * Chained Network Programming (Eg. TLS over TLS, TLS over AES-XOR)
  * Internal Fast RPC Protocol (Web, TCP/TLS)
* Database Abstraction Layer
  * PostgreSQL
  * MariaDB/MySQL
  * SQLite3
* Containers
  * Ultra-Fast allocation memory containers (eg. chunked)
  * File Mappers
* Threading
  * R/W Mutex
  * Thread-Safe Objects (Map/Queue)
  * Thread Pools
* Scripting
  * JSON Expressions evaluation
* File Formats
  * Vars File



***
## Installing packages (HOWTO)

- COPR (Fedora/CentOS/etc):  
[![Copr build status](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/libMantids/)



### Simple installation guide for Fedora/RHEL to develop software:

To activate our repo's and download/install the software:

In RHEL7:
```bash
# Install EPEL Repo + COPR
yum -y install epel-release
yum -y install yum-plugin-copr

# Install unmanarc's copr
yum copr enable amizrachi/unmanarc -y
# Install the required software:
yum -y install libMantids-devel jsoncpp-devel boost-devel boost-static openssl-devel sqlite-devel mariadb-devel postgresql-devel gcc-c++ cmake
```

In RHEL8:
```bash
# Install EPEL Repo
dnf -y install 'dnf-command(config-manager)'
dnf config-manager --set-enabled powertools
dnf -y install epel-release

# Install unmanarc's copr
dnf copr enable amizrachi/unmanarc -y
# Install the required software:
dnf -y install libMantids-devel jsoncpp-devel boost-devel boost-static openssl-devel sqlite-devel mariadb-devel postgresql-devel gcc-c++ cmake
```


***
## Compatibility

This library was tested so far in:

* OpenSUSE Tumbleweed
* Fedora Linux 35
* Ubuntu 18.04/20.04/22.04
* CentOS/RHEL 7/8/9
* CentOS/RHEL 5/6, but may require special/external C++11 compilers, we don't recommend it

### Overall Pre-requisites:

* C++11 Compatible Compiler (like GCC >=5)
* pthread
* openssl (1.1.x)
* jsoncpp
* boost

### Extras Pre-requisites:

* SQLite3 devel libs
* MariaDB devel libs
* PostgreSQL devel libs

### Win32 Pre-requisites:

* Using MSYS MinGW 64bit (recommended)
* Fedora MinGW (x86_64 or i686) compiler and required libs (deprecated)

***
## Building libMantids

Building instructions

### Instructions (CentOS 7.x):

[Building Instructions for CentOS-7 / RHEL7](INSTALL.CentOS7.md)

### Instructions (Fedora):

[Building Instructions for Fedora 3x](INSTALL.Fedora.md)

### Instructions (Ubuntu):

[Building Instructions for Ubuntu 20.04/22.04 LTS](INSTALL.Ubuntu.md)

### Instructions (Win32 - Host: Fedora):

[Building Instructions for Win32/64 via MSYS](INSTALL.Win32.md)

