#include "socket_tlsserver_callbacks.h"

Socket_TLSServer_Callbacks::Socket_TLSServer_Callbacks(void * obj) : Socket_TLS_Callbacks(obj)
{
    this->CB_TLS_ListeningOK=nullptr;
    this->CB_TLS_ListeningFAILED=nullptr;
    this->CB_TLS_ClientDisconnected=nullptr;
    this->CB_TLS_ClientConnected=nullptr;
    this->CB_TLS_ClientAuthError=nullptr;
    this->CB_TLS_ClientAcceptTimedOut=nullptr;
    this->CB_TLS_ClientAcceptReachedMaxConnectionsPerIP=nullptr;
}

