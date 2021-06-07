#include "loginrpcclient.h"
#include <cx2_net_sockets/cryptostream.h>

#include <thread>
using namespace CX2::Authentication;

LoginRPCClient::LoginRPCClient()
{
    remoteHost="127.0.0.1";
    remotePort=30301;
    useIPv6=false;

  //  certFile = "snakeoil-cli.crt";
  //  keyFile = "snakeoil-cli.key";
    caFile = "ca.crt";
}

void LoginRPCClient::process(LoginRPCClient *rpcClient,u_int16_t sleepBetweenConnectionsSeconds)
{
    for (;;)
    {
        CX2::Network::TLS::Socket_TLS tlsClient;

        tlsClient.setTLSCertificateAuthorityPath(rpcClient->getCaFile().c_str());

        if (!rpcClient->getCertFile().empty())
            tlsClient.setTLSPublicKeyPath(rpcClient->getCertFile().c_str());
        if (!rpcClient->getKeyFile().empty())
            tlsClient.setTLSPrivateKeyPath(rpcClient->getKeyFile().c_str());

        // Connecting...
        rpcClient->notifyTLSConnecting(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort());

        if (tlsClient.connectTo( rpcClient->getRemoteHost().c_str(),rpcClient->getRemotePort() ))
        {
            // TLS connected
            rpcClient->notifyTLSConnectedOK(&tlsClient);
            Network::Streams::CryptoStream cstream(&tlsClient);
            tlsClient.writeString16(rpcClient->getAppName());
            if (cstream.mutualChallengeResponseSHA256Auth(rpcClient->getApiKey(),false) == std::make_pair(true,true))
            {
                // Authentication OK...
                rpcClient->notifyAPIProcessingOK(&tlsClient);
                int code = rpcClient->getRemoteAuthManager()->processFastRPCConnection(&tlsClient);
                rpcClient->notifyTLSDisconnected(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort(), code);
            }
            else
            {
                rpcClient->notifyBadApiKey(&tlsClient);
                // Bad API Key...
            }
        }
        else
        {
            // Error connecting..
            rpcClient->notifyTLSErrorConnecting(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort());
        }
        sleep(sleepBetweenConnectionsSeconds);
    }
}

void LoginRPCClient::start(u_int16_t sleepBetweenConnectionsSeconds)
{
    auto i = std::thread(process, this, sleepBetweenConnectionsSeconds);
    i.detach();
}

std::string LoginRPCClient::getRemoteHost() const
{
    return remoteHost;
}

void LoginRPCClient::setRemoteHost(const std::string &value)
{
    remoteHost = value;
}

bool LoginRPCClient::getUseIPv6() const
{
    return useIPv6;
}

void LoginRPCClient::setUseIPv6(bool value)
{
    useIPv6 = value;
}

uint16_t LoginRPCClient::getRemotePort() const
{
    return remotePort;
}

void LoginRPCClient::setRemotePort(const uint16_t &value)
{
    remotePort = value;
}

std::string LoginRPCClient::getApiKey() const
{
    return apiKey;
}

void LoginRPCClient::setApiKey(const std::string &value)
{
    apiKey = value;
}

std::string LoginRPCClient::getAppName() const
{
    return appName;
}

void LoginRPCClient::setAppName(const std::string &value)
{
    appName = value;
}

std::string LoginRPCClient::getCertFile() const
{
    return certFile;
}

void LoginRPCClient::setCertFile(const std::string &value)
{
    certFile = value;
}

std::string LoginRPCClient::getKeyFile() const
{
    return keyFile;
}

void LoginRPCClient::setKeyFile(const std::string &value)
{
    keyFile = value;
}

std::string LoginRPCClient::getCaFile() const
{
    return caFile;
}

void LoginRPCClient::setCaFile(const std::string &value)
{
    caFile = value;
}

Manager_Remote *LoginRPCClient::getRemoteAuthManager()
{
    return &remoteAuthManager;
}

