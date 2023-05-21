#include "loginrpcclient.h"
#include <Mantids29/Net_Sockets/streams_cryptochallenge.h>

#include <thread>

using namespace Mantids29::Network::Sockets;
using namespace Mantids29::Authentication;

LoginRPCClient::LoginRPCClient()
{
    m_remoteHost="127.0.0.1";
    m_remotePort=30302;
    m_useIPv6=false;
    m_usePreSharedKeyForTLS=true;
    m_CertificateAuthorityFilePath = "ca.crt";
}

void LoginRPCClient::process(LoginRPCClient *rpcClient,uint16_t sleepBetweenConnectionsSeconds)
{
    for (;;)
    {
        Socket_TLS tlsClient;

        if (!rpcClient->m_usePreSharedKeyForTLS)
        {
            // Set the SO default security level:
            tlsClient.m_keys.setSecurityLevel(-1);

            // Authenticate that the server with X.509
            tlsClient.m_keys.loadCAFromPEMFile(rpcClient->m_CertificateAuthorityFilePath.c_str());

            // If there is any client certificate, set the client certificate (Usually this is not needed because it's sufficient to use the API LOGINRPC CONNECTION KEY to authenticate)
            if (!rpcClient->m_certificateFilePath.empty())
                tlsClient.m_keys.loadPublicKeyFromPEMFile(rpcClient->m_certificateFilePath.c_str());

            // If there is any client key file, set the key file (Usually this is not needed because it's sufficient to use the API LOGINRPC CONNECTION KEY to authenticate)
            if (!rpcClient->m_keyFilePath.empty())
                tlsClient.m_keys.loadPrivateKeyFromPEMFile(rpcClient->m_keyFilePath.c_str());
        }
        else
        {
            tlsClient.m_keys.loadPSKAsClient( rpcClient->m_applicationName, rpcClient->m_apiKey );
        }

        // Callback to notifyTLSConnecting which occurs just before the connection.
        rpcClient->notifyTLSConnecting(&tlsClient,rpcClient->m_remoteHost,rpcClient->m_remotePort);

        // Connect to the remote FastRPC TLS Server
        if (tlsClient.connectTo( rpcClient->m_remoteHost.c_str(),rpcClient->m_remotePort ))
        {
            // TLS connection is established here
            rpcClient->notifyTLSConnectedOK(&tlsClient);

            // Initialize the crypto stream (for crypto challenge authentication)
            NetStreams::CryptoChallenge cstream(&tlsClient);
            // Send the Application name
            tlsClient.writeStringEx<uint16_t>(rpcClient->m_applicationName);
            // Exchange the crypto challenge/response for the authentication (both nodes will authenticate that the other one haves the challenge key without sharing it)
            if (cstream.mutualChallengeResponseSHA256Auth(rpcClient->m_apiKey,false) == std::make_pair(true,true))
            {
                // Notify that we are fully authenticated here (mutual crypto challenge succeed)
                rpcClient->notifyAPIProcessingOK(&tlsClient);

                // Process authentication functions, while we are into this function, any process can interact with the remote server
                int code = rpcClient->getRemoteAuthManager()->processFastRPCConnection(&tlsClient);

                // When we are outside the processFastRPCConnection function, request will fail.

                // Notify that the connection was terminated with this integer code (given the result)
                rpcClient->notifyTLSDisconnected(&tlsClient,rpcClient->m_remoteHost,rpcClient->m_remotePort, code);
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
            rpcClient->notifyTLSErrorConnecting(&tlsClient,rpcClient->m_remoteHost,rpcClient->m_remotePort);
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

Manager_Remote *LoginRPCClient::getRemoteAuthManager()
{
    return &remoteAuthManager;
}
