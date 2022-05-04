# libMantids Install Guide for Win32/64 via MSYS

Simple instructions for building this library in Win32/64 via MSYS with g++

***

## Installing dependencies

Log into MSYS console and and  install the required mingw64 libraries and mingw64 compiler using __pacman__

### Install build essentials (gcc, g++, etc) and cmake:

Log into MSYS console and and  install the required mingw64 libraries and mingw64 compiler using __pacman__
Please install all the building environment with g++/gcc/cmake (mingw64)

### Install required devel libraries:

*These are the required (mandatory) libraries*:

```
boost development
openssl development
jsoncpp development
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```
sqlite3 development
postgresql development
mariadb development
```

## Building Static+Dynamic Libs (in /opt):

You can build the application using this command:

```
cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=/home/user/ROOT -DCMAKE_INSTALL_LIBDIR=lib -B../libMantids-BuildWin32 -DCMAKE_C_COMPILER=/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=/mingw64/bin/g++.exe -G "MinGW Makefiles"
cd ../libMantids-BuildWin32
mingw32-make.exe -j12 install
```


Note: please replace `/home/user/ROOT` prefix installation for your own target dir.


