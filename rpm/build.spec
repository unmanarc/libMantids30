%define name cxFramework2
%define version 2.5.0
%define build_timestamp %{lua: print(os.date("%Y%m%d"))}

Name:           %{name}
Version:        %{version}
Release:        %{build_timestamp}.git
Summary:        C++11 Framework Libraries v2
License:        LGPLv3
URL:            https://github.com/unmanarc/cxFramework2
Source0:        https://github.com/unmanarc/cxFramework2/archive/master.tar.gz#/%{name}-%{version}-%{build_timestamp}.tar.gz
Group:          Development/Libraries

%define cmake cmake

%if 0%{?rhel} == 6
%define cmake cmake3
%endif

%if 0%{?rhel} == 7
%define cmake cmake3
%endif

BuildRequires:  %{cmake} jsoncpp-devel boost-devel boost-static openssl-devel sqlite-devel mariadb-devel postgresql-devel
Requires:       jsoncpp boost-regex boost-thread openssl

%description
This package contains a enhancing C++11 framework libraries for services and network based projects

%package sqlite
Summary:        C++11 Framework Libraries v2 SQLite Extensions
Group:          Development/Libraries
Provides:       %{name}-sqlite
Requires:       %{name} sqlite

%description sqlite
This package contains the SQLite3 extensions for cx2Framework

%package postgresql
Summary:        C++11 Framework Libraries v2 PostgreSQL Extensions
Group:          Development/Libraries
Provides:       %{name}-postgresql
Requires:       %{name} postgresql-devel postgresql-libs

%description postgresql
This package contains the PostgreSQL extensions for cx2Framework

%package mariadb
Summary:        C++11 Framework Libraries v2 MariaDB Extensions
Group:          Development/Libraries
Provides:       %{name}-mariadb
Requires:       %{name} mariadb-devel mariadb-libs

%description mariadb
This package contains the MariaDB extensions for cx2Framework

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

%if "%{cmake}" == "cmake3"
%cmake3_install
%else
%cmake_install
%endif

%files
%doc
/usr/lib64/libcx2_*
%exclude /usr/lib64/libcx2_db_sqlite*
%exclude /usr/lib64/libcx2_db_pgsql*
%exclude /usr/lib64/libcx2_db_mariadb*

%files devel
/usr/include/cx2*
/usr/share/pkgconfig/cx2*

%files sqlite
/usr/lib64/libcx2_db_sqlite*

%files postgresql
/usr/lib64/libcx2_db_pgsql*

%files mariadb
/usr/lib64/libcx2_db_mariadb*

%changelog
