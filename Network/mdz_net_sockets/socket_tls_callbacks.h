#ifndef SOCKET_TLS_CALLBACKS_H
#define SOCKET_TLS_CALLBACKS_H

#include "socket_tls.h"

class Socket_TLS_Callbacks
{
public:
    Socket_TLS_Callbacks(void * obj);

    void (*CB_TLS_KEY_InvalidCA)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string & );
    void (*CB_TLS_KEY_InvalidCRT)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string & );
    void (*CB_TLS_KEY_InvalidKEY)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string & );

    void * obj;
};

#endif // SOCKET_TLS_CALLBACKS_H
