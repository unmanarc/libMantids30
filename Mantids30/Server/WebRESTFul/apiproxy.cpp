#include "apiproxy.h"
#include <Mantids30/Network/Sockets/socket_stream.h>

#include <Mantids30/Network/Sockets/socket_tcp.h>
#include <Mantids30/Network/Sockets/socket_tls.h>

#include <Mantids30/Protocol/HTTP/httpv1_client.h>
#include <memory>
#include <string>
#include <cstdarg>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Protocol;
using namespace Mantids30::Core::Memory::Streams;
using namespace Mantids30::DataFormat::JSON;
using namespace Mantids30::Runtime::Logs;

using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30;
using namespace Mantids30::Server::WebRESTFul::Sessions;

Mantids30::Protocol::HTTP::Status::Code Mantids30::Server::WebRESTFul::APIProxy(const std::string &internalPath, Mantids30::Protocol::HTTP::HTTPv1_Base::Request *request, Mantids30::Protocol::HTTP::HTTPv1_Base::Response *response,
                                                              const std::shared_ptr<void> &obj, const Mantids30::Server::WebRESTFul::Sessions::SessionInfo *sessionInfo)
{
    // TODO: how to prevent ../ (escapes)...

    // Define auth header names
    static const std::string hdrAuthUser = "X-Auth-User";
    static const std::string hdrAuthDomain = "X-Auth-Domain";
    static const std::string hdrAuthHalfSessionId = "X-Auth-Half-Session-Id";
    static const std::string hdrAuthImpersonation = "X-Auth-Impersonation";
    static const std::string hdrAuthImpersonator = "X-Auth-Impersonator";
    static const std::vector<std::string> authHeaders = {hdrAuthUser, hdrAuthDomain, hdrAuthHalfSessionId, hdrAuthImpersonation, hdrAuthImpersonator};

    APIProxyParameters *proxyParameters = (obj != nullptr) ? static_cast<APIProxyParameters *>(obj.get()) : nullptr;

    // Logging helper: uses the proxy's configured application log (proxyParameters->appLog)
    // and includes user + IP when an authenticated session is available, otherwise only
    // the client IP. Sensitive data (eg. extra headers) is never logged.
    const std::string clientIP = (request != nullptr) ? request->networkClientInfo.REMOTE_ADDR : "";
    auto doLog = [proxyParameters, sessionInfo, clientIP](std::string_view module, LogLevel level, const char *fmt, ...)
    {
        if (proxyParameters == nullptr || proxyParameters->appLog == nullptr)
        {
            return;
        }

        va_list args;
        va_start(args, fmt);
        char buffer[2048];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        std::string user;
        if (sessionInfo != nullptr && sessionInfo->authSession != nullptr)
        {
            if (!sessionInfo->authSession->getImpersonator().empty())
            {
                user = sessionInfo->authSession->getUser() + "<-" + sessionInfo->authSession->getImpersonator();
            }
            else
            {
                user = sessionInfo->authSession->getUser();
            }
        }

        if (user.empty())
        {
            proxyParameters->appLog->log1(module, clientIP, level, "%s", buffer);
        }
        else
        {
            proxyParameters->appLog->log2(module, user, clientIP, level, "%s", buffer);
        }
    };

    if (proxyParameters == nullptr)
    {
        doLog(__func__, LogLevel::ERROR, "Undefined API Proxy Object for %s %s.", request != nullptr ? request->requestLine.getHTTPMethod().c_str() : "???",
              request != nullptr ? request->getURI().c_str() : "???");
        throw std::runtime_error("Undefined API Proxy Object.");
        return Mantids30::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
    }

    doLog(__func__, LogLevel::DEBUG, "Proxying %s %s -> %s:%u (TLS=%s, proxyPath=%s).", request->requestLine.getHTTPMethod().c_str(), internalPath.c_str(), proxyParameters->remoteHost.c_str(),
          static_cast<unsigned int>(proxyParameters->remotePort), proxyParameters->useTLS ? "yes" : "no", proxyParameters->proxyPath.c_str());

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
                if (!socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath))
                {
                    doLog(__func__, LogLevel::ERROR, "Failed to load private CA from '%s' for upstream %s:%u.", proxyParameters->privateCAPath.c_str(), proxyParameters->remoteHost.c_str(),
                          static_cast<unsigned int>(proxyParameters->remotePort));
                    return Mantids30::Protocol::HTTP::Status::Code::S_502_BAD_GATEWAY;
                }
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
        Mantids30::Protocol::HTTP::HTTPv1_Client client(connection);

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

            const auto statusCode = client.serverResponse.status.getCode();
            const auto statusCodeValue = static_cast<int>(statusCode);

            if (statusCodeValue >= 500)
            {
                doLog(__func__, LogLevel::WARNING, "Upstream %s:%u returned status %d for %s %s.", proxyParameters->remoteHost.c_str(), static_cast<unsigned int>(proxyParameters->remotePort),
                      statusCodeValue, request->requestLine.getHTTPMethod().c_str(), internalPath.c_str());
            }
            else
            {
                doLog(__func__, LogLevel::DEBUG, "Upstream %s:%u returned status %d for %s %s.", proxyParameters->remoteHost.c_str(), static_cast<unsigned int>(proxyParameters->remotePort),
                      statusCodeValue, request->requestLine.getHTTPMethod().c_str(), internalPath.c_str());
            }

            // Pass to our client.
            *response = client.serverResponse;
            return statusCode;
        }

        doLog(__func__, LogLevel::ERROR, "Failed to parse upstream response from %s:%u (TLS=%s) for %s %s.", proxyParameters->remoteHost.c_str(), static_cast<unsigned int>(proxyParameters->remotePort),
              proxyParameters->useTLS ? "yes" : "no", request->requestLine.getHTTPMethod().c_str(), internalPath.c_str());
        return Mantids30::Protocol::HTTP::Status::Code::S_502_BAD_GATEWAY;
    }
    else
    {
        doLog(__func__, LogLevel::ERROR, "Failed to connect to upstream %s:%u (TLS=%s) for %s %s.", proxyParameters->remoteHost.c_str(), static_cast<unsigned int>(proxyParameters->remotePort),
              proxyParameters->useTLS ? "yes" : "no", request->requestLine.getHTTPMethod().c_str(), internalPath.c_str());
        return Mantids30::Protocol::HTTP::Status::Code::S_502_BAD_GATEWAY;
    }
}
