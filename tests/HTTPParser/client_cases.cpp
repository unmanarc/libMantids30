#include "test_support.h"

namespace HTTPParserTests {
namespace {

using Inspect = std::function<void(Client &)>;

RawCase valid(const std::string &name, const std::string &raw, const std::string &body,
              unsigned status = 200, const std::string &method = "GET", Inspect inspect = {}, bool closeDelimited = false)
{
    return {"client." + name, [=](const auto &parts)
    {
        Client client;
        client.clientRequest.requestLine.setHTTPMethod(method);
        const auto result = feed(client, raw, parts);
        completed(client, result, raw.size());
        require(result.finishedBeforeEOF != closeDelimited, "Wrong completion timing relative to EOF");
        require(static_cast<unsigned>(client.serverResponse.status.getCode()) == status, "Wrong status code");
        require(client.headerCalls == 1, "Final response headers must be delivered exactly once");
        const auto content = client.serverResponse.content.getStreamableObject();
        require(content->toString() == body, "Wrong response body");
        require(content->writeStatus.finished && content->writeStatus.succeed, "Response body destination did not finish successfully");
        if (inspect)
        {
            inspect(client);
        }
    }};
}

RawCase invalid(const std::string &name, const std::string &raw)
{
    return {"client." + name, [=](const auto &parts)
    {
        Client client;
        rejected(client, feed(client, raw, parts));
    }};
}

} // namespace

std::vector<RawCase> clientCases(const std::string &suite)
{
    if (suite == "client_valid")
    {
        const std::string binary("\0\x01\xff\r\nHTTP/1.1\0", 14);
        return {
            valid("fixed_length", response("hello"), "hello"),
            valid("binary_body", response(binary), binary),
            valid("empty_body", response(""), ""),
            valid("http_1_0.fixed_length", "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n\r\nhi", "hi", 200, "GET", [](Client &client)
            {
                require(client.serverResponse.status.getHTTPVersion()->toString() == "HTTP/1.0", "Wrong HTTP version");
            }),
            valid("http_1_0.close_delimited", "HTTP/1.0 200 OK\r\n\r\nhello", "hello", 200, "GET", {}, true),
            valid("http_1_1.close_delimited", "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nhello", "hello", 200, "GET", {}, true),
            valid("close_delimited.empty", "HTTP/1.0 200 OK\r\n\r\n", "", 200, "GET", {}, true),
            valid("status.not_found", "HTTP/1.1 404 Not Found\r\nContent-Length: 3\r\n\r\nbad", "bad", 404, "GET", [](Client &client)
            {
                require(client.serverResponse.status.getMessage() == "Not Found", "Lost multiword reason phrase");
            }),
            valid("status.unknown_extension", "HTTP/1.1 599 Custom Failure\r\nContent-Length: 0\r\n\r\n", "", 599),
            valid("status.empty_reason", "HTTP/1.1 200 \r\nContent-Length: 0\r\n\r\n", ""),
            valid("no_body.head", "HTTP/1.1 200 OK\r\nContent-Length: 999\r\n\r\n", "", 200, "HEAD"),
            valid("no_body.204", "HTTP/1.1 204 No Content\r\n\r\n", "", 204),
            valid("no_body.304", "HTTP/1.1 304 Not Modified\r\nContent-Length: 999\r\n\r\n", "", 304),
            valid("no_body.101", "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n", "", 101),
            valid("informational.multiple", "HTTP/1.1 103 Early Hints\r\nLink: </style.css>\r\nSet-Cookie: interim=discard\r\n\r\n"
                  "HTTP/1.1 100 Continue\r\n\r\n" + response("hello"), "hello", 200, "GET", [](Client &client)
            {
                require(!client.serverResponse.headers.exist("Link"), "Informational headers leaked into final response");
                require(client.serverResponse.cookies.getCookieValueByName("interim").empty(), "Informational cookies leaked");
            }),
            valid("headers.case_insensitive", "HTTP/1.1 200 OK\r\ncOnTeNt-LeNgTh: 2\r\nx-Custom: value\r\n\r\nhi", "hi", 200, "GET", [](Client &client)
            {
                require(client.getResponseHeader("X-CUSTOM") == "value", "Header lookup must ignore field-name case");
            }),
            valid("headers.no_space_after_colon", "HTTP/1.1 200 OK\r\nContent-Length:2\r\nX-Custom:value\r\n\r\nhi", "hi", 200, "GET", [](Client &client)
            {
                require(client.getResponseHeader("X-Custom") == "value", "Header without optional whitespace was lost");
            }),
            valid("headers.tab_after_colon", "HTTP/1.1 200 OK\r\nContent-Length:\t2\r\n\r\nhi", "hi"),
            valid("headers.length_optional_whitespace", "HTTP/1.1 200 OK\r\nContent-Length:  2 \t\r\n\r\nhi", "hi"),
            valid("headers.metadata_and_cookies", "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Type: text/plain; charset=utf-8\r\n"
                  "Set-Cookie: session=abc; Path=/; HttpOnly\r\nSet-Cookie: theme=dark; Path=/\r\nETag: \"v1\"\r\n"
                  "WWW-Authenticate: Basic realm=\"Private Area\"\r\nX-Trace: first\r\nX-Trace: second\r\n\r\nhi", "hi", 200, "GET", [](Client &client)
            {
                require(client.serverResponse.cookies.getCookieValueByName("session") == "abc", "Missing session cookie");
                require(client.serverResponse.cookies.getCookieValueByName("theme") == "dark", "Missing second Set-Cookie");
                require(client.getResponseETag().toString() == "\"v1\"", "Wrong ETag");
                require(client.getServerContentType() == "text/plain; charset=utf-8", "Wrong content type");
                require(client.serverResponse.sWWWAuthenticateRealm == "Private Area", "Wrong Basic realm");
                require(client.serverResponse.headers.getOptionsByName("X-Trace").size() == 2, "Repeated ordinary headers lost");
            }),
            valid("chunked.multiple_chunks", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhe\r\n3\r\nllo\r\n0\r\n\r\n", "hello"),
            valid("chunked.empty", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n", ""),
            valid("chunked.hex_uppercase", "HTTP/1.1 200 OK\r\nTransfer-Encoding: ChUnKeD\r\n\r\nA\r\n0123456789\r\n0\r\n\r\n", "0123456789"),
            valid("chunked.extensions", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2;name=value\r\nhi\r\n0;end=yes\r\n\r\n", "hi"),
            valid("chunked.precedes_content_length", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Length: 999\r\n\r\n2\r\nhi\r\n0\r\n\r\n", "hi"),
            {"client.chunked.raw_trailers", [](const auto &parts)
             {
                 Client client;
                 auto trailers = std::make_shared<StreamableString>();
                 client.serverResponse.content.setTrailerBytesContent(trailers);
                 const std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\nX-End: yes\r\nX-Sum: 42\r\n\r\n";
                 completed(client, feed(client, raw, parts), raw.size());
                 require(client.serverResponse.content.getStreamableObject()->toString() == "hi", "Trailers entered body");
                 require(trailers->getValue() == "X-End: yes\r\nX-Sum: 42\r\n\r\n" && trailers->writeStatus.finished, "Wrong raw trailers or missing EOF");
                 require(!client.serverResponse.headers.exist("X-End"), "Trailers entered initial headers");
             }},
            {"client.fixed_length.leaves_next_message", [](const auto &parts)
             {
                 Client client;
                 const auto first = response("hi");
                 completed(client, feed(client, first + response("next"), parts), first.size());
                 require(client.serverResponse.content.getStreamableObject()->toString() == "hi", "Consumed next response body");
             }},
            {"client.chunked.leaves_next_message", [](const auto &parts)
             {
                 Client client;
                 const std::string first = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n";
                 completed(client, feed(client, first + response("next"), parts), first.size());
             }},
            {"client.callback.reject_headers", [](const auto &parts)
             {
                 Client client;
                 client.acceptHeaders = false;
                 rejected(client, feed(client, response("hello"), parts));
                 require(client.headerCalls == 1 && client.serverResponse.content.getStreamableObject()->size() == 0,
                         "Headers rejection must happen before body delivery");
             }},
            {"client.destination.custom", [](const auto &parts)
             {
                 Client client;
                 auto destination = std::make_shared<StreamableString>();
                 client.serverResponse.content.setStreamableObject(destination);
                 const auto raw = response("hello");
                 completed(client, feed(client, raw, parts), raw.size());
                 require(client.serverResponse.content.getStreamableObject() == destination
                             && destination->getValue() == "hello" && destination->writeStatus.finished, "Custom destination lost data, identity or EOF");
             }},
            {"client.head.leaves_representation_bytes", [](const auto &parts)
             {
                 Client client;
                 client.clientRequest.requestLine.setHTTPMethod("HEAD");
                 const std::string headers = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n";
                 completed(client, feed(client, headers + "hello", parts), headers.size());
                 require(client.serverResponse.content.getStreamableObject()->size() == 0, "HEAD consumed body bytes");
             }}
        };
    }
    if (suite == "client_invalid")
    {
        std::vector<RawCase> cases;
        const std::vector<std::pair<std::string, std::string>> lines = {
            {"empty_status", ""}, {"one_token_status", "HTTP/1.1"}, {"nonnumeric_status", "HTTP/1.1 abc Bad"},
            {"partially_numeric_status", "HTTP/1.1 200x Bad"}, {"short_status", "HTTP/1.1 20 Bad"},
            {"out_of_range_status", "HTTP/1.1 600 Bad"}, {"negative_status", "HTTP/1.1 -200 Bad"},
            {"bad_protocol", "FTP/1.1 200 OK"}, {"bad_version", "HTTP/x.y 200 OK"}
        };
        for (const auto &line : lines)
        {
            cases.push_back(invalid("status." + line.first, line.second + "\r\nContent-Length: 0\r\n\r\n"));
        }
        const std::vector<std::pair<std::string, std::string>> lengths = {
            {"empty", ""}, {"text", "bad"}, {"negative", "-1"}, {"plus_sign", "+1"}, {"hex", "0x1"},
            {"suffix", "1x"}, {"comma_list", "1, 2"}, {"overflow", "184467440737095516160"}
        };
        for (const auto &length : lengths)
        {
            cases.push_back(invalid("content_length." + length.first, "HTTP/1.1 200 OK\r\nContent-Length: " + length.second + "\r\n\r\nx"));
        }
        const std::vector<std::pair<std::string, std::string>> framing = {
            {"conflicting_lengths", "Content-Length: 1\r\nContent-Length: 2"},
            // The current client policy rejects even identical duplicate lengths.
            {"duplicate_lengths", "Content-Length: 1\r\ncontent-length: 1"},
            {"unsupported_coding", "Transfer-Encoding: gzip"},
            {"chunked_not_final", "Transfer-Encoding: chunked, gzip"},
            {"duplicate_transfer_encoding", "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked"}
        };
        for (const auto &field : framing)
        {
            cases.push_back(invalid("framing." + field.first, "HTTP/1.1 200 OK\r\n" + field.second + "\r\n\r\nx"));
        }
        const std::vector<std::pair<std::string, std::string>> chunks = {
            {"nonhex", "z\r\nhi\r\n0\r\n\r\n"}, {"negative", "-1\r\nx\r\n0\r\n\r\n"},
            {"overflow", "100000000\r\nx\r\n0\r\n\r\n"}, {"invalid_suffix", "2x\r\nhi\r\n0\r\n\r\n"},
            {"empty_size", "\r\nhi\r\n0\r\n\r\n"}, {"bad_data_crlf", "2\r\nhiXX0\r\n\r\n"},
            {"truncated_data", "5\r\nhi"}, {"truncated_data_crlf", "2\r\nhi\r"},
            {"missing_zero", "2\r\nhi\r\n"}, {"missing_trailer_terminator", "2\r\nhi\r\n0\r\nX-End: yes\r\n"}
        };
        for (const auto &chunk : chunks)
        {
            cases.push_back(invalid("chunked." + chunk.first, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + chunk.second));
        }
        cases.push_back(invalid("truncated.fixed_body", "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nshort"));
        cases.push_back(invalid("truncated.header_terminator", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n"));
        cases.push_back(invalid("truncated.informational_only", "HTTP/1.1 100 Continue\r\n\r\n"));
        for (const auto &field : {"Broken header", "Bad Name: value", "Content-Length : 0"})
        {
            cases.push_back(invalid("headers." + std::string(field), "HTTP/1.1 200 OK\r\n" + std::string(field) + "\r\nContent-Length: 0\r\n\r\n"));
        }
        cases.push_back(invalid("headers.embedded_nul", "HTTP/1.1 200 OK\r\nX-Test: " + std::string("a\0b", 3) + "\r\nContent-Length: 0\r\n\r\n"));
        return cases;
    }
    if (suite == "client_limits")
    {
        std::vector<RawCase> cases;
        for (size_t size : {size_t(4), size_t(5), size_t(6)})
        {
            for (const std::string mode : {"fixed", "chunked", "close"})
            {
                cases.push_back({"client.limit.body_" + mode + ".size_" + std::to_string(size), [=](const auto &parts)
                {
                    Client client;
                    client.serverResponse.content.setSecurityMaxPostDataSize(5);
                    const std::string body(size, 'x');
                    const std::string raw = mode == "fixed" ? response(body) : mode == "close" ? "HTTP/1.0 200 OK\r\n\r\n" + body
                        : "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + std::to_string(size) + "\r\n" + body + "\r\n0\r\n\r\n";
                    const auto result = feed(client, raw, parts);
                    if (size > 5)
                    {
                        rejected(client, result);
                        require(client.serverResponse.content.getStreamableObject()->size() <= 5, "Body limit exceeded in destination");
                    }
                    else
                    {
                        completed(client, result, raw.size());
                        require(client.serverResponse.content.getStreamableObject()->toString() == body, "Body at limit corrupted");
                    }
                }});
            }
        }
        cases.push_back({"client.limit.chunk_size", [](const auto &parts)
        {
            Client client;
            client.serverResponse.content.setSecurityMaxHttpChunkSize(2);
            rejected(client, feed(client, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n", parts));
        }});
        cases.push_back({"client.limit.chunk_cumulative_size", [](const auto &parts)
        {
            Client client;
            client.serverResponse.content.setSecurityMaxPostDataSize(3);
            rejected(client, feed(client, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nab\r\n2\r\ncd\r\n0\r\n\r\n", parts));
        }});
        cases.push_back({"client.limit.header_line", [](const auto &parts)
        {
            Client client;
            client.serverResponse.headers.setMaxOptionSize(32);
            rejected(client, feed(client, "HTTP/1.1 200 OK\r\nX-Long: " + std::string(40, 'x') + "\r\n\r\n", parts));
        }});
        cases.push_back({"client.limit.header_count", [](const auto &parts)
        {
            Client client;
            client.serverResponse.headers.setMaxOptions(1);
            rejected(client, feed(client, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nX-Extra: value\r\n\r\n", parts));
        }});
        for (const bool failOnEOF : {false, true})
        {
            cases.push_back({std::string("client.destination.failure_") + (failOnEOF ? "eof" : "data"), [=](const auto &parts)
            {
                Client client;
                client.serverResponse.content.setStreamableObject(std::make_shared<FailingSink>(failOnEOF));
                const auto result = feed(client, response("hello"), parts);
                require(result.rejected && client.getParseResult() == Parser::ParseResult::ERR_STREAM_WRITE,
                        "Destination failure did not propagate as a stream write error");
            }});
        }
        return cases;
    }
    return {};
}

} // namespace HTTPParserTests
