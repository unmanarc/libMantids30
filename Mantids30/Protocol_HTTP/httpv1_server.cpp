#include "httpv1_server.h"

#include <memory>

#include <Mantids30/Memory/b_mmap.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;
using namespace boost::algorithm;

using namespace std;
using namespace Mantids30::Network::Protocols;
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
HTTP::HTTPv1_Server::HTTPv1_Server(std::shared_ptr<StreamableObject> connectionStream)
    : HTTPv1_Base(false, connectionStream)
{
    // Default to a conservative cache policy for dynamic responses.
    serverResponse.cacheControl.optionNoCache = true;
    serverResponse.cacheControl.optionNoStore = true;
    serverResponse.cacheControl.optionMustRevalidate = true;

    // Start parsing from the request line
    m_currentSubParser = (Memory::Streams::SubParser *) (&clientRequest.requestLine);

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
        return changeToNextParserFromClientRequestLine();
    else if (m_currentSubParser == &clientRequest.headers)
        return changeToNextParserFromClientHeaders();
    else if (m_currentSubParser == &webSocketFrame.header)
        return changeToNextParserFromWebSocketFrameHeader();
    else if (m_currentSubParser == &webSocketFrame.content)
        return changeToNextParserFromWebSocketFrameContent();
    else
        return changeToNextParserFromClientContentData();
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
        return sendFullHTTPResponse();
    }

    size_t headerIncommingContentLength;

    // Gets the content length and create the container that will receive the data.
    if (!setupContentHandling(headerIncommingContentLength))
    {
        return sendFullHTTPResponse();
    }

    enum eProtocolRequestType {
        PROTOCOL_REQUEST_SIMPLE_HTTP,
        PROTOCOL_REQUEST_WEBSOCKETS,
    };

    eProtocolRequestType protocolRequestType = PROTOCOL_REQUEST_SIMPLE_HTTP;

    // Validate WebSocket Protocol:
    protocolRequestType = !isWebSocketConnectionRequest()? protocolRequestType:PROTOCOL_REQUEST_WEBSOCKETS;

    // Manage current protocol:
    switch ( protocolRequestType )
    {
        // HTTP PROTOCOL (REQ/RES)
        case PROTOCOL_REQUEST_SIMPLE_HTTP:
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
        } break;
        // WEBSOCKETS PROTOCOL
        case PROTOCOL_REQUEST_WEBSOCKETS:
        {
            if (!onWebSocketHTTPClientHeadersReceived())
            {
                // Here the consumer decided to deliver a http response.
                return sendFullHTTPResponse();
            }
            else
            {
                // Authentication and everything went fine, send the header and start processing messages:
                if (setupAndSendWebSocketHeaderResponse())
                {
                    // Connection established. <<
                    onWebSocketConnectionEstablished();

                    // Receive the frame header...
                    m_currentSubParser = &webSocketFrame.header;
                    return true;
                }
                else
                {
                    m_currentSubParser = nullptr;
                    return false;
                }
            }
        } break;
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
        return sendFullHTTPResponse();
    }
    else
    {
        m_currentSubParser = !onHTTPClientURIReceived()? nullptr : &clientRequest.headers;
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
    m_currentSubParser = nullptr;
    serverResponse.status.setCode(onHTTPClientContentReceived());
    return sendFullHTTPResponse();
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
        serverResponse.status.setCode(HTTP::Status::S_505_HTTP_VERSION_NOT_SUPPORTED);
        return false;
    }
    else
    {
        serverResponse.status.getHTTPVersion()->setMinor(clientRequest.requestLine.getHTTPVersion()->getMinor());
        return true;
    }
}
