#include "httpv1_server.h"
#include <boost/algorithm/string/predicate.hpp>

#include <Mantids30/Helpers/encoders.h>

using namespace std;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

void HTTP::HTTPv1_Server::parseAllClientHeaders()
{
    parseHostHeader();
    parseAuthenticationHeaders();
    parseUserAgent();
}

// Parse the Host header and extract host and port information
void HTTP::HTTPv1_Server::parseHostHeader()
{
    const string hostValue = clientRequest.headers.getOptionValueStringByName("HOST");
    if (hostValue.empty())
    {
        return;
    }

    // Default port for HTTP
    clientRequest.virtualPort = 80;

    // Split host:port
    size_t colonPos = hostValue.find(':');
    if (colonPos == string::npos)
    {
        clientRequest.virtualHost = hostValue;
    }
    else
    {
        clientRequest.virtualHost = hostValue.substr(0, colonPos);
        parsePort(hostValue.substr(colonPos + 1));
    }
}

void HTTP::HTTPv1_Server::parsePort(const string &portStr)
{
    char *endptr;
    unsigned long port = (uint16_t) strtoul(portStr.c_str(), &endptr, 10);
    if (*endptr == '\0' && port <= 65535) // Check if conversion was successful and port is valid
    {
        clientRequest.virtualPort = (uint16_t) port;
    }
    else
    {
        // Invalid port number, set to default or handle error appropriately
        clientRequest.virtualPort = 80;
    }
}

void HTTP::HTTPv1_Server::parseAuthenticationHeaders()
{
    clientRequest.basicAuth.isEnabled = false;

    if (!clientRequest.headers.exist("Authorization"))
    {
        return;
    }

    const string authHeader = clientRequest.headers.getOptionValueStringByName("Authorization");
    if (!parseBasicAuth(authHeader))
    {
        // TODO: Log authentication parsing error if needed
    }
}

bool HTTP::HTTPv1_Server::parseBasicAuth(const string &authHeader)
{
    // Expected format: "Basic <base64-encoded-credentials>"
    const string BASIC_PREFIX = "Basic ";
    if (!boost::starts_with(authHeader, BASIC_PREFIX))
    {
        return false;
    }

    const string encodedCredentials = authHeader.substr(BASIC_PREFIX.length());
    const string decodedCredentials = Helpers::Encoders::decodeFromBase64(encodedCredentials);

    size_t colonPos = decodedCredentials.find(':');
    if (colonPos == string::npos)
    {
        return false;
    }

    clientRequest.basicAuth.isEnabled = true;
    clientRequest.basicAuth.username = decodedCredentials.substr(0, colonPos);
    clientRequest.basicAuth.password = decodedCredentials.substr(colonPos + 1);
    return true;
}


void HTTP::HTTPv1_Server::parseUserAgent()
{
    if (clientRequest.headers.exist("User-Agent"))
    {
        clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");
    }
}

// Parse Content-Length and Content-Type headers
bool HTTP::HTTPv1_Server::setupContentHandling(size_t &contentLength)
{
    // Initialize in zero:
    clientRequest.content.setCurrentSize(0);

    // Extract payload size and content type hints from headers.
    contentLength = clientRequest.headers.getOptionAsUINT64("Content-Length");
    string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
    if (contentLength)
    {
        clientRequest.content.setTransmitionMode(HTTP::Content::TRANSMIT_MODE_CONTENT_LENGTH);
        if (!clientRequest.content.setCurrentSize(contentLength))
        {
            // Abort: the advertised length cannot be allocated within limits.
            serverResponse.status.setCode(HTTP::Status::S_413_PAYLOAD_TOO_LARGE);
            return false;
        }
        if (boost::icontains(contentType, "multipart/form-data"))
        {
            clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_MIME);
            clientRequest.content.getMultiPartVars()->setMultiPartBoundary(clientRequest.headers.getOptionByName("Content-Type")->getSubVar("boundary"));
        }
        else if (boost::icontains(contentType, "application/x-www-form-urlencoded"))
        {
            clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_URL);
        }
        else if (boost::icontains(contentType, "application/json"))
        {
            clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_JSON);
        }
        else
            clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_BIN);
        /////////////////////////////////////////////////////////////////////////////////////
    }
    return true;
}



bool HTTP::HTTPv1_Server::validateHTTPv11Requirements()
{
    // HTTP/1.1+ requires Host header
    if (clientRequest.requestLine.getHTTPVersion()->getMinor() >= 1 &&
        clientRequest.virtualHost.empty())
    {
        serverResponse.status.setCode(HTTP::Status::S_400_BAD_REQUEST);
        return false;
    }
    return true;
}


