TEMPLATE = subdirs

SUBDIRS+=libcx2_netp_urlvars
# Project folders:
libcx2_netp_urlvars.subdir    = libcx2_netp_urlvars
#libcx2_netp_urlvars.depends   =

SUBDIRS+=libcx2_netp_mime
# Project folders:
libcx2_netp_mime.subdir    = libcx2_netp_mime
#libcx2_netp_mime.depends   =

SUBDIRS+=libcx2_netp_http
# Project folders:
libcx2_netp_http.subdir    = libcx2_netp_http
libcx2_netp_http.depends   = libcx2_netp_mime libcx2_netp_urlvars

SUBDIRS+=libcx2_netp_linerecv
# Project folders:
libcx2_netp_linerecv.subdir    = libcx2_netp_linerecv
libcx2_netp_linerecv.depends   = 


#END-
