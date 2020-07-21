# cxFramework2 

C++11 Based Libraries  
  
Author: Aaron Mizrachi (unmanarc) <aaron@unmanarc.com>   
License: LGPLv3   

## Functionality

This framework provides C++11 based enhancing libraries for console and network based proyects.

### Components:

* Program User Authentication
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


## Building cxFramework2

### Overall Pre-requisites:

* pthread
* openssl (1.1.x)
* sqlite3
* jsoncpp
* boost
* C++11 Compatible Compiler (like GCC >=5)

### Instructions:

as root:

```
qmake-qt5 . PREFIX=/usr
make -j8 install
```

`ATT: This project isn't related or using QT, We are just using QT Makefiles`

## Compatibility

We tested this libs so far in:

* Fedora Linux 32
* Ubuntu 18.04/20.04
* CentOS/RHEL 7/8

`CentOS/RHEL 5/6 may require special C++11 compilers.`

Other distros or OS's may also build without problems.

### Win32 build Notice:

This project should be win32/64 compatible, we suggest to use fedora mingw-w64.  
By now is only tested in Linux.
