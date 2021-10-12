# cxFramework2 

C++11 Based Libraries  
  
Author: Aaron Mizrachi (unmanarc) <aaron@unmanarc.com>   
Main License: LGPLv3   
WIN32 License for cx2_net_interfaces: GPLv2 (tap-windows.h is GPLv2)  


***
## Building cxFramework2 in Linux

### Instructions:

as root:

```
cmake . -DBUILD_SHARED_LIBS=ON
make -j12
make install
```

***
## Functionality

This framework provides C++11 based enhancing libraries for console and network based proyects.

### Components:

* Program User Authentication
* Database Abstraction Layer (PostgreSQL (w+l)/MariaDB (l)/SQLite3 (w+l))
* File Formats
* Network Programming
  * Sockets (UDP/TCP/UNIX/TLS)
  * Protocols (HTTP/MIME/URL/LINE2LINE)
  * Chained Network Programming (Eg. TLS over TLS, TLS over AES-XOR)
  * RPC (Web, TCP/TLS)
* Containers
  * Fast allocation memory containers (eg. chunked)
  * File Mappers
* Threading
* Scripting

***
## Compatibility

This library was tested so far in:

* Fedora Linux 32
* Ubuntu 18.04/20.04
* CentOS/RHEL 7/8
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

* Using MSYS MinGW 64bit
* Fedora MinGW (x86_64 or i686) compiler and required libs (deprecated)
