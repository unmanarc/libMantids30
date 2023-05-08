#pragma once

#include "manager_remote.h"

#include <string>
#include <stdint.h>

#include <Mantids29/Net_Sockets/socket_tls.h>

namespace Mantids29 { namespace Authentication {

class LoginRPCClient
{
public:
    LoginRPCClient();

    /**
     * @brief notifyTLSConnecting Notify just before the TLS/TCP-IP Connection
     */
    virtual void notifyTLSConnecting(Mantids29::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &) {}
    /**
     * @brief notifyTLSDisconnected Notify just after the TLS/TCP-IP Connection (with the error code as integer)
     */
    virtual void notifyTLSDisconnected(Mantids29::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &, int) {}
    /**
     * @brief notifyAPIProcessingOK Notify when the TLS/TCP-IP connection is established and the application is authenticated
     */
    virtual void notifyAPIProcessingOK(Mantids29::Network::Sockets::Socket_TLS * ) {}
    /**
     * @brief notifyTLSConnectedOK Notify when the TLS/TCP-IP connection is established and we are about to authenticate
     */
    virtual void notifyTLSConnectedOK(Mantids29::Network::Sockets::Socket_TLS * ) {}
    /**
     * @brief notifyBadApiKey Notify when there is an error during the mutual API KEY exchange/authentication
     */
    virtual void notifyBadApiKey(Mantids29::Network::Sockets::Socket_TLS * ) {}
    /**
     * @brief notifyTLSErrorConnecting Notify when there is an error during the TLS/TCP-IP Connection
     */
    virtual void notifyTLSErrorConnecting(Mantids29::Network::Sockets::Socket_TLS *, const std::string &, const uint16_t & ) {}

    ///////////////////////////////////////////////////////
    /**
     * @brief connect Internal Static function for processing the connections
     * @param rpcClient Login RPC Client Class to process
     */
    static void process(LoginRPCClient * rpcClient,uint16_t sleepBetweenConnectionsSeconds);

    /**
     * @brief start start connection loop in background
     * NOTE: every set for this call should be done before start.
     */
    void start(uint16_t sleepBetweenConnectionsSeconds=5);

    /**
     * @brief Returns the Remote Authentication Manager object
     *
     * This method returns a pointer to the Remote Authentication Manager object
     *
     * @return A pointer to the Remote Authentication Manager object.
     */
    Manager_Remote *getRemoteAuthManager();


public:
    /**
    * @brief m_remoteHost Remote hostname or IP address.
    */
    std::string m_remoteHost;

    /**
     * @brief m_remotePort Remote port number.
     */
    uint16_t m_remotePort;

    /**
     * @brief m_useIPv6 Use IPv6 instead of IPv4.
     */
    bool m_useIPv6;

    /**
     * @brief m_usingTLSPSK Whether to use TLS with pre-shared keys.
     */
    bool m_usePreSharedKeyForTLS;

    /**
     * @brief m_apiKey API key used for authentication.
     */
    std::string m_apiKey;

    /**
     * @brief m_appName Name of the application making the connection.
     */
    std::string m_applicationName;

    /**
     * @brief m_certFile Path to certificate file for TLS.
     */
    std::string m_certificateFilePath;

    /**
     * @brief m_keyFile Path to private key file for TLS.
     */
    std::string m_keyFilePath;

    /**
     * @brief m_CertificateAuthorityFilePath Path to certificate authority (CA) file for TLS.
     */
    std::string m_CertificateAuthorityFilePath;

private:
    Manager_Remote remoteAuthManager;


};
}}
