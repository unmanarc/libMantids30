# libMantids Install Guide for Ubuntu 20.04/22.04 (RHEL7)

Simple instructions for building this library in Ubuntu 20.04/22.04

***

## Installing dependencies

Log as root and install the required programs/libraries...

### Install build essentials (gcc, g++, etc) and cmake:

*These are the compiler and other build tools*

```
apt update
apt install build-essential cmake
```

### Install required devel libraries:

*These are the required (mandatory) libraries*

```
apt -y install libboost-all-dev libssl-dev libjsoncpp-dev
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```
apt -y install libsqlite3-dev libpq-dev libmariadb-dev
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






