#ifndef LOGINRPCCLIENTCBM_H
#define LOGINRPCCLIENTCBM_H

#include "loginrpcclient.h"
#include <Mantids29/Net_Sockets/callbacks_socket_tls_client.h>


namespace Mantids29 { namespace Authentication {


/**
 * @brief The LoginRPCClientCBM class Login RPC Client in callback mode...
 */
class LoginRPCClientCBM : public LoginRPCClient, public Network::Sockets::Callbacks_Socket_TLS_Client
{
public:
    LoginRPCClientCBM(void * obj);
    /**
     * @brief onAuthenticationFailure Callback to Notify when there is an error during the mutual API KEY exchange/authentication
     */
    void (*onAPIAuthenticationFailure)(void * obj, Mantids29::Network::Sockets::Socket_TLS * );
    /**
     * @brief onAPIAuthenticated Callback to Notify when the TLS/TCP-IP connection is established and the application is authenticated
     */
    void (*onAPIAuthenticated)(void * obj, Mantids29::Network::Sockets::Socket_TLS * );

    // TODOv3: rename this virtuals like
    // Virtual calls:
    /**
     * @brief notifyTLSConnecting Notify just before the TLS/TCP-IP Connection
     */
    void notifyTLSConnecting(Mantids29::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &);
    /**
     * @brief notifyTLSDisconnected Notify just after the TLS/TCP-IP Connection (with the error code as integer)
     */
    void notifyTLSDisconnected(Mantids29::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &, int);
    /**
     * @brief notifyAPIProcessingOK Notify when the TLS/TCP-IP connection is established and the application is authenticated
     */
    void notifyAPIProcessingOK(Mantids29::Network::Sockets::Socket_TLS * );
    /**
     * @brief notifyTLSConnectedOK Notify when the TLS/TCP-IP connection is established and we are about to authenticate
     */
    void notifyTLSConnectedOK(Mantids29::Network::Sockets::Socket_TLS * );
    /**
     * @brief notifyBadApiKey Notify when there is an error during the mutual API KEY exchange/authentication
     */
    void notifyBadApiKey(Mantids29::Network::Sockets::Socket_TLS * );
    /**
     * @brief notifyTLSErrorConnecting Notify when there is an error during the TLS/TCP-IP Connection
     */
    void notifyTLSErrorConnecting(Mantids29::Network::Sockets::Socket_TLS *, const std::string &, const uint16_t & );

private:
    void * obj;


};

}}

#endif // LOGINRPCCLIENTCBM_H
