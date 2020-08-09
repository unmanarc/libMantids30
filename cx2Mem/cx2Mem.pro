TEMPLATE = subdirs

# Vars:
SUBDIRS += libcx2_mem_vars
# Project folders:
libcx2_mem_vars.subdir    = libcx2_mem_vars
libcx2_mem_vars.depends   =

# Stream Encoders:
SUBDIRS += libcx2_mem_streamencoders
# Project folders:
libcx2_mem_streamencoders.subdir    = libcx2_mem_streamencoders
libcx2_mem_streamencoders.depends   = libcx2_mem_vars


#END-
