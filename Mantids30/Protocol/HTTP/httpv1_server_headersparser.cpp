#include "httpv1_server.h"
#include <boost/algorithm/string/predicate.hpp>

#include <Mantids30/Core/Helpers/encoders.h>

using namespace std;
using namespace Mantids30::Protocol;
using namespace Mantids30;

void Mantids30::Protocol::HTTP::HTTPv1_Server::parseAllClientHeaders()
{
    parseHostHeader();
    parseAuthenticationHeaders();
    parseUserAgent();
}

// Parse the Host header and extract host and port information
void Mantids30::Protocol::HTTP::HTTPv1_Server::parseHostHeader()
{
    const string hostValue = clientRequest.headers.getOptionValueStringByName("HOST");
    if (hostValue.empty())
    {
        return;
    }

    // Default port for HTTP
    clientRequest.virtualPort = 80;

    size_t colonPos = hostValue.find(':');
    if (hostValue.front() == '[')
    {
        const auto close = hostValue.find(']');
        if (close == string::npos)
        {
            return;
        }
        colonPos = close + 1 < hostValue.size() ? close + 1 : string::npos;
        if (colonPos != string::npos && hostValue[colonPos] != ':')
        {
            return;
        }
    }
    clientRequest.virtualHost = hostValue.substr(0, colonPos);
    if (colonPos != string::npos)
    {
        parsePort(hostValue.substr(colonPos + 1));
    }
}

void Mantids30::Protocol::HTTP::HTTPv1_Server::parsePort(std::string_view portStr)
{
    uint32_t port = 0;

    auto [ptr, ec] = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port, 10);

    if (ec == std::errc{} && ptr == portStr.data() + portStr.size() && port <= 65535)
    {
        clientRequest.virtualPort = static_cast<uint16_t>(port);
    }
    else
    {
        clientRequest.virtualHost.clear();
    }
}

void Mantids30::Protocol::HTTP::HTTPv1_Server::parseAuthenticationHeaders()
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

bool Mantids30::Protocol::HTTP::HTTPv1_Server::parseBasicAuth(std::string_view authHeader)
{
    // Expected format: "Basic <base64-encoded-credentials>"
    const string BASIC_PREFIX = "Basic ";
    if (!boost::starts_with(authHeader, BASIC_PREFIX))
    {
        return false;
    }

    const string encodedCredentials(authHeader.substr(BASIC_PREFIX.length()));
    const string decodedCredentials = Mantids30::Core::Helpers::Encoders::decodeFromBase64(encodedCredentials);

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

void Mantids30::Protocol::HTTP::HTTPv1_Server::parseUserAgent()
{
    if (clientRequest.headers.exist("User-Agent"))
    {
        clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");
    }
}

// Parse Content-Length and Content-Type headers
bool Mantids30::Protocol::HTTP::HTTPv1_Server::setupContentHandling(size_t &contentLength)
{
    contentLength = 0;
    const auto lengths = clientRequest.headers.getOptionsByName("Content-Length");
    const auto codings = clientRequest.headers.getOptionsByName("Transfer-Encoding");
    const auto badRequest = [&]()
    {
        serverResponse.status.setCode(Status::Code::S_400_BAD_REQUEST);
        return false;
    };
    if (lengths.size() > 1 || codings.size() > 1 || (!lengths.empty() && !codings.empty()))
    {
        return badRequest();
    }
    if (!lengths.empty())
    {
        const string value = clientRequest.headers.getOptionRawStringByName("Content-Length");
        const auto result = std::from_chars(value.data(), value.data() + value.size(), contentLength);
        if (value.empty() || result.ec != std::errc() || result.ptr != value.data() + value.size())
        {
            return badRequest();
        }
    }
    const bool chunked = !codings.empty();
    if (chunked && !boost::iequals(clientRequest.headers.getOptionRawStringByName("Transfer-Encoding"), "chunked"))
    {
        return badRequest();
    }
    string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
    clientRequest.content.setTransmissionMode(chunked ? Content::TransmissionMode::CHUNKS : Content::TransmissionMode::CONTENT_LENGTH);
    if (!chunked && !clientRequest.content.setCurrentSize(contentLength))
    {
        serverResponse.status.setCode(Mantids30::Protocol::HTTP::Status::Code::S_413_PAYLOAD_TOO_LARGE);
        return false;
    }
    if (contentLength || chunked)
    {
        if (boost::icontains(contentType, "multipart/form-data"))
        {
            clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::MIME);
            clientRequest.content.getMultiPartVars()->setMultiPartBoundary(clientRequest.headers.getOptionByName("Content-Type")->getSubVar("boundary"));
        }
        else if (boost::icontains(contentType, "application/x-www-form-urlencoded"))
        {
            clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::URL);
        }
        else if (boost::icontains(contentType, "application/json"))
        {
            clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::JSON);
        }
        else
        {
            clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::BIN);
        }
        /////////////////////////////////////////////////////////////////////////////////////
    }
    return true;
}

bool Mantids30::Protocol::HTTP::HTTPv1_Server::validateHTTPv11Requirements()
{
    // HTTP/1.1+ requires Host header
    if (clientRequest.headers.getOptionsByName("Host").size() > 1
        || (clientRequest.headers.exist("Host") && clientRequest.virtualHost.empty())
        || (clientRequest.requestLine.getHTTPVersion()->getMinor() >= 1 && clientRequest.virtualHost.empty()))
    {
        serverResponse.status.setCode(Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST);
        return false;
    }
    return true;
}
