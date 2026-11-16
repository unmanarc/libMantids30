#include "test_support.h"
#include <Mantids30/Core/Memory/b_chunks.h>
#include <Mantids30/Core/Memory/b_mem.h>
#include <Mantids30/Core/Memory/b_ref.h>
#include <Mantids30/Core/Memory/streamable_string.h>
#include <array>
#include <sstream>

using namespace Mantids30::Core::Memory::Containers;
using namespace MemoryTests;

namespace {
void contents(B_Base &buffer, const std::string &expected)
{
    CHECK(buffer.size() == expected.size());
    CHECK(buffer.isNull() == expected.empty());
    CHECK(buffer.toString() == expected);
    const auto copied = buffer.copyToBuffer();
    CHECK(std::string(copied.begin(), copied.end()) == expected);
    CHECK(buffer.compare(expected.data(), expected.size(), true));
}

void readContract(B_Base &buffer, const std::string &data)
{
    contents(buffer, data);
    for (size_t offset = 0; offset <= data.size(); ++offset)
    {
        for (size_t count : {size_t(0), size_t(1), size_t(3), data.size()})
        {
            std::vector<char> output(count + 2, '!');
            const auto result = buffer.copyOut(output.data() + 1, count, offset);
            if (count <= data.size() - offset)
            {
                CHECK(result == count);
                CHECK(std::string(output.data() + 1, count) == data.substr(offset, count));
                CHECK(buffer.toString(count, offset) == data.substr(offset, count));
            }
            else
            {
                CHECK(!result);
                CHECK(!buffer.toString(count, offset));
                CHECK(std::all_of(output.begin(), output.end(), [](char c)
                                  {
                                      return c == '!';
                                  }));
            }
            CHECK(output.front() == '!');
            CHECK(output.back() == '!');
        }
        std::ostringstream stream;
        CHECK(buffer.copyToStream(stream, unlimited, offset) == data.size() - offset);
        CHECK(stream.str() == data.substr(offset));
        Mantids30::Core::Memory::Streams::StreamableString destination;
        CHECK(buffer.appendTo(destination, unlimited, offset) == data.size() - offset);
        CHECK(destination.getValue() == data.substr(offset));
        CHECK(!destination.writeStatus.finished); // Copying must not send EOF.
    }
    char output = '!';
    CHECK(!buffer.copyOut(&output, 1, unlimited));
    CHECK(!buffer.toString(2, unlimited));
    CHECK(!buffer.toString(unlimited, data.size() + 1));
    CHECK(!buffer.compare("x", 1, true, unlimited));
    CHECK(!buffer.copyOut(&output, 1, data.size()));
    CHECK(output == '!');
}
}

TEST("containers", "empty_and_zero_length_operations")
{
    B_Chunks buffer;
    contents(buffer, "");
    CHECK(buffer.copyOut(nullptr, 0) == 0);
    CHECK(buffer.prepend(nullptr, 0) == 0);
    CHECK(!buffer.append(nullptr, 0)); // Existing append contract rejects empty input.
    CHECK(buffer.displace(0) == 0);
    CHECK(buffer.displace(unlimited) == 0);
    CHECK(!buffer.truncate(0));
    CHECK(!buffer.findChar('x'));
    CHECK(buffer.clear());
    CHECK(buffer.clear());
    CHECK(buffer.append("reused") == 6);
    contents(buffer, "reused");
}

TEST("containers", "binary_read_contract_across_storage_types")
{
    const std::string data = std::string("Ab\0cd\xffXYZ", 9);
    B_MEM memory(data.data(), data.size());
    readContract(memory, data);
    for (size_t chunkSize : {size_t(1), size_t(2), size_t(3), size_t(64)})
    {
        B_Chunks chunks;
        chunks.setMaxChunkSize(chunkSize);
        CHECK(chunks.append(data.data(), data.size()) == data.size());
        readContract(chunks, data);
        B_Ref reference;
        CHECK(reference.reference(&chunks, 1, 6));
        readContract(reference, data.substr(1, 6));
    }
}

TEST("containers", "large_binary_copy_boundaries")
{
    for (size_t size : {size_t(8191), size_t(8192), size_t(8193), size_t(65535), size_t(65536), size_t(65537), size_t(131079)})
    {
        const auto data = binaryData(size);
        B_Chunks buffer(data);
        contents(buffer, data);
        CHECK(buffer.toString(3, size - 3) == data.substr(size - 3));
        std::string target = "prefix:";
        CHECK(buffer.copyToString(target).has_value());
        CHECK(target == "prefix:" + data);
        B_Chunks copy;
        CHECK(buffer.appendTo(copy) == size);
        contents(copy, data);
    }
}

TEST("containers", "append_prepend_preserve_order_across_chunks")
{
    for (size_t chunkSize : {size_t(1), size_t(2), size_t(3), size_t(4)})
    {
        B_Chunks buffer("middle");
        buffer.setMaxChunkSize(chunkSize);
        CHECK(buffer.prepend("abcdef") == 6);
        CHECK(buffer.append("XYZ") == 3);
        contents(buffer, "abcdefmiddleXYZ");
        CHECK(buffer.displace(5) == 5);
        contents(buffer, "fmiddleXYZ");
        CHECK(buffer.truncate(4) == 4);
        contents(buffer, "fmid");
    }
}

TEST("containers", "truncate_and_displace_every_boundary")
{
    const std::string data = "0123456789abcdef";
    for (size_t chunkSize : {size_t(1), size_t(3), size_t(4), size_t(7)})
    {
        for (size_t count = 0; count <= data.size() + 1; ++count)
        {
            B_Chunks head, tail;
            head.setMaxChunkSize(chunkSize);
            tail.setMaxChunkSize(chunkSize);
            CHECK(head.append(data.data(), data.size()) == data.size());
            CHECK(tail.append(data.data(), data.size()) == data.size());
            CHECK(head.displace(count) == std::min(count, data.size()));
            contents(head, data.substr(std::min(count, data.size())));
            if (count < data.size())
            {
                CHECK(tail.truncate(count) == count);
            }
            else
            {
                CHECK(!tail.truncate(count));
            }
            contents(tail, data.substr(0, count));
        }
    }
}

TEST("containers", "capacity_overflow_and_rejection_are_non_destructive")
{
    B_Chunks buffer("abc");
    buffer.setMaxSize(5);
    CHECK(buffer.getSizeLeft() == 2);
    CHECK(buffer.append("de") == 2);
    CHECK(buffer.getSizeLeft() == 0);
    CHECK(!buffer.append("f"));
    CHECK(!buffer.prepend("f"));
    CHECK(!buffer.append("x", unlimited));
    CHECK(!buffer.prepend("x", unlimited));
    contents(buffer, "abcde");
    buffer.setMaxSize(2);
    CHECK(buffer.getSizeLeft() == 0);
    CHECK(!buffer.append("x"));
    buffer.reduceMaxSizeBy(2);
    CHECK(buffer.getMaxSize() == 0);
    throws<std::runtime_error>([&]
                               {
                                   buffer.reduceMaxSizeBy(1);
                               });
    CHECK(buffer.clear());
    CHECK(!buffer.append("x"));
}

TEST("containers", "chunk_limit_partial_writes_and_zero_configuration")
{
    B_Chunks buffer;
    buffer.setMaxChunkSize(2);
    buffer.setMaxChunks(2);
    CHECK(buffer.getMaxChunks() == 2);
    CHECK(buffer.append("abcdef") == 4);
    contents(buffer, "abcd");
    CHECK(buffer.append("e") == 0);
    CHECK(buffer.clear());
    CHECK(buffer.prepend("abcdef") == 4);
    contents(buffer, "abcd");
    CHECK(buffer.clear());
    buffer.setMaxChunkSize(0);
    CHECK(!buffer.append("x"));
    contents(buffer, "");
}

TEST("containers", "reference_views_are_relative_bounded_and_non_owning")
{
    B_Chunks source("prefixAbCtail");
    B_Ref view;
    CHECK(view.reference(&source, 6, 3));
    contents(view, "AbC");
    CHECK(view.findChar('A', 0, 0, true) == 0);
    CHECK(view.findChar('b', 0, 0, true) == 1);
    CHECK(!view.findChar('t'));
    CHECK(view.find("bC", 2) == 1);
    CHECK(!view.find("Ctail", 5));
    CHECK(!view.append("x"));
    CHECK(!view.prepend("x"));
    CHECK(view.displace(1) == 1);
    contents(view, "bC");
    CHECK(view.truncate(1) == 1);
    contents(view, "b");
    CHECK(view.clear());
    contents(source, "prefixAbCtail");
    CHECK(view.getReferencedBC() == nullptr);
    CHECK(!view.reference(&source, source.size() + 1));
    CHECK(!view.reference(nullptr));
}

TEST("containers", "reference_constructor_and_source_shrink")
{
    B_Chunks source("abcdef");
    B_Ref view(&source, 2);
    contents(view, "cdef");
    CHECK(source.truncate(1) == 1);
    contents(view, ""); // A shrinking source must not underflow the view size.
}

TEST("containers", "memory_reference_does_not_copy_or_modify_source")
{
    std::string source = "abcdef";
    B_MEM view(source.data(), source.size());
    source[1] = 'X';
    contents(view, "aXcdef");
    CHECK(!view.append("x"));
    CHECK(!view.prepend("x"));
    CHECK(view.displace(2) == 2);
    CHECK(view.truncate(2) == 2);
    contents(view, "cd");
    CHECK(view.clear());
    CHECK(source == "aXcdef");
    view.reference(source.data(), source.size());
    contents(view, source);
}

TEST("containers", "search_case_range_and_binary_contract")
{
    const std::string data("aBaba\0\xffZ", 8);
    B_MEM memory(data.data(), data.size());
    B_Chunks chunks;
    chunks.setMaxChunkSize(2);
    CHECK(chunks.append(data.data(), data.size()) == data.size());
    for (B_Base *buffer : {static_cast<B_Base *>(&memory), static_cast<B_Base *>(&chunks)})
    {
        CHECK(buffer->findChar('b', 0, 0, false) == 1);
        CHECK(buffer->findChar('b', 0, 0, true) == 3);
        CHECK(buffer->findChar(0, 0, 0, true) == 5);
        CHECK(buffer->findChar(255, 0, 0, true) == 6);
        CHECK(buffer->find("aba", 3, true) == 2);
        CHECK(buffer->find("AbA", 3, false) == 0);
        CHECK(!buffer->find("aba", 3, true, 2, 2));
        CHECK(!buffer->find("aba", 3, true, 0, 3)); // Candidate may not cross search end.
        CHECK(!buffer->find("x", 1, true, unlimited));
        CHECK(!buffer->find("x", 1, true, 1, unlimited));
        CHECK(!buffer->findChar('a', data.size()));
        CHECK(buffer->compare("ABABA", 5, false));
        CHECK(!buffer->compare("ABABA", 5, true));
    }
}

TEST("containers", "delimiter_copy_displace_and_destination_failure")
{
    for (bool remove : {false, true})
    {
        B_Chunks source("first::rest"), destination;
        CHECK(source.copyUntil(destination, "::", 2, 20, remove) == 0);
        contents(destination, remove ? "first" : "first::");
        contents(source, "first::rest");
        CHECK(destination.clear());
        CHECK(source.displaceUntil(destination, "::", 2, 20, remove) == 0);
        contents(source, "rest");
    }
    B_Chunks source("first::rest"), destination("prefix");
    CHECK(source.displaceUntil(destination, "::", 2, 20, true) == 0);
    contents(destination, "prefixfirst");
    contents(source, "rest");
    B_Chunks limited;
    limited.setMaxSize(1);
    B_Chunks unchanged("first::rest");
    CHECK(unchanged.displaceUntil(limited, "::", 2, 20, true) == -1);
    contents(unchanged, "first::rest");
    contents(limited, "");
    CHECK(unchanged.copyUntil(limited, "missing", 7, 20) == -1);
}

TEST("containers", "deterministic_operation_sequences_against_string")
{
    // Fixed seeds make every failing sequence reproducible.
    for (unsigned seed : {0u, 1u, 42u, 0xC0FFEEu})
    {
        std::mt19937 random(seed);
        B_Chunks buffer;
        buffer.setMaxChunkSize(7);
        std::string expected;
        for (size_t step = 0; step < 1500; ++step)
        {
            const size_t count = random() % 24;
            const std::string part = binaryData(count + 1);
            switch (random() % 5)
            {
            case 0:
                CHECK(buffer.append(part.data(), part.size()) == part.size());
                expected += part;
                break;
            case 1:
                CHECK(buffer.prepend(part.data(), part.size()) == part.size());
                expected.insert(0, part);
                break;
            case 2:
                CHECK(buffer.displace(count) == std::min(count, expected.size()));
                expected.erase(0, count);
                break;
            case 3:
                if (count < expected.size())
                {
                    CHECK(buffer.truncate(count) == count);
                    expected.resize(count);
                }
                else
                {
                    CHECK(!buffer.truncate(count));
                }
                break;
            case 4:
                CHECK(buffer.clear());
                expected.clear();
                break;
            }
            contents(buffer, expected);
        }
    }
}

TEST("containers", "numeric_conversion_boundaries_and_invalid_input")
{
    for (const std::string text : {"0", "42", "4294967295"})
    {
        B_Chunks buffer(text);
        CHECK(buffer.toUInt32() == std::stoull(text));
        CHECK(buffer.toUInt64() == std::stoull(text));
    }
    B_Chunks maximum("18446744073709551615"), hex("ff"), slice("xx123yy");
    CHECK(maximum.toUInt64() == UINT64_MAX);
    CHECK(hex.toUInt32(16) == 255);
    CHECK(slice.toUInt64(10, 3, 2) == 123);
    for (const std::string text : std::vector<std::string>{"", "-1", "42x", "4294967296", "9999999999999999999999999", std::string("12\0x", 4)})
    {
        B_Chunks buffer(text);
        CHECK(!buffer.toUInt32());
    }
    for (const std::string text : {"-1", "18446744073709551616", "42x", ""})
    {
        B_Chunks buffer(text);
        CHECK(!buffer.toUInt64());
    }
}

TEST("containers", "empty_needle_repeated_candidates_and_search_model")
{
    std::mt19937 random(20261116);
    for (size_t iteration = 0; iteration < 250; ++iteration)
    {
        std::string data(32, 'a');
        for (char &c : data)
        {
            c = "abAB\0"[random() % 5];
        }
        B_Chunks chunks;
        chunks.setMaxChunkSize(3);
        CHECK(chunks.append(data.data(), data.size()) == data.size());
        B_MEM memory(data.data(), data.size());
        B_Ref view(&chunks, 5, 20);
        for (B_Base *buffer : {static_cast<B_Base *>(&memory), static_cast<B_Base *>(&chunks), static_cast<B_Base *>(&view)})
        {
            const std::string text = buffer == &view ? data.substr(5, 20) : data;
            const size_t offset = random() % (text.size() + 1);
            const size_t space = random() % (text.size() - offset + 1);
            const size_t limit = space == 0 ? text.size() : offset + space;
            const std::string needle = iteration % 2 ? "aaaab" : std::string("b\0", 2);
            const auto expected = text.substr(0, limit).find(needle, offset);
            const auto actual = buffer->find(needle.data(), needle.size(), true, offset, space);
            CHECK(actual == (expected == std::string::npos ? std::optional<size_t>{} : std::optional<size_t>{expected}));
            CHECK(buffer->find(nullptr, 0, true, offset) == offset);
        }
    }
    B_Chunks empty;
    CHECK(empty.find(nullptr, 0) == 0);
}

TEST("containers", "assignment_self_assignment_and_failed_output_stream")
{
    B_Chunks source("source"), destination("old");
    static_cast<B_Base &>(destination) = source;
    contents(destination, "source");
    static_cast<B_Base &>(destination) = destination;
    contents(destination, "source");
    std::ostringstream failed;
    failed.setstate(std::ios::failbit);
    CHECK(!source.copyToStream(failed));
    CHECK(failed.str().empty());
    CHECK(source.compare(std::string("SOURCE"), false));
    CHECK(!source.compare(std::string("sour"), false));
}

TEST("containers", "delimiter_at_start_copy_limits_and_reference_source")
{
    B_Chunks source("::rest"), destination;
    CHECK(source.displaceUntil(destination, "::", 2, 0, true) == 0);
    contents(source, "rest");
    contents(destination, "");
    B_Chunks limited("body::tail");
    CHECK(limited.copyUntil(destination, "::", 2, 3, true) == -2);
    CHECK(limited.copyUntil(destination, "::", 2, 4, false) == -2);
    contents(destination, "");
    CHECK(limited.copyUntil(destination, "::", 2, 4, true) == 0);
    contents(destination, "body");
    B_Ref view(&limited, 2, 4);
    CHECK(view.find("::", 2) == 2);
}
