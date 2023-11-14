# libMantids30 Install Guide for RHEL Based Distributions...

Simple instructions for building this library in RHEL Based Distributions

***

## Installing dependencies

### Install epel (required for cmake3):
*This is required to install cmake3 (only for build)*
```bash
yum -y install epel-release
```

### Install build essentials (gcc, g++, etc) and cmake3:

*These are the compiler and other build tools*

```bash
yum -y groupinstall "Development Tools"
# for rhel 7.x use cmake3 instead of cmake:
yum -y install cmake
```

### Install required devel libraries:

*These are the required (mandatory) libraries*

```bash
yum -y install openssl-devel jsoncpp-devel
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```bash
yum -y install sqlite-devel postgresql-devel mysql-devel
```

## Building Static+Dynamic Libs (in /opt):

This is the default build for the library:

```bash
PREFIXPATH=/opt/osslibs

prjdir=$(pwd)
cmake3 . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/libMantids30-Build-Shared
cd ~/builds/libMantids30-Build-Shared
make clean
make -j12 install
cd "$prjdir"
cmake3 . -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/libMantids30-Build-Static
cd ~/builds/libMantids30-Build-Static
make clean
make -j12 install
cd "$prjdir"
```


***
## Dependencies when copying binaries

You can build this in one system, and you only need install the following runtimes:

```bash
yum -y install jsoncpp openssl 
```

If you need database support:

```bash
yum -y install sqlite mysql postgresql
```
