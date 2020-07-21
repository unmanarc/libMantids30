TEMPLATE = subdirs

# Streams
SUBDIRS += libcx2_mem_streams
# Project folders:
libcx2_mem_streams.subdir      = libcx2_mem_streams

# Stream Encoders:
SUBDIRS += libcx2_mem_streamencoders
# Project folders:
libcx2_mem_streamencoders.subdir    = libcx2_mem_streamencoders
libcx2_mem_streamencoders.depends   = libcx2_mem_streams

# Containers:
SUBDIRS += libcx2_mem_containers
# Project folders:
libcx2_mem_containers.subdir    = libcx2_mem_containers
libcx2_mem_containers.depends   = libcx2_mem_streams
libcx2_mem_containers.depends   = libcx2_mem_abstracts

# Vars:
SUBDIRS += libcx2_mem_vars
# Project folders:
libcx2_mem_vars.subdir    = libcx2_mem_vars
libcx2_mem_vars.depends   = libcx2_mem_containers libcx2_mem_abstracts

# Parser:
SUBDIRS += libcx2_mem_streamparser
# Project folders:
libcx2_mem_streamparser.subdir    = libcx2_mem_streamparser
libcx2_mem_streamparser.depends   = libcx2_mem_streams libcx2_mem_containers

SUBDIRS += libcx2_mem_abstracts
# Project folders:
libcx2_mem_abstracts.subdir      = libcx2_mem_abstracts

#END-
