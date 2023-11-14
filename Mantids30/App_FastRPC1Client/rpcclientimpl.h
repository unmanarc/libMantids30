#pragma once

#include <Mantids30/Protocol_FastRPC1/fastrpc.h>
#include <Mantids30/Net_Sockets/socket_tls.h>

#include <atomic>
#include <string>

namespace Mantids30 { namespace Applications { namespace FastRPC1 {

class RPCClientImpl
{
public:
    struct PSKIdKey {
        std::string id;
        std::string psk;
    };

    RPCClientImpl();
    virtual ~RPCClientImpl();

    /**
     * @brief runRPClient0 Run the class RPC Client (to be called from a thread)
     * @param rpcImpl RPC Client Implementation object
     */
    static void runRPClient0(RPCClientImpl * rpcImpl);
    /**
     * @brief runRPClient run RPC Client and made connection/authentication/processing (will block indefinitely)
     */
    void runRPClient();
    /**
     * @brief retrieveConfigFromLocalFile Retrieve the jRetrievedConfig from the local file.
     * @return true if can retrieve, otherwise false.
     */
    bool retrieveConfigFromLocalFile();
    /**
     * @brief retrieveConfigFromC2 Retrieve config from the C2
     * @return true if retrieved, otherwise false
     */
    bool retrieveConfigFromC2();

    // TODO: thread-safety: if you are accessing this object during a thread from the rpc client,
    // you should not be able to modify it, and during the initialization, both components are running.
    json getJRetrievedConfig();


    PSKIdKey loadPSK();

    /**
     * @brief defaultPSK Virtual Function to be called when PSK is not defined.
     * @return pair with identity and default PSK
     */
    virtual PSKIdKey defaultPSK();


protected:

    /**
     * @brief connectedToC2AfterFailingToLoadC2Config This function is called back when the first connection to the C2 was not successful.
     * and the new connection was made after it. And because we can't load the C2 configuration here, not execute nothing here,
     * we recommend you to _exit(-1111); the program.
     */
    virtual void connectedToC2AfterFailingToLoadC2Config() = 0;
    /**
     * @brief addMethods This function is called back once to add the RPC client methods to be available to the C2.
     */
    virtual void addMethods() = 0;
    /**
     * @brief postConnect This function is called back after the connection is made and before any authentication
     * @param sockRPCClient TLS Socket
     * @return if false, the connection will not continue
     */
    virtual bool postConnect(Mantids30::Network::Sockets::Socket_TLS * sockRPCClient) { return  true; }

    virtual std::string decryptStr(const std::string & src) { return src; };
    virtual std::string encryptStr(const std::string & src) { return src; };

    std::string getClientConfigCmd,updateClientConfigLoadTimeCmd;

    Mantids30::Network::Protocols::FastRPC::FastRPC1 fastRPC;
    json jRetrievedConfig;
    std::atomic<bool> failedToRetrieveC2Config;

};
}}}


