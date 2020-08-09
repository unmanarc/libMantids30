TEMPLATE = subdirs

# Sockets LIB.
SUBDIRS = libcx2_net_sockets
# Project folders:
libcx2_net_sockets.subdir      = libcx2_net_sockets

# Multiplexer LIB.
SUBDIRS+=libcx2_net_multiplexer
# Project folders:
libcx2_net_multiplexer.subdir    = libcx2_net_multiplexer
libcx2_net_multiplexer.depends   = libcx2_net_sockets

# Network Interface configuration
SUBDIRS+=libcx2_net_interfaces
# Project folders:
libcx2_net_interfaces.subdir    = libcx2_net_interfaces

# Chain Sockets LIB.
SUBDIRS+=libcx2_net_chains
# Project folders:
libcx2_net_chains.subdir    = libcx2_net_chains
libcx2_net_chains.depends   = libcx2_net_sockets

#END-
