#include "socket_tls_callbacks.h"

Socket_TLS_Callbacks::Socket_TLS_Callbacks(void *obj)
{
    this->obj = obj;

    CB_TLS_KEY_InvalidCA = nullptr;
    CB_TLS_KEY_InvalidCRT = nullptr;
    CB_TLS_KEY_InvalidKEY = nullptr;
}
