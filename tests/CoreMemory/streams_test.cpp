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
    std::optional<size_t> writeImpl(const void *data, const size_t &count) override
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

TEST("streams", "progress_percentage_byte_intervals_and_completion")
{
    StreamableNull sink;
    std::vector<StreamableObject::WriteProgress> events;
    sink.setWriteTargetSize(10000);
    sink.setWriteProgressCallback([&](const auto &progress) { events.push_back(progress); });
    const std::string data(10, 'x');
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(sink.write(data.data(), 1) == 1);
    }
    CHECK(events.size() == 1);
    CHECK(events.back().bytesWritten == 10);
    CHECK(events.back().totalSize == 10000);
    CHECK(events.back().percentage == 0.1);
    CHECK(sink.writeFullStream(data.data(), data.size()));
    CHECK(events.size() == 2);
    CHECK(events.back().percentage == 0.2);
    CHECK(sink.writeFullStream(data.data(), data.size()));
    CHECK(events.size() == 3);
    CHECK(events.back().percentage == 0.3);

    sink.setWriteTargetSize(250);
    sink.setWriteProgressCallback([&](const auto &progress) { events.push_back(progress); }, 0, 100);
    events.clear();
    const std::string block(99, 'x');
    CHECK(sink.writeString(block));
    CHECK(events.empty());
    CHECK(sink.writeString("x"));
    CHECK(events.size() == 1 && events.back().bytesWritten == 100);
    CHECK(sink.writeString(std::string(150, 'x')));
    CHECK(events.size() == 2 && events.back().percentage == 100);
    CHECK(sink.writeEOF());
    CHECK(events.size() == 2);
}

TEST("streams", "progress_unknown_total_partial_writes_and_eof_flush")
{
    Sink sink(Sink::PARTIAL, 3);
    std::vector<StreamableObject::WriteProgress> events;
    sink.setWriteProgressCallback([&](const auto &progress) { events.push_back(progress); }, 50, 100);
    CHECK(sink.writeString("abcdefgh"));
    CHECK(events.size() == 3);
    CHECK(events[0].bytesWritten == 3 && events[1].bytesWritten == 6 && events[2].bytesWritten == 8);
    CHECK(!events.back().totalSize && !events.back().percentage);
    CHECK(sink.writeEOF());
    CHECK(events.size() == 3);

    Sink shortSink;
    shortSink.setWriteTargetSize(100);
    shortSink.setWriteProgressCallback([&](const auto &progress) { events.push_back(progress); }, 50);
    events.clear();
    CHECK(shortSink.writeString("abc"));
    CHECK(events.empty());
    CHECK(shortSink.writeEOF());
    CHECK(events.size() == 1 && events.back().percentage == 3);
}

TEST("streams", "progress_empty_unknown_reset_disabled_and_invalid_configuration")
{
    StreamableNull sink;
    std::vector<StreamableObject::WriteProgress> events;
    const auto callback = [&](const auto &progress) { events.push_back(progress); };
    sink.setWriteProgressCallback(callback);
    CHECK(sink.writeEOF());
    CHECK(events.empty());
    sink.writeStatus.reset();
    sink.setWriteTargetSize(0);
    CHECK(sink.writeEOF());
    CHECK(events.size() == 1 && events.back().percentage == 100);
    sink.writeStatus.reset();
    sink.setWriteTargetSize(std::nullopt);
    CHECK(sink.writeString("x"));
    CHECK(events.size() == 2 && events.back().bytesWritten == 1 && !events.back().percentage);
    sink.setWriteProgressCallback({});
    CHECK(sink.writeString("y"));
    CHECK(events.size() == 2);
    for (double interval : {-1.0, 101.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        throws<std::invalid_argument>([&] { sink.setWriteProgressCallback(callback, interval); });
    }
}

TEST("streams", "progress_failures_do_not_notify")
{
    for (auto mode : {Sink::ERROR, Sink::ZERO, Sink::OVERSIZED, Sink::STATUS_ERROR})
    {
        Sink sink(mode);
        size_t calls = 0;
        sink.setWriteProgressCallback([&](const auto &) { ++calls; });
        CHECK(!sink.writeString("abc"));
        CHECK(calls == 0);
    }
    Sink sink(Sink::EOF_ERROR);
    size_t calls = 0;
    sink.setWriteTargetSize(10);
    sink.setWriteProgressCallback([&](const auto &) { ++calls; }, 50);
    CHECK(sink.writeString("a"));
    CHECK(!sink.writeEOF());
    CHECK(calls == 0);
}
