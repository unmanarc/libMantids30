#!/bin/bash

PREFIXPATH=/opt/osslibs

prjdir=$(pwd)
cmake3 . -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/cxFramework2-Build-Shared
cd ~/builds/cxFramework2-Build-Shared
make clean
make -j12 install
cd "$prjdir"
cmake3 . -DCMAKE_INSTALL_PREFIX:PATH=${PREFIXPATH} -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_LIBDIR=lib -B~/builds/cxFramework2-Build-Static
cd ~/builds/cxFramework2-Build-Static
make clean
make -j12 install
cd "$prjdir"


