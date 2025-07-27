#include "apiproxy.h"
#include "Mantids30/Net_Sockets/socket_stream.h"

#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Net_Sockets/socket_tcp.h>

#include <Mantids30/Protocol_HTTP/httpv1_client.h>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory::Streams;

using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;

// TODO: logs via callback?
// TODO: how to prvent ../ (escapes)...

HTTP::Status::Codes Mantids30::Network::Servers::Web::ApiProxy(
    const std::string &internalPath, HTTP::HTTPv1_Base::Request *request, HTTP::HTTPv1_Base::Response *response, std::shared_ptr<void> obj)
{
    if (obj == nullptr)
    {
        throw std::runtime_error("Undefined API Proxy Object.");
        return HTTP::Status::S_500_INTERNAL_SERVER_ERROR;
    }

    ApiProxyParameters *proxyParameters = static_cast<ApiProxyParameters *>(obj.get());

    std::shared_ptr<Socket_Stream> connection;

    if (proxyParameters->useTLS)
    {
        auto socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            socket->setCertValidation(Socket_TLS::CERT_X509_VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            socket->setCertValidation(Socket_TLS::CERT_X509_NOVALIDATE);
        }

        connection = socket;

    }
    else
    {
        auto socket = std::make_shared<Socket_TCP>();
        connection = socket;
    }

    // Make the connection
    if (connection->connectTo( proxyParameters->remoteHost.c_str(), proxyParameters->remotePort ))
    {
        HTTP::HTTPv1_Client client(connection);

        // Set the same request.
        client.clientRequest = *request;

        // Use the new internal path (removing the proxy original URL)...
        client.clientRequest.requestLine.setRequestURI( internalPath );

        // Replace current headers with extra headers (eg. x-api-key... X-Originating-IP )
        for (const auto& header : proxyParameters->extraHeaders)
        {
            client.clientRequest.headers.replace(header.first, header.second);
        }

        // Make the petition...
        Parser::ErrorMSG msg;
        client.parseObject(&msg);

        if (msg == Parser::PARSING_SUCCEED)
        {
            *response = client.serverResponse;
            return (HTTP::Status::Codes)client.serverResponse.status.getCode();
        }

        return HTTP::Status::S_502_BAD_GATEWAY;
    }
    else
    {
        return HTTP::Status::S_502_BAD_GATEWAY;
    }
}
