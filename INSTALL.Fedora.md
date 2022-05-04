# libMantids Install Guide for Fedora 3x

Simple instructions for building this library in Fedora

***

## Installing dependencies

### Install epel (required for cmake):
*This is required to install cmake (only for build)*
```
dnf -y install epel-release
```

### Install build essentials (gcc, g++, etc) and cmake:

*These are the compiler and other build tools*

```
dnf -y groupinstall "Development Tools"
dnf -y install cmake
```

### Install required devel libraries:

*These are the required (mandatory) libraries*

```
dnf -y install openssl-devel jsoncpp-devel boost-devel
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```
dnf -y install sqlite-devel postgresql-devel mysql-devel
```

## Building Static+Dynamic Libs (in /opt):

This is the default build for the library:

```
PREFIXPATH=/opt/osslibs

prjdir=$(pwd)
cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -B~/builds/libMantids-Build-Shared
cd ~/builds/libMantids-Build-Shared
make clean
make -j12 install
cd "$prjdir"
cmake . -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -B~/builds/libMantids-Build-Static
cd ~/builds/libMantids-Build-Static
make clean
make -j12 install
cd "$prjdir"
```


***
## Dependencies when copying binaries

You can build this in one system, and you only need install the following runtimes:

```
dnf -y install jsoncpp openssl boost
```

If you need database support:

```
dnf -y install sqlite mysql postgresql
```




