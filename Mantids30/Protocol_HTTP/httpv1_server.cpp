#include "httpv1_server.h"

#include <memory>

#include <Mantids30/Memory/b_mmap.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;
using namespace boost::algorithm;

using namespace std;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Network;
using namespace Mantids30;

/**
 * @brief Constructor for HTTPv1_Server.
 *
 * Initializes the HTTP/1.x server-side parser, sets default cache control headers,
 * and configures the initial parser state to process the request line.
 *
 * @param connectionStream Shared pointer to the streamable object used for communication.
 */
HTTP::HTTPv1_Server::HTTPv1_Server(const std::shared_ptr<StreamableObject> &connectionStream)
    : HTTPv1_Base(false, connectionStream)
{
    // Modified now (unless specified)!
    HTTP::Date fileModificationDate;
    fileModificationDate.setUnixTime(time(nullptr));
    serverResponse.headers.replace("Last-Modified", fileModificationDate.toString());

    // Start parsing from the request line
    m_currentSubParser = static_cast<Memory::Streams::SubParser *>(&clientRequest.requestLine);
    loadDefaultMIMETypes();
}

/**
 * @brief Progresses the parser through different stages of the HTTP request.
 *
 * Transitions from one parsing stage (request line, headers, content) to the next
 * based on the current parser state.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParser()
{
    // Server mode progresses through request line → headers → body.
    if (m_currentSubParser == &clientRequest.requestLine)
    {
        return changeToNextParserFromClientRequestLine();
    }
    else if (m_currentSubParser == &clientRequest.headers)
    {
        return changeToNextParserFromClientHeaders();
    }
    else if (m_currentSubParser == &webSocketCurrentFrame.header)
    {
        return changeToNextParserFromWebSocketFrameHeader();
    }
    else if (m_currentSubParser == &webSocketCurrentFrame.content)
    {
        return changeToNextParserFromWebSocketFrameContent();
    }
    else
    {
        return changeToNextParserFromClientContentData();
    }
}

void HTTP::HTTPv1_Server::reset()
{
    // Reset all components except for connection-related information, which should remain static.
    serverResponse = Response();
    HTTPv1_Base::Request::NetworkClientInfo preservedClientInfo = clientRequest.networkClientInfo;
    clientRequest = Request();
    clientRequest.networkClientInfo = preservedClientInfo;
}

/**
 * @brief Handles parsing after the request line has been fully received.
 *
 * Processes HTTP headers, extracts metadata (host, auth, user-agent, etc.), and
 * determines whether to expect a body or respond immediately.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientHeaders()
{
    // Client headers have been received; parse metadata and decide next step.

    // Parse all headers first
    parseAllClientHeaders();

    // Validate HTTP requirements
    if (!validateHTTPv11Requirements())
    {
        connectionContinue = false;
        return sendFullHTTPResponse();
    }

    size_t headerIncommingContentLength;

    // Gets the content length and create the container that will receive the data.
    if (!setupContentHandling(headerIncommingContentLength))
    {
        connectionContinue = false;
        return sendFullHTTPResponse();
    }

    enum class ProtocolType : uint8_t
    {
        SIMPLE_HTTP,
        WEBSOCKETS,
    };

    ProtocolType protocolRequestType = ProtocolType::SIMPLE_HTTP;

    // Validate WebSocket Protocol:
    protocolRequestType = !isWebSocketConnectionRequest() ? protocolRequestType : ProtocolType::WEBSOCKETS;

    // Manage current protocol:
    switch (protocolRequestType)
    {
    // HTTP PROTOCOL (REQ/RES)
    case ProtocolType::SIMPLE_HTTP:
    {
        // Headers parsed. Allow consumer code to inspect headers
        if (!onHTTPClientHeadersReceived())
        {
            // Here the consumer decided to terminate the connection.
            m_currentSubParser = nullptr;
            return true;
        }

        if (headerIncommingContentLength == 0)
        {
            // No body expected, pass to the next phase.
            return changeToNextParserFromClientContentData();
        }
        else
        {
            // Don´t respond here, change the parser to receive the content.
            m_currentSubParser = &clientRequest.content;
            return true;
        }
    }
    break;
    // WEBSOCKETS PROTOCOL
    case ProtocolType::WEBSOCKETS:
    {
        if (prohibitConnectionUpgrade)
        {
            // Upgrade should start in the first request.
            m_currentSubParser = nullptr;
            return false;
        }

        if (!onWebSocketHTTPClientHeadersReceived())
        {
            connectionContinue = false;
            // Not authenticated or the endpoint does not exist.
            return sendFullHTTPResponse();
        }

        // Authentication and everything went fine, send the header and start processing messages:
        if (setupAndSendWebSocketHeaderResponse())
        {
            // Connection established. <<
            onWebSocketConnectionEstablished();

            // Receive the frame header...
            m_currentSubParser = &webSocketCurrentFrame.header;
            return true;
        }
        else
        {
            m_currentSubParser = nullptr;
            return false;
        }
    }
    break;
    default:
        return false;
    }
}

/**
 * @brief Handles parsing after the request line has been parsed.
 *
 * Validates the HTTP version and URI, then transitions to parsing headers.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientRequestLine()
{
    // Request-line parsed; validate URI and HTTP version before reading headers.
    if (!prepareServerVersionOnURI())
    {
        connectionContinue = false;
        return sendFullHTTPResponse();
    }
    else
    {
        if (!onHTTPClientURIReceived())
        {
            m_currentSubParser = nullptr;
        }
        else
        {
            m_currentSubParser = &clientRequest.headers;
        }
    }
    return true;
}

/**
 * @brief Handles parsing of the body content.
 *
 * Currently, streaming bodies are not supported. This method ends parsing
 * and sends the HTTP response.
 *
 * @return Always returns true.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientContentData()
{
    // TODO: Streaming body support not yet implemented
    serverResponse.status.setCode(onHTTPClientContentReceived());

    // Only check conditional headers on successful GET/HEAD responses (2xx)
    if (static_cast<uint16_t>(serverResponse.status.getCode()) >= 200 && static_cast<uint16_t>(serverResponse.status.getCode()) < 300)
    {
        std::string httpMethod = clientRequest.requestLine.getHTTPMethod();
        if (httpMethod == "GET" || httpMethod == "HEAD")
        {
            // Check If-None-Match header (ETag validation)
            std::string ifNoneMatch = clientRequest.getHeaderOption("If-None-Match");
            if (!ifNoneMatch.empty() && !serverResponse.etag.isEmpty())
            {
                Headers::ETag clientETag;
                clientETag.fromString(ifNoneMatch);
                if (serverResponse.etag.matches(clientETag))
                {
                    // 304 Not Modified - no body
                    serverResponse.status.setCode(HTTP::Status::Code::S_304_NOT_MODIFIED);
                    serverResponse.setContentDataStreamer(nullptr);
                }
            }
            else
            {
                // Check If-Modified-Since header (Last-Modified validation)
                std::string ifModifiedSince = clientRequest.getHeaderOption("If-Modified-Since");
                if (!ifModifiedSince.empty())
                {
                    std::shared_ptr<MIME::MIME_HeaderOption> lastModifiedHeader = serverResponse.headers.getOptionByName("Last-Modified");
                    if (lastModifiedHeader)
                    {
                        // If the Last-Modified matches or is older than If-Modified-Since, return 304
                        std::string lastModifiedStr = lastModifiedHeader->getValue();
                        if (lastModifiedStr == ifModifiedSince)
                        {
                            serverResponse.status.setCode(HTTP::Status::Code::S_304_NOT_MODIFIED);
                            serverResponse.setContentDataStreamer(nullptr);
                        }
                    }
                }
            }
        }
    }

    bool r = sendFullHTTPResponse();
    if (!r)
    {
        r = !r;
        r = !r;
    }
    return r;
}
/**
 * @brief Sets the HTTP version for the server's response based on client request.
 *
 * Ensures the server responds with a compatible HTTP version.
 */
bool HTTP::HTTPv1_Server::prepareServerVersionOnURI()
{
    serverResponse.status.getHTTPVersion()->setMajor(1);
    serverResponse.status.getHTTPVersion()->setMinor(0);

    // Validate major version
    if (clientRequest.requestLine.getHTTPVersion()->getMajor() != 1)
    {
        serverResponse.status.setCode(HTTP::Status::Code::S_505_HTTP_VERSION_NOT_SUPPORTED);
        return false;
    }
    else
    {
        serverResponse.status.getHTTPVersion()->setMinor(clientRequest.requestLine.getHTTPVersion()->getMinor());
        return true;
    }
}
