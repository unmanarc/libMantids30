#ifndef RPCCLIENTIMPL_H
#define RPCCLIENTIMPL_H

#include <cx2_xrpc_fast/fastrpc.h>
#include <cx2_net_sockets/socket_tls.h>

namespace CX2 { namespace RPC {

class RPCClientImpl
{
public:
    RPCClientImpl();
    virtual ~RPCClientImpl();

    static void runRPClient0(RPCClientImpl * rpcImpl);
    void runRPClient();

    bool retrieveConfigFromLocalFile();
    bool retrieveConfigFromC2();

    json getJRetrievedConfig();

protected:
    virtual void addMethods() = 0;
    virtual bool postConnect(CX2::Network::TLS::Socket_TLS * sockRPCClient) { return  true; }

    virtual std::string decryptStr(const std::string & src) { return src; };
    virtual std::string encryptStr(const std::string & src) { return src; };

    std::string getClientConfigCmd,updateClientConfigLoadTimeCmd;

    int secsBetweenConnections;
    CX2::RPC::Fast::FastRPC fastRPC;
    json jRetrievedConfig;

};
}}


#endif // RPCCLIENTIMPL_H
