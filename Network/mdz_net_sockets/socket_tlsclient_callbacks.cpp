#include "socket_tlsclient_callbacks.h"

Socket_TLSClient_Callbacks::Socket_TLSClient_Callbacks(void * obj) : Socket_TLS_Callbacks(obj)
{
    CB_TLS_Connecting = nullptr;
    CB_TLS_Disconnected = nullptr;
    CB_TLS_ConnectedOK = nullptr;
    CB_TLS_ErrorConnecting = nullptr;
}
