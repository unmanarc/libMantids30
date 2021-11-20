# cxFramework2 

C++11 Based Libraries  
  
Author: Aaron Mizrachi (unmanarc) <aaron@unmanarc.com>   
Main License: LGPLv3   
WIN32 License for cx2_net_interfaces: GPLv2 (tap-windows.h is GPLv2)  


***
## Functionality

This framework provides C++11 based enhancing libraries for console and network based proyects.

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

***
## Building cxFramework2

For each system, you need to have already installed the pre-requisites

### Instructions (CentOS 7.x):

[Building Instructions for CentOS-7 / RHEL7](INSTALL.CentOS7.md)

### Instructions (Fedora):

```
cmake . -DBUILD_SHARED_LIBS=ON
make -j12
make install
```

### Instructions (Ubuntu):

Given that ubuntu have another library paths, it requires aditional steps:

```
cd /usr/include
ln -s mariadb mysql
ln -s /usr/lib/x86_64-linux-gnu/libboost_regex.so /usr/lib/x86_64-linux-gnu/libboost_regex-mt.so
ln -s /usr/lib/x86_64-linux-gnu/libboost_thread.so /usr/lib/x86_64-linux-gnu/libboost_thread-mt.so
```

then, continue with the compilation:
```
cmake . -DBUILD_SHARED_LIBS=ON -B../cxFramework2-BuildLinux
cd  ../cxFramework2-BuildLinux
make -j12
make install
```
### Instructions (Win32 - Host: Fedora):

installing everything in /winpath:

```
/usr/bin/mingw32-cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=/winpath -DCMAKE_INSTALL_LIBDIR=lib -B../cxFramework2-BuildWin32
cd ../cxFramework2-BuildWin32
make -j12
make install
```


### Instructions (Win32 - Host: Win):

You can use MSYS2... First install the required mingw64 libraries and mingw64 compiler using pacman (boost devel, sqlite3 devel, mysql devel, openssl devel, postgresql devel, etc...), then open MSYS and build the project using this CMAKE options:

```
cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=/home/user/ROOT -DCMAKE_INSTALL_LIBDIR=lib -B../cxFramework2-BuildWin32 -DCMAKE_C_COMPILER=/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=/mingw64/bin/g++.exe -G "MinGW Makefiles"
cd ../cxFramework2-BuildWin32
mingw32-make.exe -j12 install
```




