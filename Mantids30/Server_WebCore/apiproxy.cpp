#include "apiproxy.h"
#include <Mantids30/Net_Sockets/socket_stream.h>

#include <Mantids30/Net_Sockets/socket_tcp.h>
#include <Mantids30/Net_Sockets/socket_tls.h>

#include <Mantids30/Protocol_HTTP/httpv1_client.h>
#include <memory>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Memory::Streams;

using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace Mantids30::Sessions;

HTTP::Status::Code Mantids30::Network::Servers::Web::APIProxy(const std::string &internalPath, HTTP::HTTPv1_Base::Request *request, HTTP::HTTPv1_Base::Response *response,
                                                              const std::shared_ptr<void> &obj, const Sessions::SessionInfo *sessionInfo)
{
    // TODO: logs via callback?
    // TODO: how to prevent ../ (escapes)...

    // Define auth header names
    static const std::string hdrAuthUser = "X-Auth-User";
    static const std::string hdrAuthDomain = "X-Auth-Domain";
    static const std::string hdrAuthHalfSessionId = "X-Auth-Half-Session-Id";
    static const std::string hdrAuthImpersonation = "X-Auth-Impersonation";
    static const std::string hdrAuthImpersonator = "X-Auth-Impersonator";
    static const std::vector<std::string> authHeaders = {hdrAuthUser, hdrAuthDomain, hdrAuthHalfSessionId, hdrAuthImpersonation, hdrAuthImpersonator};

    if (obj == nullptr)
    {
        throw std::runtime_error("Undefined API Proxy Object.");
        return HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
    }

    APIProxyParameters *proxyParameters = static_cast<APIProxyParameters *>(obj.get());

    std::shared_ptr<Socket_Stream> connection;

    if (proxyParameters->useTLS)
    {
        std::shared_ptr<Socket_TLS> socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            socket->setCertValidation(Socket_TLS::X509ValidationOption::VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            socket->setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
        }

        socket->setTcpNoDelayOption(false);

        connection = socket;
    }
    else
    {
        std::shared_ptr<Socket_TCP> socket = std::make_shared<Socket_TCP>();

        socket->setTcpNoDelayOption(false);

        connection = socket;
    }

    // Make the connection
    if (connection->connectTo(proxyParameters->remoteHost.c_str(), proxyParameters->remotePort))
    {
        HTTP::HTTPv1_Client client(connection);

        // Set the same request.
        client.clientRequest = *request;

        // Use the new internal path (removing the proxy original URL)...
        client.clientRequest.requestLine.setRequestURI(internalPath);

        // Replace current headers with extra headers (eg. x-api-key... X-Originating-IP )
        for (const auto &header : proxyParameters->extraHeaders)
        {
            client.clientRequest.headers.replace(header.first, header.second);
        }

        // Inject authentication context headers if there's an active session,
        // otherwise remove them to prevent header injection attacks.
        if (sessionInfo != nullptr && sessionInfo->authSession != nullptr)
        {
            client.clientRequest.headers.replace(hdrAuthUser, sessionInfo->authSession->getUser());
            client.clientRequest.headers.replace(hdrAuthDomain, sessionInfo->authSession->getDomain());
            client.clientRequest.headers.replace(hdrAuthHalfSessionId, sessionInfo->halfSessionId);
            client.clientRequest.headers.replace(hdrAuthImpersonation, sessionInfo->isImpersonation ? "true" : "false");
            client.clientRequest.headers.replace(hdrAuthImpersonator, sessionInfo->authSession->getImpersonator());
        }
        else
        {
            // No active session: remove auth headers to prevent injection
            for (const auto &hdr : authHeaders)
            {
                client.clientRequest.headers.remove(hdr);
            }
        }

        client.clientRequest.headers.replace("Connection", "close");

        // Make the petition...
        Parser::ParseResult msg;
        client.parseObject(&msg);

        if (msg == Parser::ParseResult::SUCCEED)
        {
            // Transform cookie paths if enabled
            if (proxyParameters->transformCookiePath)
            {
                client.serverResponse.cookies.prependPathToAllCookies(proxyParameters->proxyPath);
            }

            client.serverResponse.immutableHeaders = true;

            // Pass to our client.
            *response = client.serverResponse;

            return client.serverResponse.status.getCode();
        }

        return HTTP::Status::Code::S_502_BAD_GATEWAY;
    }
    else
    {
        return HTTP::Status::Code::S_502_BAD_GATEWAY;
    }
}
