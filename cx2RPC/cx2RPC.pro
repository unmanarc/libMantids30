TEMPLATE = subdirs

# JSON Async RPC stream (TCP/Unix Sockets) client
SUBDIRS+=libcx2_xrpc_common
# Project folders:
libcx2_xrpc_common.subdir    = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) server
SUBDIRS+=libcx2_xrpc_server
# Project folders:
libcx2_xrpc_server.subdir    = libcx2_xrpc_server
libcx2_xrpc_server.depends   = libcx2_xrpc_common

# JSON Async WEB RPC server
SUBDIRS+=libcx2_xrpcweb_server
# Project folders:
libcx2_xrpcweb_server.subdir    = libcx2_xrpcweb_server
libcx2_xrpcweb_server.depends   = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) client
SUBDIRS+=libcx2_xrpc_client
# Project folders:
libcx2_xrpc_client.subdir    = libcx2_xrpc_client
libcx2_xrpc_client.depends   = libcx2_xrpc_common

# JSON Async RPC stream (TCP/Unix Sockets) templace functions (auth,etc)
SUBDIRS+=libcx2_xrpc_templates
# Project folders:
libcx2_xrpc_templates.subdir    = libcx2_xrpc_templates
libcx2_xrpc_templates.depends   = libcx2_xrpc_common

#END-

# TODO:
# - qt client lib
# - user/auth manipulation functions
# - common user management gui
# - common local user management gui (2fa, pass, etc)

