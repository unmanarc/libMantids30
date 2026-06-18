%define name libMantids30
%define version 3.0.0
%define build_timestamp %{lua: print(os.date("%Y%m%d"))}
Name:           %{name}
Version:        %{version}
Release:        %{build_timestamp}.el%{?rhel}.git%{?dist}
Summary:        Mini-Advanced C++17 Network Toolkit for Internet Services Development
License:        LGPLv3
URL:            https://github.com/unmanarc/libMantids30
Source0:        https://github.com/unmanarc/libMantids30/archive/master.tar.gz#/%{name}-%{version}-%{build_timestamp}.tar.gz
Group:          Development/Libraries
# --- DEPRECATE RHEL6: Minimum supported is RHEL 7 ---
# RHEL 6 support has been removed (EOL November 2024)
# Debug packages are automatically handled on RHEL 8+ and Fedora 33+
%if 0%{?rhel} == 7
%define debug_package %{nil}
%endif
%if 0%{?rhel} >= 8
%define debug_package %{nil}
%endif
%if 0%{?fedora} >= 33
%define debug_package %{nil}
%endif
# Build dependencies
%if 0%{?rhel} == 7
BuildRequires:  scl-utils-build
BuildRequires:  devtoolset-9-gcc
BuildRequires:  devtoolset-9-gcc-c++
BuildRequires:  cmake3
BuildRequires:  pkgconfig
%else
BuildRequires:  cmake >= 3.10
BuildRequires:  pkg-config
BuildRequires:  gcc-c++ >= 8
%endif
# Common build dependencies
BuildRequires:  jsoncpp-devel
BuildRequires:  boost-devel
BuildRequires:  boost-static
BuildRequires:  openssl-devel
BuildRequires:  sqlite-devel
BuildRequires:  postgresql-devel
BuildRequires:  mariadb-devel
# Runtime dependencies
Requires:       jsoncpp
Requires:       boost-regex
Requires:       boost-thread
Requires:       openssl
# RHEL 7 specific: may need openssl11 from EPEL if using SSLRHEL7
%if 0%{?rhel} == 7
BuildRequires:  openssl11-devel
Requires:       openssl11
%endif
%description
This package contains a C++17 framework library for services and network-based projects.
Minimum supported platform: RHEL/CentOS 7 (RHEL 6 is deprecated and no longer supported).
%package sqlite
Summary:        C++17 Framework Library SQLite Extensions
Group:          Development/Libraries
Provides:       %{name}-sqlite
Requires:       %{name}
Requires:       sqlite
%description sqlite
This package contains the SQLite3 extensions for libMantids.
%package postgresql
Summary:        C++17 Framework Library PostgreSQL Extensions
Group:          Development/Libraries
Provides:       %{name}-postgresql
Requires:       %{name}
Requires:       postgresql-devel
Requires:       postgresql-libs
%description postgresql
This package contains the PostgreSQL extensions for libMantids.
%package mariadb
Summary:        C++17 Framework Library MariaDB Extensions
Group:          Development/Libraries
Provides:       %{name}-mariadb
Requires:       %{name}
Requires:       mariadb-devel
Requires:       mariadb-libs
%description mariadb
This package contains the MariaDB extensions for libMantids.
%package devel
Summary:        C++17 Framework Library development files
Group:          Development/Libraries
Provides:       %{name}-devel
Requires:       %{name}
%description devel
This package contains necessary header files and pkg-config files for %{name} development.
%prep
%autosetup -n %{name}-master
%build
%if 0%{?rhel} == 7
export PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
export LD_LIBRARY_PATH=/opt/rh/devtoolset-9/root/usr/lib64:/opt/rh/devtoolset-9/root/usr/lib:$LD_LIBRARY_PATH
export CC=/opt/rh/devtoolset-9/root/usr/bin/gcc
export CXX=/opt/rh/devtoolset-9/root/usr/bin/g++
cmake3 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
       -DCMAKE_INSTALL_PREFIX:PATH=/usr \
       -DBUILD_SHARED_LIBS=ON \
       -DCMAKE_BUILD_TYPE=MinSizeRel \
       -DSSLRHEL7=ON .
make %{?_smp_mflags}
%else
cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel
make %{?_smp_mflags}
%endif
%clean
rm -rf $RPM_BUILD_ROOT
%install
rm -rf $RPM_BUILD_ROOT
# Symlink workarounds for certain architectures and distros
%if 0%{?fedora} >= 33
ln -s . %{_host}
%endif
%if 0%{?rhel} >= 9
ln -s . %{_host}
ln -s . redhat-linux-build
%endif
%if "%{_host}" == "powerpc64le-redhat-linux-gnu"
ln -s . ppc64le-redhat-linux-gnu
%endif
%if "%{_host}" == "s390x-ibm-linux-gnu"
ln -s . s390x-redhat-linux-gnu
%endif
%cmake_install
%files
%doc
%{_libdir}/libMantids30_*
%exclude %{_libdir}/libMantids30_DB_SQLite3*
%exclude %{_libdir}/libMantids30_DB_PostgreSQL*
%exclude %{_libdir}/libMantids30_DB_MariaDB*
%files devel
/usr/include/Mantids30*
/usr/share/pkgconfig/Mantids30*
%files sqlite
%{_libdir}/libMantids30_DB_SQLite3*
%files postgresql
%{_libdir}/libMantids30_DB_PostgreSQL*
%files mariadb
%{_libdir}/libMantids30_DB_MariaDB*
%changelog
* Thu Jun 18 2026 Aaron G. Mizrachi P. <dev@unmanarc.com> - 3.0.0
- Deprecate RHEL 6 support (EOL November 2024)
- Update C++ standard from C++11 to C++17 in spec metadata
- Simplify cmake configuration (removed cmake3 define for RHEL 6/7)
- Add minimum cmake version requirement (>= 3.10)
- Add pkg-config as build requirement
- Update summary to reflect C++17 standard
