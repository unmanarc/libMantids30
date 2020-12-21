TEMPLATE = subdirs

# SQLITE3 DB Support
SUBDIRS += libcx2_db_sqlite3
# Project folders:
libcx2_db_sqlite3.subdir    = libcx2_db_sqlite3

# MySQL DB Support
SUBDIRS += libcx2_db_mariadb
# Project folders:
libcx2_db_mariadb.subdir    = libcx2_db_mariadb

# PostgreSQL DB Support
SUBDIRS += libcx2_db_pgsql
# Project folders:
libcx2_db_pgsql.subdir    = libcx2_db_pgsql

#END-
