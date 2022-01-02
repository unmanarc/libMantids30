%define name libMantids
%define version %(cat VERSION)
%define build_timestamp %{lua: print(os.date("%Y%m%d"))}

Name:           %{name}
Version:        %{version}
Release:        %{build_timestamp}.git%{?dist}
Summary:        Mini-Advanced C++11 Network Toolkit for Internet Services Development
License:        LGPLv3
URL:            https://github.com/unmanarc/libMantids
Source0:        https://github.com/unmanarc/libMantids/archive/master.tar.gz#/%{name}-%{version}-%{build_timestamp}.tar.gz
Group:          Development/Libraries

%define cmake cmake

%if 0%{?rhel} == 6
%define cmake cmake3
%endif

%if 0%{?rhel} == 7
%define cmake cmake3
%endif

%if 0%{?rhel} == 8
%define debug_package %{nil}
%endif

%if 0%{?rhel} == 9
%define debug_package %{nil}
%endif

%if 0%{?fedora} >= 33
%define debug_package %{nil}
%endif


%if 0%{?rhel} == 6
BuildRequires:  %{cmake} jsoncpp-devel boost-devel boost-static openssl-devel sqlite-devel mysql-devel postgresql-devel gcc-c++
%else
BuildRequires:  %{cmake} jsoncpp-devel boost-devel boost-static openssl-devel sqlite-devel mariadb-devel postgresql-devel gcc-c++
%endif

Requires:       jsoncpp boost-regex boost-thread openssl

%description
This package contains a enhancing C++11 framework libraries for services and network based projects

%package sqlite
Summary:        C++11 Framework Libraries v2 SQLite Extensions
Group:          Development/Libraries
Provides:       %{name}-sqlite
Requires:       %{name} sqlite

%description sqlite
This package contains the SQLite3 extensions for libMantids

%package postgresql
Summary:        C++11 Framework Libraries v2 PostgreSQL Extensions
Group:          Development/Libraries
Provides:       %{name}-postgresql
Requires:       %{name} postgresql-devel postgresql-libs

%description postgresql
This package contains the PostgreSQL extensions for libMantids

%package mariadb
Summary:        C++11 Framework Libraries v2 MariaDB Extensions
Group:          Development/Libraries
Provides:       %{name}-mariadb
Requires:       %{name} mariadb-devel mariadb-libs

%description mariadb
This package contains the MariaDB extensions for libMantids

%package devel
Summary:        C++11 Framework Libraries v2 development files
Group:          Development/Libraries
Provides:       %{name}-devel
Requires:       %{name}

%description devel
This package contains necessary header files for %{name} development.

%prep
%autosetup -n %{name}-master

%build
%{cmake} -DCMAKE_INSTALL_PREFIX:PATH=/usr -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel
%{cmake} -DCMAKE_INSTALL_PREFIX:PATH=/usr -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel
make %{?_smp_mflags}

%clean
rm -rf $RPM_BUILD_ROOT

%install
rm -rf $RPM_BUILD_ROOT

%if 0%{?fedora} >= 33
ln -s . %{_host}
%endif

%if 0%{?rhel} >= 9
ln -s . %{_host}
%endif

%if 0%{?fedora} == 35
ln -s . redhat-linux-build
%endif

%if "%{_host}" == "powerpc64le-redhat-linux-gnu"
ln -s . ppc64le-redhat-linux-gnu
%endif

%if "%{_host}" == "s390x-ibm-linux-gnu"
ln -s . s390x-redhat-linux-gnu
%endif

%if "%{cmake}" == "cmake3"
%cmake3_install
%else
%cmake_install
%endif

%files
%doc
%{_libdir}/libmdz_*
%exclude %{_libdir}/libmdz_db_sqlite*
%exclude %{_libdir}/libmdz_db_pgsql*
%exclude %{_libdir}/libmdz_db_mariadb*

%files devel
/usr/include/mdz*
/usr/share/pkgconfig/mdz*

%files sqlite
%{_libdir}/libmdz_db_sqlite*

%files postgresql
%{_libdir}/libmdz_db_pgsql*

%files mariadb
%{_libdir}/libmdz_db_mariadb*

%changelog
