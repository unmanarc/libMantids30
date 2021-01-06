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



#END-
