#include "test_support.h"
#include <Mantids30/Core/Memory/parser.h>
#include <Mantids30/Core/Memory/streamable_string.h>

using namespace Mantids30::Core::Memory::Streams;
using namespace MemoryTests;

namespace {
// Capture parser callbacks so tests can compare consumed input and delivered payload.
class Probe : public SubParser
{
public:
    explicit Probe(ParseStrategy strategy, size_t limit = 1024, const std::string &delimiter = "\r\n")
    {
        setParseStrategy(strategy);
        setParseDataTargetSize(limit);
        setParseDelimiter(delimiter);
    }
    void delimiters(const std::list<std::string> &values)
    {
        setParseMultiDelimiter(values);
    }
    std::vector<std::string> pieces;
    std::string output;
    ParseResult result = ParseResult::GOTO_NEXT_SUBPARSER;

protected:
    ParseResult parse() override
    {
        auto data = getParsedBuffer()->toString();
        CHECK(data.has_value());
        pieces.push_back(*data);
        output += *data;
        return result;
    }
};

// Exercise transitions from a delimited header to a fixed-size body.
class Protocol : public Parser
{
public:
    explicit Protocol(std::shared_ptr<StreamableObject> source = nullptr)
        : Parser(source, true), header(SubParser::ParseStrategy::DELIMITER), body(SubParser::ParseStrategy::SIZE, 3)
    {
    }
    Probe header, body;
    bool initialize = true, change = true, ended = false;
    bool initProtocol() override
    {
        initSubParser(&header);
        initSubParser(&body);
        m_currentSubParser = &header;
        return initialize;
    }
    void endProtocol() override
    {
        ended = true;
    }
    bool changeToNextParser() override
    {
        if (!change)
        {
            return false;
        }
        m_currentSubParser = m_currentSubParser == &header ? &body : nullptr;
        return true;
    }
};
}

TEST("parsers", "buffered_delimiter_every_fragment_size_and_unconsumed_tail")
{
    for (const auto &delimiter : {std::string("\r\n"), std::string("abab"), std::string("\0X", 2)})
    {
        const std::string payload = "body", input = payload + delimiter + "TAIL";
        for (size_t block = 1; block <= input.size(); ++block)
        {
            Probe parser(SubParser::ParseStrategy::DELIMITER, 1024, delimiter);
            size_t consumed = 0;
            while (parser.pieces.empty())
            {
                const size_t count = std::min(block, input.size() - consumed);
                CHECK(count != 0);
                auto result = parser.writeIntoSubParser(input.data() + consumed, count);
                CHECK(result && *result <= count);
                consumed += *result;
            }
            CHECK(consumed == payload.size() + delimiter.size());
            CHECK(parser.output == payload);
            CHECK(parser.getParseStatus() == SubParser::ParseResult::GOTO_NEXT_SUBPARSER);
        }
    }
}

TEST("parsers", "direct_delimiter_bytewise_overlap_binary_and_case_sensitivity")
{
    for (const auto &delimiter : {std::string("abab"), std::string("aaaaab"), std::string("END"), std::string("\0X", 2)})
    {
        const std::string payload = std::string("a\0b", 3) + delimiter.substr(0, delimiter.size() - 1) + "!end!";
        const std::string input = payload + delimiter + "tail";
        Probe parser(SubParser::ParseStrategy::DIRECT_DELIMITER, 1024, delimiter);
        parser.result = SubParser::ParseResult::GET_MORE_DATA;
        size_t consumed = 0;
        while (parser.getFoundDelimiter().empty())
        {
            CHECK(consumed < input.size());
            CHECK(parser.writeIntoSubParser(input.data() + consumed, 1) == 1);
            ++consumed;
        }
        CHECK(parser.output == payload);
        CHECK(consumed == payload.size() + delimiter.size());
    }
}

TEST("parsers", "size_strategy_waits_and_consumes_only_target_bytes")
{
    for (size_t split = 1; split < 6; ++split)
    {
        Probe parser(SubParser::ParseStrategy::SIZE, 6);
        CHECK(parser.writeIntoSubParser("abcdef", split) == split);
        CHECK(parser.pieces.empty());
        CHECK(parser.writeIntoSubParser("abcdefTAIL" + split, 10 - split) == 6 - split);
        CHECK(parser.output == "abcdef");
        CHECK(parser.pieces.size() == 1);
    }
    Probe incomplete(SubParser::ParseStrategy::SIZE, 6);
    CHECK(incomplete.writeIntoSubParser("abc", 3) == 3);
    CHECK(incomplete.writeIntoSubParser(nullptr, 0) == 0);
    CHECK(incomplete.isStreamEnded());
    CHECK(incomplete.pieces.empty());
}

TEST("parsers", "connection_end_buffers_binary_and_parses_once_at_eof")
{
    Probe parser(SubParser::ParseStrategy::CONNECTION_END);
    CHECK(parser.writeIntoSubParser("ab", 2) == 2);
    CHECK(parser.writeIntoSubParser("\0c", 2) == 2);
    CHECK(parser.pieces.empty());
    CHECK(parser.writeIntoSubParser(nullptr, 0) == 0);
    CHECK(parser.output == std::string("ab\0c", 4));
    CHECK(parser.pieces.size() == 1);
    CHECK(parser.isStreamEnded());
}

TEST("parsers", "delimiter_eof_flushes_unterminated_payload")
{
    Probe parser(SubParser::ParseStrategy::DELIMITER);
    CHECK(parser.writeIntoSubParser("tail", 4) == 4);
    CHECK(parser.pieces.empty());
    CHECK(parser.writeIntoSubParser(nullptr, 0) == 0);
    CHECK(parser.output == "tail");
    CHECK(parser.isStreamEnded());
}

TEST("parsers", "buffer_limits_and_unimplemented_validator_fail_without_looping")
{
    Probe parser(SubParser::ParseStrategy::DELIMITER, 4);
    CHECK(parser.writeIntoSubParser("abcd", 4) == 4);
    CHECK(!parser.writeIntoSubParser("e", 1));
    CHECK(parser.pieces.empty());
    Probe validator(SubParser::ParseStrategy::VALIDATOR);
    CHECK(!validator.writeIntoSubParser("abc", 3));
}

TEST("parsers", "direct_strategy_budget_and_chunk_delivery")
{
    Probe parser(SubParser::ParseStrategy::DIRECT, 5);
    parser.result = SubParser::ParseResult::GET_MORE_DATA;
    CHECK(parser.writeIntoSubParser("abc", 3) == 3);
    CHECK(parser.output == "abc");
    CHECK(parser.writeIntoSubParser("defg", 4) == 2);
    CHECK(parser.output == "abcde");
    CHECK(!parser.writeIntoSubParser("f", 1));
}

TEST("parsers", "multiple_delimiters_fragmented_and_report_match")
{
    Probe parser(SubParser::ParseStrategy::MULTIDELIMITER);
    parser.delimiters({"\r\n", "!"});
    CHECK(parser.writeIntoSubParser("body\r", 5) == 5);
    CHECK(parser.pieces.empty());
    CHECK(parser.writeIntoSubParser("\nTAIL", 5) == 1);
    CHECK(parser.output == "body");
    CHECK(parser.getFoundDelimiter() == "\r\n");
}

TEST("parsers", "protocol_transitions_and_fragmented_input")
{
    const std::string input = "header\r\nabc";
    for (size_t split = 1; split < input.size(); ++split)
    {
        Protocol parser;
        CHECK(parser.initProtocol());
        CHECK(parser.write(input.data(), split) == split);
        CHECK(parser.write(input.data() + split, input.size() - split) == input.size() - split);
        CHECK(parser.header.output == "header");
        CHECK(parser.body.output == "abc");
        CHECK(parser.writeStatus.finished);
        CHECK(!parser.write("extra", 5));
    }
}

TEST("parsers", "protocol_initialization_errors_parse_errors_and_completion")
{
    auto source = std::make_shared<StreamableString>();
    *source = "header\r\nabc";
    Protocol parser(source);
    Parser::ParseResult result;
    parser.parseObject(&result);
    CHECK(result == Parser::ParseResult::SUCCEED);
    CHECK(parser.ended);
    CHECK(parser.body.output == "abc");
    Protocol failed(source);
    failed.initialize = false;
    failed.parseObject(&result);
    CHECK(result == Parser::ParseResult::ERR_INIT);
    CHECK(!failed.ended);
    Protocol invalid(source);
    invalid.header.result = SubParser::ParseResult::PARSE_ERROR;
    invalid.parseObject(&result);
    CHECK(result == Parser::ParseResult::ERR_PARSING);
    CHECK(invalid.ended);
    Protocol transition(source);
    transition.change = false;
    transition.parseObject(&result);
    CHECK(result == Parser::ParseResult::ERR_PARSING);
}

TEST("parsers", "protocol_pre_stream_and_recursion_budget")
{
    auto source = std::make_shared<StreamableString>();
    *source = "\nabc";
    auto prefix = std::make_shared<StreamableString>();
    *prefix = "header\r";
    Protocol parser(source);
    parser.setPreStreamableObject(prefix);
    Parser::ParseResult result;
    parser.parseObject(&result);
    CHECK(result == Parser::ParseResult::SUCCEED);
    CHECK(parser.header.output == "header");
    CHECK(parser.body.output == "abc");
    Protocol limited;
    CHECK(limited.initProtocol());
    limited.setMaxTTL(0);
    CHECK(!limited.write("header\r\nabc", 11));
}

TEST("parsers", "destination_write_errors_propagate_from_subparsers")
{
    auto source = std::make_shared<StreamableString>();
    *source = "header\r\nabc";
    for (const bool failInHeader : {true, false})
    {
        Protocol parser(source);
        (failInHeader ? parser.header : parser.body).result = SubParser::ParseResult::ERROR_STREAM_WRITE;
        Parser::ParseResult result;
        parser.parseObject(&result);
        CHECK(result == Parser::ParseResult::ERR_STREAM_WRITE);
        CHECK(parser.getParseResult() == result);
        CHECK(parser.ended);
    }

    Protocol direct;
    CHECK(direct.initProtocol());
    direct.header.result = SubParser::ParseResult::ERROR_STREAM_WRITE;
    CHECK(!direct.write("header\r\n", 8));
    CHECK(direct.getParseResult() == Parser::ParseResult::ERR_STREAM_WRITE);
}

TEST("parsers", "direct_delimiter_eof_flushes_partial_delimiter_as_payload")
{
    Probe parser(SubParser::ParseStrategy::DIRECT_DELIMITER, 1024, "END");
    parser.result = SubParser::ParseResult::GET_MORE_DATA;
    CHECK(parser.writeIntoSubParser("bodyEN", 6) == 6);
    CHECK(parser.output == "body");
    CHECK(parser.writeIntoSubParser(nullptr, 0) == 0);
    CHECK(parser.output == "bodyEN");
    CHECK(parser.getFoundDelimiter().empty());
    CHECK(parser.isStreamEnded());
}

TEST("parsers", "empty_delimiter_configuration_fails_without_zero_progress")
{
    for (auto strategy : {SubParser::ParseStrategy::DELIMITER, SubParser::ParseStrategy::DIRECT_DELIMITER})
    {
        Probe parser(strategy, 1024, "");
        CHECK(!parser.writeIntoSubParser("abc", 3));
        CHECK(parser.pieces.empty());
    }
}
