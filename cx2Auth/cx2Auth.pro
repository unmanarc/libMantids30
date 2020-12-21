TEMPLATE = subdirs

# Base lib for authenticate/validate account-password
SUBDIRS += libcx2_auth
# Project folders:
libcx2_auth.subdir = libcx2_auth

# DB based authenticator (db based)
SUBDIRS += libcx2_auth_db
# Project folders:
libcx2_auth_db.subdir    = libcx2_auth_db
libcx2_auth_db.depends   = libcx2_auth

# Internal based authenticator (filesystem based)
SUBDIRS += libcx2_auth_fs
# Project folders:
libcx2_auth_fs.subdir    = libcx2_auth_fs
libcx2_auth_fs.depends   = libcx2_auth

# Memory based authenticator (memory based)
SUBDIRS += libcx2_auth_volatile
# Project folders:
libcx2_auth_volatile.subdir    = libcx2_auth_volatile
libcx2_auth_volatile.depends   = libcx2_auth

#END-
