#include "test_support.h"

#include <iostream>

namespace HTTPParserTests {

std::vector<RawCase> streamingCases()
{
    return {
        {"client.every_split.fixed_length", [](const auto &)
         {
             const std::string raw = response(std::string("a\0b\r\nc", 6));
             for (size_t split = 1; split < raw.size(); ++split)
             {
                 Client client;
                 const auto result = feed(client, raw, {split, 0});
                 completed(client, result, raw.size());
                 require(client.serverResponse.content.getStreamableObject()->toString() == std::string("a\0b\r\nc", 6),
                         "Binary response corrupted at split " + std::to_string(split));
             }
         }, false},
        {"client.every_split.chunked", [](const auto &)
         {
             const std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhe\r\n3\r\nllo\r\n0\r\nX-End: yes\r\n\r\n";
             for (size_t split = 1; split < raw.size(); ++split)
             {
                 Client client;
                 completed(client, feed(client, raw, {split, 0}), raw.size());
                 require(client.serverResponse.content.getStreamableObject()->toString() == "hello", "Chunked body corrupted");
             }
         }, false},
        {"server.every_split.fixed_length", [](const auto &)
         {
             const std::string raw = request(std::string("a\0b\r\nc", 6));
             for (size_t split = 1; split < raw.size(); ++split)
             {
                 Server server;
                 completed(server, feed(server, raw, {split, 0}), raw.size());
                 require(server.events == "UHC", "Wrong server callback order");
                 require(server.clientRequest.content.getStreamableObject()->toString() == std::string("a\0b\r\nc", 6),
                         "Binary request corrupted at split " + std::to_string(split));
             }
         }, false},
        {"client.every_truncated_prefix.fixed_length", [](const auto &parts)
         {
             const std::string raw = response("hello");
             everyPrefix(raw, [&](const std::string &prefix)
             {
                 Client client;
                 rejected(client, feed(client, prefix, parts));
             });
         }},
        {"server.every_truncated_prefix.fixed_length", [](const auto &parts)
         {
             const std::string raw = request("hello");
             everyPrefix(raw, [&](const std::string &prefix)
             {
                 Server server;
                 rejected(server, feed(server, prefix, parts));
             });
         }},
        {"client.every_truncated_prefix.chunked", [](const auto &parts)
         {
             const std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\nX-End: yes\r\n\r\n";
             everyPrefix(raw, [&](const std::string &prefix)
             {
                 Client client;
                 rejected(client, feed(client, prefix, parts));
             });
         }},
        {"server.partial_body.waits_for_remaining_bytes", [](const auto &parts)
         {
             Server server;
             const auto raw = request("hello");
             const auto first = feed(server, raw.substr(0, raw.size() - 2), parts, false);
             require(!first.rejected && !server.writeStatus.finished && server.events == "UH", "Server completed a partial body");
             require(server.contentCalls == 0, "Application received a partial body");
             completed(server, feed(server, "lo", parts), 2);
             require(server.events == "UHC" && server.contentCalls == 1, "Server did not dispatch the completed request once");
         }},
        {"client.partial_body.waits_for_remaining_bytes", [](const auto &parts)
         {
             Client client;
             const auto raw = response("hello");
             const auto first = feed(client, raw.substr(0, raw.size() - 2), parts, false);
             require(!first.rejected && !client.writeStatus.finished && client.headerCalls == 1, "Client completed a partial body");
             completed(client, feed(client, "lo", parts), 2);
             require(client.serverResponse.content.getStreamableObject()->toString() == "hello", "Partial response body was lost");
         }},
        {"client.close_delimited.waits_for_eof", [](const auto &parts)
         {
             Client client;
             const auto result = feed(client, "HTTP/1.0 200 OK\r\n\r\nhello", parts, false);
             require(!result.rejected && !client.writeStatus.finished, "Close-delimited response completed before EOF");
             require(client.writeEOF() && client.writeStatus.finished, "Close-delimited response did not complete at EOF");
             require(client.serverResponse.content.getStreamableObject()->toString() == "hello", "Close-delimited body was lost");
         }},
        {"server.pipelined_requests.reset_state", [](const auto &parts)
         {
             Server server;
             server.setConnectionContinue(true);
             size_t requests = 0;
             server.inspectContent = [&](Server &current)
             {
                 ++requests;
                 if (requests == 1)
                 {
                     require(current.clientRequest.getURI() == "/first", "Wrong first URI");
                     require(current.clientRequest.getCookie("session") == "one", "Missing first cookie");
                 }
                 else
                 {
                     require(current.clientRequest.getURI() == "/second", "Wrong second URI");
                     require(current.clientRequest.getCookie("session").empty(), "Cookie leaked between requests");
                     current.setConnectionContinue(false);
                 }
             };
             const std::string raw = "GET /first HTTP/1.1\r\nHost: first.test\r\nCookie: session=one\r\n\r\n"
                                     "GET /second HTTP/1.1\r\nHost: second.test\r\nConnection: close\r\n\r\n";
             completed(server, feed(server, raw, parts), raw.size());
             require(requests == 2 && server.events == "UHCUHC", "Both pipelined requests must complete once");
         }}
    };
}

} // namespace HTTPParserTests

int main(int argc, char **argv)
{
    using namespace HTTPParserTests;
    if (argc < 2 || argc > 3)
    {
        std::cerr << "Usage: http_parser_test SUITE [CASE_SUBSTRING]\n";
        return 2;
    }
    const std::string suite = argv[1];
    const auto cases = suite == "streaming" ? streamingCases()
        : suite.rfind("client_", 0) == 0 ? clientCases(suite) : serverCases(suite);
    if (cases.empty())
    {
        std::cerr << "Unknown suite: " << suite << '\n';
        return 2;
    }

    const std::vector<std::pair<std::string, std::vector<size_t>>> schedules = {
        {"whole", {0}}, {"bytewise", {1}}, {"two_bytes", {2}}, {"seven_bytes", {7}}, {"irregular", {1, 13, 2, 3, 29, 5}}
    };
    size_t executed = 0, failed = 0;
    for (const auto &test : cases)
    {
        if (argc == 3 && test.name.find(argv[2]) == std::string::npos)
        {
            continue;
        }
        for (const auto &schedule : schedules)
        {
            if (!test.allSchedules && schedule.first != "whole")
            {
                continue;
            }
            ++executed;
            try
            {
                test.run(schedule.second);
            }
            catch (const std::exception &error)
            {
                ++failed;
                std::cerr << "FAIL " << test.name << " [" << schedule.first << "]: " << error.what() << '\n';
            }
        }
    }
    std::cout << suite << ": " << executed - failed << '/' << executed << " passed (" << failed << " failed)\n";
    return executed == 0 ? 2 : failed ? 1 : 0;
}
