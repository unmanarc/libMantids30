#include "test_support.h"

namespace HTTPParserTests {
namespace {

using Inspect = std::function<void(Server &)>;

RawCase valid(const std::string &name, const std::string &raw, Inspect inspect = {})
{
    return {"server." + name, [=](const auto &parts)
    {
        Server server;
        server.inspectContent = inspect;
        const auto result = feed(server, raw, parts);
        completed(server, result, raw.size());
        require(result.finishedBeforeEOF, "Request must complete without waiting for connection EOF");
        require(server.contentCalls == 1 && server.events == "UHC", "Wrong callback count or order");
        require(server.response().find(" 200 OK\r\n") != std::string::npos, "Missing successful HTTP response");
    }};
}

RawCase invalid(const std::string &name, const std::string &raw, unsigned status = 0)
{
    return {"server." + name, [=](const auto &parts)
    {
        Server server;
        rejected(server, feed(server, raw, parts), status);
    }};
}

void bodyEquals(Server &server, const std::string &expected)
{
    const auto body = server.clientRequest.content.getStreamableObject();
    require(body->toString() == expected, "Wrong request body");
    if (!expected.empty())
    {
        require(body->writeStatus.finished && body->writeStatus.succeed, "Request body destination did not finish successfully");
    }
}

void varEquals(const std::shared_ptr<Mantids30::Core::Memory::Abstract::Vars> &vars,
               const std::string &name, const std::string &expected)
{
    const auto value = vars->getValue(name);
    require(value && value->toString() == expected, "Wrong decoded variable: " + name);
}

} // namespace

std::vector<RawCase> serverCases(const std::string &suite)
{
    if (suite == "server_valid")
    {
        std::vector<RawCase> cases = {
            valid("get.http_1_1", "GET /hello HTTP/1.1\r\nHost: example.test\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.getURI() == "/hello", "Wrong URI");
                require(server.clientRequest.requestLine.getHTTPMethod() == "GET", "Wrong method");
                require(server.clientRequest.requestLine.getHTTPVersion()->toString() == "HTTP/1.1", "Wrong version");
                require(server.clientRequest.virtualHost == "example.test" && server.clientRequest.virtualPort == 80, "Wrong host or default port");
                bodyEquals(server, "");
            }),
            valid("get.http_1_0_without_host", "GET / HTTP/1.0\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.virtualHost.empty(), "Invented Host for HTTP/1.0");
                require(server.clientRequest.requestLine.getHTTPVersion()->toString() == "HTTP/1.0", "Wrong version");
            }),
            valid("head", "HEAD / HTTP/1.1\r\nHost: example.test\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.requestLine.getHTTPMethod() == "HEAD", "HEAD method lost");
            }),
            valid("options.asterisk", "OPTIONS * HTTP/1.1\r\nHost: example.test\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.requestLine.getHTTPMethod() == "OPTIONS" && server.clientRequest.getURI() == "*", "Wrong OPTIONS target");
            }),
            valid("headers.case_insensitive", "GET / HTTP/1.1\r\nhOsT: example.test:8080\r\nuSeR-aGeNt: RawParser/1.0\r\nX-Trace: first\r\nx-trace: second\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.virtualHost == "example.test" && server.clientRequest.virtualPort == 8080, "Wrong explicit host port");
                require(server.clientRequest.userAgent == "RawParser/1.0", "Wrong User-Agent");
                require(server.clientRequest.headers.getOptionsByName("X-TRACE").size() == 2, "Repeated ordinary headers lost");
            }),
            valid("headers.no_space_after_colon", "GET / HTTP/1.1\r\nHost:example.test\r\n\r\n"),
            valid("headers.tab_after_colon", "GET / HTTP/1.1\r\nHost:\texample.test\r\n\r\n"),
            valid("headers.ipv6_host", "GET / HTTP/1.1\r\nHost: [::1]:8080\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.virtualHost == "[::1]" && server.clientRequest.virtualPort == 8080, "Wrong bracketed IPv6 host or port");
            }),
            valid("query.percent_plus_repeated_empty", "GET /hello%20world?name=Ada+Lovelace&tag=one&tag=two&empty=&encoded=a%26b%3Dc HTTP/1.1\r\nHost: example.test\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.getURI() == "/hello world", "Path was not percent decoded");
                const auto vars = server.clientRequest.requestLine.urlVars();
                varEquals(vars, "name", "Ada Lovelace");
                varEquals(vars, "encoded", "a&b=c");
                varEquals(vars, "empty", "");
                require(vars->varCount("tag") == 2, "Repeated query variables lost");
                const auto tags = vars->getValues("tag");
                require(tags.front()->toString() == "one" && tags.back()->toString() == "two", "Wrong repeated query values");
            }),
            valid("headers.cookies_basic_auth", "GET / HTTP/1.1\r\nHost: example.test\r\nCookie: session=abc; theme=dark\r\nAuthorization: Basic dXNlcjpwYXNzOndvcmQ=\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.getCookie("session") == "abc" && server.clientRequest.getCookie("theme") == "dark", "Wrong cookies");
                require(server.clientRequest.basicAuth.isEnabled && server.clientRequest.basicAuth.username == "user"
                            && server.clientRequest.basicAuth.password == "pass:word", "Wrong Basic credentials");
            }),
            valid("headers.bearer_auth", "GET / HTTP/1.1\r\nHost: example.test\r\nAuthorization: Bearer token.value\r\n\r\n", [](Server &server)
            {
                require(server.clientRequest.getAuthorizationBearer() == "token.value", "Wrong bearer token");
                require(!server.clientRequest.basicAuth.isEnabled, "Bearer treated as Basic authentication");
            }),
            valid("body.fixed_length", request("hello"), [](Server &server) { bodyEquals(server, "hello"); }),
            valid("body.binary", request(std::string("a\0b\xff\r\n", 6)), [](Server &server) { bodyEquals(server, std::string("a\0b\xff\r\n", 6)); }),
            valid("body.empty_post", request(""), [](Server &server) { bodyEquals(server, ""); }),
            valid("body.length_optional_whitespace", "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length:  2 \t\r\n\r\nhi", [](Server &server) { bodyEquals(server, "hi"); }),
            valid("body.json", request("{\"name\":\"Ada\",\"count\":2,\"ok\":true}", "application/json; charset=utf-8"), [](Server &server)
            {
                require(server.clientRequest.content.getContainerType() == Content::ContainerType::JSON, "Wrong JSON container");
                const auto json = server.clientRequest.content.getJSONVars()->getValue();
                require(json && json->is_object(), "JSON body did not parse");
                const auto &object = json->as_object();
                require(object.at("name").as_string() == "Ada" && object.at("count").as_int64() == 2 && object.at("ok").as_bool(), "Wrong JSON values");
            }),
            valid("body.url_form", request("name=Ada+Lovelace&encoded=a%26b%3Dc&empty=&tag=one&tag=two", "application/x-www-form-urlencoded"), [](Server &server)
            {
                require(server.clientRequest.content.getContainerType() == Content::ContainerType::URL, "Wrong URL form container");
                const auto vars = server.clientRequest.content.postVars();
                varEquals(vars, "name", "Ada Lovelace");
                varEquals(vars, "encoded", "a&b=c");
                varEquals(vars, "empty", "");
                require(vars->varCount("tag") == 2, "Repeated form fields lost");
            }),
            valid("body.chunked", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhe\r\n3\r\nllo\r\n0\r\n\r\n", [](Server &server)
            {
                bodyEquals(server, "hello");
            }),
            {"server.message_boundary", [](const auto &parts)
             {
                 Server server;
                 const auto first = request("hi");
                 completed(server, feed(server, first + request("next"), parts), first.size());
                 bodyEquals(server, "hi");
                 require(server.contentCalls == 1, "Parsed a second request after connection close");
             }},
            {"server.callback.stop_after_uri", [](const auto &parts)
             {
                 Server server;
                 server.acceptURI = false;
                 const std::string line = "GET / HTTP/1.1\r\n";
                 completed(server, feed(server, line + "Host: example.test\r\n\r\n", parts), line.size());
                 require(server.events == "U" && server.response().empty(), "URI callback did not stop parsing");
             }},
            {"server.callback.stop_after_headers", [](const auto &parts)
             {
                 Server server;
                 server.acceptHeaders = false;
                 const std::string headers = "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length: 5\r\n\r\n";
                 completed(server, feed(server, headers + "hello", parts), headers.size());
                 require(server.events == "UH" && server.contentCalls == 0, "Headers callback did not stop before body");
             }}
        };
        for (const std::string method : {"PUT", "PATCH", "DELETE"})
        {
            const std::string raw = method + " /resource HTTP/1.1\r\nHost: example.test\r\nContent-Length: 2\r\n\r\nhi";
            cases.push_back(valid("method." + method, raw, [method](Server &server)
            {
                require(server.clientRequest.requestLine.getHTTPMethod() == method, "Wrong request method");
                bodyEquals(server, "hi");
            }));
        }
        const std::string multipart = "--raw-boundary\r\nContent-Disposition: form-data; name=\"name\"\r\n\r\nAda\r\n"
            "--raw-boundary\r\nContent-Disposition: form-data; name=\"file\"; filename=\"sample.bin\"\r\n"
            "Content-Type: application/octet-stream\r\n\r\n" + std::string("a\0b", 3) + "\r\n--raw-boundary--\r\n";
        cases.push_back(valid("body.multipart_file", request(multipart, "multipart/form-data; boundary=\"raw-boundary\""), [](Server &server)
        {
            require(server.clientRequest.content.getContainerType() == Content::ContainerType::MIME, "Wrong multipart container");
            const auto mime = server.clientRequest.content.getMultiPartVars();
            varEquals(mime, "name", "Ada");
            varEquals(mime, "file", std::string("a\0b", 3));
            const auto file = mime->getFirstMessageByName("file");
            require(file && file->getHeader()->getOptionByName("Content-Disposition")->getSubVar("filename") == "sample.bin", "Wrong upload filename");
        }));
        return cases;
    }
    if (suite == "server_invalid")
    {
        std::vector<RawCase> cases = {
            invalid("host.missing", "GET / HTTP/1.1\r\n\r\n", 400),
            invalid("host.empty", "GET / HTTP/1.1\r\nHost: \r\n\r\n", 400),
            invalid("host.duplicate", "GET / HTTP/1.1\r\nHost: first.test\r\nHost: second.test\r\n\r\n", 400),
            invalid("host.invalid_port", "GET / HTTP/1.1\r\nHost: example.test:bad\r\n\r\n", 400),
            invalid("version.unsupported_major", "GET / HTTP/2.0\r\nHost: example.test\r\n\r\n", 505),
            invalid("request_line.one_token", "GET\r\n\r\n"),
            invalid("request_line.empty", "\r\n\r\n"),
            invalid("request_line.bad_protocol", "GET / FTP/1.1\r\nHost: example.test\r\n\r\n"),
            invalid("request_line.bad_version", "GET / HTTP/x.y\r\nHost: example.test\r\n\r\n"),
            invalid("request_line.extra_token", "GET / HTTP/1.1 extra\r\nHost: example.test\r\n\r\n"),
            invalid("truncated.body", "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length: 10\r\n\r\nshort"),
            invalid("truncated.headers", "GET / HTTP/1.1\r\nHost: example.test\r\n"),
            invalid("framing.conflicting_content_lengths", "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nxx"),
            // Requests with both framing mechanisms must be rejected before dispatch.
            invalid("framing.transfer_encoding_with_content_length", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: chunked\r\nContent-Length: 1\r\n\r\nx"),
            invalid("framing.unsupported_transfer_encoding", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: gzip\r\n\r\nx"),
            invalid("framing.duplicate_transfer_encoding", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n"),
            invalid("chunked.invalid_size", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: chunked\r\n\r\nz\r\nhi\r\n0\r\n\r\n"),
            invalid("chunked.truncated", "POST / HTTP/1.1\r\nHost: example.test\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhi")
        };
        for (const auto &length : {"", "bad", "-1", "+1", "1x", "0x1", "1, 2", "184467440737095516160"})
        {
            cases.push_back(invalid("content_length." + std::string(*length ? length : "empty"), "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length: " + std::string(length) + "\r\n\r\nx"));
        }
        for (const auto &field : {"Broken header", "Bad Name: value", "Content-Length : 0"})
        {
            cases.push_back(invalid("headers." + std::string(field), "GET / HTTP/1.1\r\nHost: example.test\r\n" + std::string(field) + "\r\n\r\n"));
        }
        cases.push_back(invalid("headers.embedded_nul", "GET / HTTP/1.1\r\nHost: example.test\r\nX-Test: " + std::string("a\0b", 3) + "\r\n\r\n"));
        return cases;
    }
    if (suite == "server_limits")
    {
        std::vector<RawCase> cases;
        for (size_t size : {size_t(4), size_t(5), size_t(6)})
        {
            cases.push_back({"server.limit.body.size_" + std::to_string(size), [=](const auto &parts)
            {
                Server server;
                server.clientRequest.content.setSecurityMaxPostDataSize(5);
                const std::string body(size, 'x');
                const auto raw = request(body);
                const auto result = feed(server, raw, parts);
                if (size > 5)
                {
                    rejected(server, result, 413);
                    require(server.clientRequest.content.getStreamableObject()->size() == 0, "Oversized body was read");
                }
                else
                {
                    completed(server, result, raw.size());
                    bodyEquals(server, body);
                }
            }});
        }
        cases.push_back({"server.limit.request_line", [](const auto &parts)
        {
            Server server;
            server.clientRequest.requestLine.setSecurityMaxURLSize(32);
            rejected(server, feed(server, "GET /" + std::string(40, 'x') + " HTTP/1.1\r\nHost: example.test\r\n\r\n", parts));
        }});
        cases.push_back({"server.limit.header_line", [](const auto &parts)
        {
            Server server;
            server.clientRequest.headers.setMaxOptionSize(32);
            rejected(server, feed(server, "GET / HTTP/1.1\r\nHost: example.test\r\nX-Long: " + std::string(40, 'x') + "\r\n\r\n", parts));
        }});
        cases.push_back({"server.limit.header_count", [](const auto &parts)
        {
            Server server;
            server.clientRequest.headers.setMaxOptions(1);
            rejected(server, feed(server, "GET / HTTP/1.1\r\nHost: example.test\r\nX-Extra: value\r\n\r\n", parts));
        }});
        for (const bool failOnEOF : {false, true})
        {
            cases.push_back({std::string("server.destination.failure_") + (failOnEOF ? "eof" : "data"), [=](const auto &parts)
            {
                Server server;
                server.clientRequest.content.setStreamableObject(std::make_shared<FailingSink>(failOnEOF));
                const auto result = feed(server, request("hello"), parts);
                require(result.rejected && server.getParseResult() == Parser::ParseResult::ERR_STREAM_WRITE,
                        "Destination failure did not propagate as a stream write error");
                require(server.contentCalls == 0, "Application received a request with a failed destination");
            }});
        }
        return cases;
    }
    return {};
}

} // namespace HTTPParserTests
