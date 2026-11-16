#include <Mantids30/Core/Helpers/mem.h>
#include <Mantids30/Core/Memory/subparser.h>
#include "test_support.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

using Mantids30::Core::Helpers::Mem;
using Mantids30::Core::Memory::Streams::SubParser;

namespace {

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

unsigned char foldASCII(unsigned char c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

class DelimitedParser : public SubParser
{
public:
    explicit DelimitedParser(const std::string &delimiter)
    {
        setParseStrategy(ParseStrategy::DIRECT_DELIMITER);
        setParseDelimiter(delimiter);
    }

    std::string output;

protected:
    ParseResult parse() override
    {
        const auto data = getParsedBuffer()->toString();
        require(data.has_value(), "Parsed buffer readable");
        output += *data;
        return getFoundDelimiter().empty() ? ParseResult::GET_MORE_DATA : ParseResult::GOTO_NEXT_SUBPARSER;
    }
};

void testComparisons()
{
    for (unsigned int a = 0; a < 256; ++a)
    {
        for (unsigned int b = 0; b < 256; ++b)
        {
            const unsigned char left = a, right = b;
            const bool equal = foldASCII(left) == foldASCII(right);
            require(Mem::icharcmp(left, right) == equal, "ASCII character comparison changed");
            require((Mem::memicmp2(&left, &right, 1, false) == 0) == equal, "ASCII buffer comparison changed");
            require((Mem::memicmp2(&left, &right, 1, true) == 0) == (a == b), "Sensitive comparison changed");
        }
    }
    const char left[] = {'A', '\0', 'B', static_cast<char>(0xff)};
    const char equal[] = {'a', '\0', 'b', static_cast<char>(0xff)};
    const char different[] = {'a', '\0', 'c', static_cast<char>(0xff)};
    require(Mem::memicmp2(left, equal, sizeof(left), false) == 0, "Binary comparison must fold ASCII");
    require(Mem::memicmp2(left, different, sizeof(left), false) != 0, "NUL must not end comparison");
    require(Mem::memicmp2(nullptr, nullptr, 0, false) == 0, "Empty insensitive comparison");
    require(Mem::memcmp64(nullptr, nullptr, 0) == 0, "Empty sensitive comparison");
}

void testCopies()
{
    for (size_t size : {size_t(0), size_t(1), size_t(65535), size_t(65536), size_t(65537), size_t(131079)})
    {
        std::vector<unsigned char> source(size + 2, 0);
        for (size_t i = 0; i < source.size(); ++i)
        {
            source[i] = static_cast<unsigned char>((i * 31 + i / 251) % 256);
        }
        auto actual = source;
        auto expected = source;
        require(Mem::memcpy64(actual.data() + 1, source.data(), size) == actual.data() + 1, "Copy return value");
        std::memcpy(expected.data() + 1, source.data(), size);
        require(actual == expected, "Copy across former block boundary");
        require(Mem::memcmp64(actual.data(), expected.data(), actual.size()) == 0, "Large equal comparison");
        actual.back() ^= 1;
        require(Mem::memcmp64(actual.data(), expected.data(), actual.size()) != 0, "Large unequal comparison");

        for (bool forward : {false, true})
        {
            actual = expected = source;
            const size_t destination = forward ? 0 : 1;
            const size_t origin = forward ? 1 : 0;
            require(Mem::memmove64(actual.data() + destination, actual.data() + origin, size) == actual.data() + destination,
                    "Move return value");
            std::memmove(expected.data() + destination, expected.data() + origin, size);
            require(actual == expected, "Overlapping move across former block boundary");
            Mem::memmove64(actual.data(), actual.data(), actual.size());
            require(actual == expected, "Move to same address");
        }
    }
    require(Mem::memcpy64(nullptr, nullptr, 0) == nullptr, "Empty copy");
    require(Mem::memmove64(nullptr, nullptr, 0) == nullptr, "Empty move");
}

void testFragmentedDelimiter()
{
    for (const std::string &delimiter : {std::string("abab"), std::string("\r\n"), std::string("\0X", 2), std::string("!")})
    {
        const std::string payload = "body" + delimiter.substr(0, delimiter.size() - 1) + "mismatch";
        const std::string input = payload + delimiter;
        for (size_t split = 1; split < input.size(); ++split)
        {
            DelimitedParser parser(delimiter);
            require(parser.writeIntoSubParser(input.data(), split) == split, "First fragment consumed");
            require(parser.writeIntoSubParser(input.data() + split, input.size() - split) == input.size() - split,
                    "Second fragment consumed");
            require(parser.output == payload, "Fragmented delimiter payload");
            require(parser.getFoundDelimiter() == delimiter, "Fragmented delimiter found");
        }
    }
}

} // namespace

TEST("helpers", "exhaustive_ascii_comparisons")
{
    testComparisons();
}
TEST("helpers", "copy_and_overlapping_move_boundaries")
{
    testCopies();
}
TEST("parsers", "direct_delimiter_every_two_fragment_split")
{
    testFragmentedDelimiter();
}
