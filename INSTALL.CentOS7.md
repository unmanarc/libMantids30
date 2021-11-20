# cxFramework2 Install Guide for CentOS 7.x (RHEL7)

Simple instructions for building this library in CentOS 7.x

***

## Installing dependencies

### Install epel (required for cmake3):
*This is required to install cmake3 (only for build)*
```
yum -y install epel-release
```



### Install build essentials (gcc, g++, etc) and cmake3:

*These are the compiler and other build tools*

```
yum -y groupinstall "Development Tools"
yum -y install cmake3
```

### Install required devel libraries:

*These are the required (mandatory) libraries*

```
yum -y install openssl-devel jsoncpp-devel
```

### Install Optional devel libraries:

*These are the optional libraries for database compatibility*

```
yum -y install sqlite-devel postgresql-devel mysql-devel
```

## Building Static+Dynamic Libs (in /opt):

This is the default build for the library:

```
PREFIXPATH=/opt/osslibs

prjdir=$(pwd)
cmake3 . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/cxFramework2-Build-Shared
cd ~/builds/cxFramework2-Build-Shared
make clean
make -j12 install
cd "$prjdir"
cmake3 . -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/cxFramework2-Build-Static
cd ~/builds/cxFramework2-Build-Static
make clean
make -j12 install
cd "$prjdir"
```


***
## Dependencies when copying binaries

You can build this in one system, and you only need install the following runtimes:

```
yum -y install jsoncpp openssl 
```

If you need database support:

```
yum -y install sqlite mysql postgresql
```




