#ifndef LOGINRPCCLIENT_H
#define LOGINRPCCLIENT_H

#include <string>
#include "manager_remote.h"
#include <cx2_net_sockets/socket_tls.h>

namespace CX2 { namespace Authentication {

class LoginRPCClient
{
public:
    LoginRPCClient();

    virtual void notifyTLSConnecting(CX2::Network::TLS::Socket_TLS * , const std::string & , const uint16_t &) {}
    virtual void notifyTLSDisconnected(CX2::Network::TLS::Socket_TLS * , const std::string & , const uint16_t &, int) {}
    virtual void notifyAPIProcessingOK(CX2::Network::TLS::Socket_TLS * ) {}
    virtual void notifyTLSConnectedOK(CX2::Network::TLS::Socket_TLS * ) {}
    virtual void notifyBadApiKey(CX2::Network::TLS::Socket_TLS * ) {}
    virtual void notifyTLSErrorConnecting(CX2::Network::TLS::Socket_TLS *, const std::string &, const uint16_t & ) {}

    /**
     * @brief connect Internal Static function for processing the connections
     * @param rpcClient Login RPC Client Class to process
     */
    static void process(LoginRPCClient * rpcClient,u_int16_t sleepBetweenConnectionsSeconds);
    /**
     * @brief start start connection loop in background
     * NOTE: every set for this call should be done before start.
     */
    void start(u_int16_t sleepBetweenConnectionsSeconds=5);
    /**
     * @brief getRemoteHost Get Remote RPC Login Server Host
     * @return RPC Login Server Host
     */
    std::string getRemoteHost() const;
    /**
     * @brief setRemoteHost Set Remote RPC Login Server Host
     * @param value RPC Login Server Host (default: 127.0.0.1)
     */
    void setRemoteHost(const std::string &value);
    /**
     * @brief getUseIPv6 Get if the socket is using IPv6
     * @return true if using ipv6
     */
    bool getUseIPv6() const;
    /**
     * @brief setUseIPv6 Set to use or not IPv6 on the connection socket
     * @param value true to use IPv6, false for IPv4 only. (default: false)
     */
    void setUseIPv6(bool value);
    /**
     * @brief getRemotePort Get remote TCP/IP Port for the RPC Login Server
     * @return numeric TCP/IP Port for the RPC Login Server
     */
    uint16_t getRemotePort() const;
    /**
     * @brief setRemotePort Set remote TCP/IP Port for the RPC Login Server
     * @param value numeric TCP/IP Port for the RPC Login Server (default: 30301)
     */
    void setRemotePort(const uint16_t &value);
    /**
     * @brief getApiKey Get the Secret API Key for the RPC Login
     * @return Secret API Key String
     */
    std::string getApiKey() const;
    /**
     * @brief setApiKey Set the Secret API Key for the RPC Login
     * @param value Secret API Key String (default: none, required!)
     */
    void setApiKey(const std::string &value);
    /**
     * @brief getAppName Get Local APP Name
     * @return Local app Name
     */
    std::string getAppName() const;
    /**
     * @brief setAppName Set Local APP Name
     * @param value Local app Name (default: none, required!)
     */
    void setAppName(const std::string &value);
    /**
     * @brief getCertFile Get Certificate Public Key File Path
     * @return Certificate Public Key File Path
     */
    std::string getCertFile() const;
    /**
     * @brief setCertFile Set Certificate Public Key File Path
     * @param value Certificate Public Key Path (default: empty, optional)
     */
    void setCertFile(const std::string &value);
    /**
     * @brief getKeyFile Get Certificate Private Key File Path
     * @return  Certificate Private Key File Path
     */
    std::string getKeyFile() const;
    /**
     * @brief setKeyFile Set Certificate Private Key File Path
     * @param value Certificate Private Key Path (default: empty, optional)
     */
    void setKeyFile(const std::string &value);
    /**
     * @brief getCaFile Get Certificate Authority File Path
     * @return Certificate Authority File Path
     */
    std::string getCaFile() const;
    /**
     * @brief setCaFile Set Certificate Authority File Path
     * @param value Certificate Authority Path (default: ca.crt @CWD, please change!)
     */
    void setCaFile(const std::string &value);

    Manager_Remote * getRemoteAuthManager();

private:
    Manager_Remote remoteAuthManager;

    std::string remoteHost;
    bool useIPv6;
    uint16_t remotePort;

    std::string apiKey, appName, certFile, keyFile, caFile;

};
}}
#endif // LOGINRPCCLIENT_H
