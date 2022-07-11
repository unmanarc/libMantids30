#include "loginrpcclient.h"
#include <mdz_net_sockets/streams_cryptochallenge.h>

#include <thread>

using namespace Mantids::Network::Sockets;
using namespace Mantids::Authentication;

LoginRPCClient::LoginRPCClient()
{
    remoteHost="127.0.0.1";
    remotePort=30301;
    useIPv6=false;

    caFile = "ca.crt";
}

void LoginRPCClient::process(LoginRPCClient *rpcClient,uint16_t sleepBetweenConnectionsSeconds)
{
    for (;;)
    {
        Socket_TLS tlsClient;

        // Authenticate that the server with X.509
        tlsClient.setTLSCertificateAuthorityPath(rpcClient->getCaFile().c_str());

        // If there is any client certificate, set the client certificate (Usually this is not needed because it's sufficient to use the API LOGINRPC CONNECTION KEY to authenticate)
        if (!rpcClient->getCertFile().empty())
            tlsClient.setTLSPublicKeyPath(rpcClient->getCertFile().c_str());

        // If there is any client key file, set the key file (Usually this is not needed because it's sufficient to use the API LOGINRPC CONNECTION KEY to authenticate)
        if (!rpcClient->getKeyFile().empty())
            tlsClient.setTLSPrivateKeyPath(rpcClient->getKeyFile().c_str());

        // Callback to notifyTLSConnecting which occurs just before the connection.
        rpcClient->notifyTLSConnecting(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort());

        // Connect to the remote FastRPC TLS Server
        if (tlsClient.connectTo( rpcClient->getRemoteHost().c_str(),rpcClient->getRemotePort() ))
        {
            // TLS connection is established here
            rpcClient->notifyTLSConnectedOK(&tlsClient);

            // Initialize the crypto stream (for crypto challenge authentication)
            NetStreams::CryptoChallenge cstream(&tlsClient);
            // Send the Application name
            tlsClient.writeStringEx<uint16_t>(rpcClient->getAppName());
            // Exchange the crypto challenge/response for the authentication (both nodes will authenticate that the other one haves the challenge key without sharing it)
            if (cstream.mutualChallengeResponseSHA256Auth(rpcClient->getApiKey(),false) == std::make_pair(true,true))
            {
                // Notify that we are fully authenticated here (mutual crypto challenge succeed)
                rpcClient->notifyAPIProcessingOK(&tlsClient);

                // Process authentication functions, while we are into this function, any process can interact with the remote server
                int code = rpcClient->getRemoteAuthManager()->processFastRPCConnection(&tlsClient);

                // When we are outside the processFastRPCConnection function, request will fail.

                // Notify that the connection was terminated with this integer code (given the result)
                rpcClient->notifyTLSDisconnected(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort(), code);
            }
            else
            {
                // Notify that the crypto-challenge failed (bad api key), client and server api key are not equal.
                rpcClient->notifyBadApiKey(&tlsClient);
            }
        }
        else
        {
            // Notify that there is an error during the TLS/TCP-IP connection
            rpcClient->notifyTLSErrorConnecting(&tlsClient,rpcClient->getRemoteHost(),rpcClient->getRemotePort());
        }

        // Sleep until the next try
        sleep(sleepBetweenConnectionsSeconds);
    }
}

void LoginRPCClient::start(uint16_t sleepBetweenConnectionsSeconds)
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

