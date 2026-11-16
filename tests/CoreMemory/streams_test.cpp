#include "test_support.h"
#include <Mantids30/Core/Memory/b_chunks.h>
#include <Mantids30/Core/Memory/streamable_null.h>
#include <Mantids30/Core/Memory/streamable_string.h>
#include <Mantids30/Core/Memory/streamable_transformer.h>

using namespace Mantids30::Core::Memory::Streams;
using namespace MemoryTests;

namespace {
// Emulate partial progress, invalid write counts, and EOF failures for stream contracts.
class Sink : public StreamableObject
{
public:
    enum Mode
    {
        PARTIAL,
        ERROR,
        ZERO,
        OVERSIZED,
        STATUS_ERROR,
        EOF_ERROR
    };
    explicit Sink(Mode mode = PARTIAL, size_t block = 3) : mode(mode), block(block)
    {
    }
    std::optional<size_t> write(const void *data, const size_t &count) override
    {
        ++calls;
        if (!count)
        {
            ++eofs;
            if (mode == EOF_ERROR)
            {
                return std::nullopt;
            }
            writeStatus += 0;
            return 0;
        }
        if (mode == ERROR)
        {
            return std::nullopt;
        }
        if (mode == ZERO)
        {
            return 0;
        }
        if (mode == OVERSIZED)
        {
            return count + 1;
        }
        if (mode == STATUS_ERROR)
        {
            writeStatus += -1;
            return count;
        }
        const auto copied = std::min(count, block);
        output.append(static_cast<const char *>(data), copied);
        writeStatus += copied;
        return copied;
    }
    Mode mode;
    size_t block, calls = 0, eofs = 0;
    std::string output;
};

class Uppercase : public StreamableTransformer
{
    size_t writeTo(Mantids30::Core::Memory::Streams::StreamableObject *destination, const void *data, const size_t &count) override
    {
        std::string result(static_cast<const char *>(data), count);
        for (char &c : result)
        {
            if (c >= 'a' && c <= 'z')
            {
                c -= 'a' - 'A';
            }
        }
        return destination->writeString(result) ? count : 0;
    }
    size_t writeTransformerEOF(Mantids30::Core::Memory::Streams::StreamableObject *destination) override
    {
        ++eofs;
        destination->writeString("!");
        return 0;
    }

public:
    int eofs = 0;
};
}

TEST("streams", "partial_writes_complete_without_duplication")
{
    const auto data = binaryData(65537);
    for (size_t block : {size_t(1), size_t(7), size_t(8192), size_t(65536)})
    {
        Sink sink(Sink::PARTIAL, block);
        CHECK(sink.writeFullStream(data.data(), data.size()));
        CHECK(sink.output == data);
        CHECK(sink.writeStatus.bytesWritten == data.size());
        CHECK(!sink.writeStatus.finished);
        CHECK(sink.eofs == 0);
    }
}

TEST("streams", "zero_length_full_write_does_not_signal_eof")
{
    Sink sink;
    CHECK(sink.writeFullStream(nullptr, 0));
    CHECK(sink.writeString(""));
    CHECK(sink.calls == 0);
    CHECK(!sink.writeStatus.finished);
    CHECK(sink.writeFullStreamWithEOF(nullptr, 0));
    CHECK(sink.eofs == 1);
    CHECK(sink.writeStatus.finished);
}

TEST("streams", "failed_stalled_oversized_and_status_errors_stop_immediately")
{
    for (auto mode : {Sink::ERROR, Sink::ZERO, Sink::OVERSIZED, Sink::STATUS_ERROR})
    {
        Sink sink(mode);
        CHECK(!sink.writeFullStream("abc", 3));
        CHECK(sink.calls == 1);
        CHECK(sink.output.empty());
        CHECK(!sink.writeStatus.succeed);
        CHECK(sink.writeStatus.finished);
        CHECK(sink.eofs == 0);
    }
    Sink failed(Sink::ERROR);
    CHECK(!failed.writeFullStreamWithEOF("abc", 3));
    CHECK(failed.eofs == 0);
    Sink eof(Sink::EOF_ERROR);
    CHECK(!eof.writeFullStreamWithEOF("abc", 3));
    CHECK(eof.output == "abc");
    CHECK(!eof.writeStatus.succeed);
}

TEST("streams", "binary_string_assignment_append_and_transfer")
{
    StreamableString source;
    source = std::string("a\0b", 3);
    CHECK(source.writeString(std::string("\xffZ", 2)));
    CHECK(source.getValue() == std::string("a\0b\xffZ", 5));
    CHECK(source.toString() == source.getValue());
    Sink destination;
    CHECK(source.streamTo(&destination));
    CHECK(destination.output == source.getValue());
    CHECK(!destination.writeStatus.finished);
    CHECK(source.writeEOF());
    CHECK(source.writeStatus.finished);
    CHECK(source.getValue() == std::string("a\0b\xffZ", 5));
}

TEST("streams", "container_to_partial_sink_and_error_propagation")
{
    Mantids30::Core::Memory::Containers::B_Chunks source;
    source.setMaxChunkSize(3);
    CHECK(source.append("0123456789") == 10);
    Sink destination;
    CHECK(source.appendTo(destination, 5, 3) == 5);
    CHECK(destination.output == "34567");
    CHECK(destination.eofs == 0);
    Sink failed(Sink::ERROR);
    CHECK(!source.streamTo(&failed));
    CHECK(!failed.writeStatus.succeed);
}

TEST("streams", "null_sink_counts_binary_bytes_and_eof")
{
    StreamableNull sink;
    CHECK(sink.size() == 0);
    CHECK(sink.write("abc", 3) == 3);
    CHECK(sink.writeString(std::string("\0\xff", 2)));
    CHECK(sink.size() == 5);
    CHECK(sink.writeStatus.bytesWritten == 5);
    CHECK(sink.writeEOF());
    CHECK(sink.size() == 5);
    CHECK(sink.writeStatus.finished);
}

TEST("streams", "write_status_accumulates_finishes_resets_and_rejects_invalid_operations")
{
    WriteStatus status;
    CHECK(status.succeed && !status.finished && status.bytesWritten == 0);
    status += 3;
    status += 4;
    CHECK(status.bytesWritten == 7);
    status += 0;
    CHECK(status.finished && status.succeed);
    throws<std::runtime_error>([&]
                               {
                                   status += 1;
                               });
    status.reset();
    CHECK(status.succeed && !status.finished && status.bytesWritten == 0 && status.writeError == 0);
    status += -7;
    CHECK(!status.succeed && status.finished && status.writeError == -7);
    throws<std::runtime_error>([&]
                               {
                                   status += 1;
                               });
    status.reset();
    WriteStatus other(3);
    status += other;
    CHECK(status.bytesWritten == 3);
    status - other;
    CHECK(status.bytesWritten == 0);
    throws<std::runtime_error>([&]
                               {
                                   status - other;
                               });
}

TEST("streams", "transformer_handles_binary_and_finalizes_each_transformation")
{
    Uppercase transformer;
    for (int iteration = 0; iteration < 2; ++iteration)
    {
        StreamableString source, destination;
        source = std::string("ab\0z", 4);
        transformer.transform(&source, &destination);
        CHECK(destination.getValue() == std::string("AB\0Z!", 5));
        CHECK(transformer.eofs == iteration + 1);
    }
}

TEST("streams", "formatted_write_reports_byte_count")
{
    StreamableString destination;
    CHECK(destination.strPrintf("%s:%d", "value", 42) == 8);
    CHECK(destination.getValue() == "value:42");
}

TEST("streams", "formatted_binary_empty_and_failed_output")
{
    StreamableString destination;
    CHECK(destination.strPrintf("%c%c%c", 'a', 0, 'b') == 3);
    CHECK(destination.getValue() == std::string("a\0b", 3));
    CHECK(destination.strPrintf("%s", "") == 0);
    Sink failed(Sink::ERROR);
    CHECK(failed.strPrintf("%s", "abc") == 0);
    CHECK(!failed.writeStatus.succeed);
}
