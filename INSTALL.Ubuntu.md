# libMantids Install Guide for Ubuntu 20.04/22.04 (RHEL7)

Simple instructions for building this library in Ubuntu 20.04/22.04

***

## Installing dependencies

Log as root and install the required programs/libraries...

### Install build essentials (gcc, g++, etc) and cmake:

*These are the compiler and other build tools*

```bash
apt update
apt install build-essential cmake pkg-config
```

### Install required devel libraries:

*These are the required (mandatory) libraries*

```bash
apt -y install libboost-all-dev libssl-dev libjsoncpp-dev
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```bash
apt -y install libsqlite3-dev libpq-dev libmariadb-dev
```

## Building Static+Dynamic Libs (in /opt):

This is the default build for the library:

```bash
PREFIXPATH=/opt/osslibs

prjdir=$(pwd)
cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -B~/builds/libMantids29-Build-Shared
cd ~/builds/libMantids29-Build-Shared
make clean
make -j12 install
cd "$prjdir"
cmake . -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -B~/builds/libMantids29-Build-Static
cd ~/builds/libMantids29-Build-Static
make clean
make -j12 install
cd "$prjdir"
```






