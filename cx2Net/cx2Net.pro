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

# TCP Server LIB. 2
SUBDIRS+=libcx2_net_threadedacceptor
# Project folders:
libcx2_net_threadedacceptor.subdir    = libcx2_net_threadedacceptor
libcx2_net_threadedacceptor.depends   = libcx2_net_sockets

SUBDIRS+=libcx2_net_poolthreadedacceptor
# Project folders:
libcx2_net_poolthreadedacceptor.subdir    = libcx2_net_poolthreadedacceptor
libcx2_net_poolthreadedacceptor.depends   = libcx2_net_sockets

# Network Interface configuration
SUBDIRS+=libcx2_net_ifcfg
# Project folders:
libcx2_net_ifcfg.subdir    = libcx2_net_ifcfg

# Virtual Interfaces (TUN/TAP)
SUBDIRS+=libcx2_net_virtualif
# Project folders:
libcx2_net_virtualif.subdir    = libcx2_net_virtualif
libcx2_net_virtualif.depends   = libcx2_net_ifcfg

# TLS Server LIB.
SUBDIRS+=libcx2_net_tls
# Project folders:
libcx2_net_tls.subdir    = libcx2_net_tls
libcx2_net_tls.depends   = libcx2_net_sockets

# Chain Sockets LIB.
SUBDIRS+=libcx2_net_chains
# Project folders:
libcx2_net_chains.subdir    = libcx2_net_chains
libcx2_net_chains.depends   = libcx2_net_sockets libcx2_net_tls

# Chain TLS Sockets LIB.
SUBDIRS+=libcx2_net_chains_tls
# Project folders:
libcx2_net_chains_tls.subdir    = libcx2_net_chains_tls
libcx2_net_chains_tls.depends   = libcx2_net_sockets libcx2_net_tls libcx2_net_chains

#END-
