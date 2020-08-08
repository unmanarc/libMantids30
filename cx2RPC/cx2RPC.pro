TEMPLATE = subdirs

# JSON Async RPC stream (TCP/Unix Sockets) client
SUBDIRS+=libcx2_xrpc_common
# Project folders:
libcx2_xrpc_common.subdir    = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) server
#SUBDIRS+=libcx2_xrpc_server
# Project folders:
#libcx2_xrpc_server.subdir    = libcx2_xrpc_server
#libcx2_xrpc_server.depends   = libcx2_xrpc_common

# JSON Async WEB RPC server
SUBDIRS+=libcx2_xrpc_webserver
# Project folders:
libcx2_xrpc_webserver.subdir    = libcx2_xrpc_webserver
libcx2_xrpc_webserver.depends   = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) client
#SUBDIRS+=libcx2_xrpc_client
# Project folders:
#libcx2_xrpc_client.subdir    = libcx2_xrpc_client
#libcx2_xrpc_client.depends   = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) templace functions (auth,etc)
SUBDIRS+=libcx2_xrpc_templates
# Project folders:
libcx2_xrpc_templates.subdir    = libcx2_xrpc_templates
libcx2_xrpc_templates.depends   = libcx2_xrpc_common

# JSON Sync-Parallelized RPC
SUBDIRS+=libcx2_xrpc_fast
# Project folders:
libcx2_xrpc_fast.subdir    = libcx2_xrpc_fast
libcx2_xrpc_fast.depends   = libcx2_xrpc_common

#END-
# TODO:
# - common user management gui
# - common local user management gui (2fa, pass, etc)
