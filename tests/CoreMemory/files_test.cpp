#include "test_support.h"
#include <Mantids30/Core/Memory/b_chunks.h>
#include <Mantids30/Core/Memory/b_mmap.h>
#include <Mantids30/Core/Memory/filemap.h>
#include <Mantids30/Core/Memory/streamable_file.h>
#include <Mantids30/Core/Memory/streamable_string.h>
#include <fcntl.h>
#include <fstream>
#include <iterator>

using namespace Mantids30::Core::Memory::Containers;
using namespace Mantids30::Core::Memory::Streams;
using namespace MemoryTests;

namespace {
std::string readFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    CHECK(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
}

TEST("files", "filemap_empty_append_prepend_displace_truncate_and_reopen")
{
    TempDirectory directory;
    const auto path = directory.path / "data.bin";
    FileMap map;
    CHECK(map.openFile(path.string(), false, true));
    CHECK(map.getFileOpenSize() == 0);
    CHECK(map.mmapAppend(nullptr, 0) == 0);
    CHECK(map.mmapAppend("body", 4) == 4);
    CHECK(map.mmapPrepend("head", 4) == 4);
    CHECK(std::string(map.getMmapAddr(), map.getFileOpenSize()) == "headbody");
    CHECK(map.mmapDisplace(2));
    CHECK(std::string(map.getMmapAddr(), map.getFileOpenSize()) == "adbody");
    CHECK(map.mmapTruncate(3));
    CHECK(std::string(map.getMmapAddr(), map.getFileOpenSize()) == "adb");
    CHECK(map.closeFile());
    CHECK(map.getFileOpenSize() == 0);
    CHECK(map.getMmapAddr() == nullptr);
    CHECK(readFile(path) == "adb");
    CHECK(map.openFile(path.string(), true, false));
    CHECK(map.getFileOpenSize() == 3);
    CHECK(!map.mmapAppend("x", 1));
    CHECK(!map.mmapTruncate(0));
    CHECK(map.closeFile());
    CHECK(map.closeFile());
}

TEST("files", "filemap_invalid_ranges_and_readonly_displace")
{
    TempDirectory directory;
    const auto path = (directory.path / "data.bin").string();
    FileMap map;
    CHECK(map.openFile(path, false, true));
    CHECK(map.mmapAppend("abcd", 4) == 4);
    CHECK(!map.mmapDisplace(5));
    CHECK(!map.mmapAppend("x", unlimited));
    CHECK(!map.mmapPrepend("x", unlimited));
    CHECK(std::string(map.getMmapAddr(), map.getFileOpenSize()) == "abcd");
    CHECK(map.closeFile());
    CHECK(map.openFile(path, true, false));
    CHECK(!map.mmapDisplace(1)); // Must reject before touching a read-only mapping.
    CHECK(readFile(path) == "abcd");
}

TEST("files", "mmap_binary_mutations_and_clear")
{
    TempDirectory directory;
    const auto path = (directory.path / "mapped.bin").string();
    const auto data = binaryData(65537);
    B_MMAP buffer;
    CHECK(buffer.referenceFile(path));
    CHECK(buffer.append(data.data(), data.size()) == data.size());
    CHECK(buffer.toString() == data);
    CHECK(buffer.prepend("HEAD") == 4);
    CHECK(buffer.toString() == "HEAD" + data);
    CHECK(buffer.displace(4) == 4);
    CHECK(buffer.toString() == data);
    CHECK(buffer.truncate(8193) == 8193);
    CHECK(buffer.toString() == data.substr(0, 8193));
    CHECK(buffer.clear());
    CHECK(buffer.size() == 0);
    CHECK(buffer.toString() == "");
    CHECK(readFile(path) == data.substr(0, 8193));
}

TEST("files", "mmap_readonly_rejects_all_mutations_without_losing_mapping")
{
    TempDirectory directory;
    const auto path = directory.path / "readonly.bin";
    {
        std::ofstream file(path);
        file << "preserve";
    }
    B_MMAP buffer;
    CHECK(buffer.referenceFile(path.string(), true, false));
    CHECK(!buffer.append("x"));
    CHECK(!buffer.prepend("x"));
    CHECK(!buffer.truncate(2));
    CHECK(!buffer.displace(1));
    CHECK(buffer.toString() == "preserve");
    CHECK(buffer.size() == 8);
    CHECK(readFile(path) == "preserve");
}

TEST("files", "temporary_mapping_cleanup_and_persistent_copy")
{
    TempDirectory directory;
    std::string temporaryPath;
    {
        B_Chunks source(std::string("a\0b", 3));
        source.setFsDirectoryPath(directory.path.string());
        source.setFsBaseFileName("memory-test");
        auto mapped = source.copyToFS("", true);
        CHECK(mapped);
        temporaryPath = mapped->getCurrentFileName();
        CHECK(std::filesystem::exists(temporaryPath));
        CHECK(mapped->toString() == std::string("a\0b", 3));
    }
    CHECK(!std::filesystem::exists(temporaryPath));
    const auto persistent = directory.path / "persistent.bin";
    {
        B_Chunks source("persist");
        auto mapped = source.copyToFS(persistent.string(), false);
        CHECK(mapped);
    }
    CHECK(readFile(persistent) == "persist");
}

TEST("files", "chunks_spill_threshold_mutations_search_and_cleanup")
{
    TempDirectory directory;
    std::string path;
    {
        B_Chunks buffer;
        buffer.setFsDirectoryPath(directory.path.string());
        buffer.setMaxSizeInMemoryBeforeMovingToDisk(4);
        CHECK(buffer.getMaxSizeInMemoryBeforeMovingToDisk() == 4);
        CHECK(buffer.append("Abcd") == 4);
        CHECK(!buffer.isUsingFiles());
        CHECK(buffer.append("E") == 1);
        CHECK(buffer.isUsingFiles());
        auto map = buffer.getMmapContainer();
        CHECK(map);
        path = map->getCurrentFileName();
        CHECK(buffer.toString() == "AbcdE");
        CHECK(buffer.findChar('A', 0, 1, true) == 0);
        CHECK(!buffer.findChar('a', 0, 1, true));
        CHECK(!buffer.findChar('E', 0, 2, false));
        CHECK(buffer.prepend("12") == 2);
        CHECK(buffer.toString() == "12AbcdE");
        CHECK(buffer.displace(2) == 2);
        CHECK(buffer.truncate(3) == 3);
        CHECK(buffer.toString() == "Abc");
    }
    CHECK(!std::filesystem::exists(path));
}

TEST("files", "spill_failure_preserves_existing_memory")
{
    TempDirectory directory;
    B_Chunks buffer("abc");
    buffer.setFsDirectoryPath((directory.path / "missing").string());
    buffer.setMaxSizeInMemoryBeforeMovingToDisk(3);
    CHECK(!buffer.append("d"));
    CHECK(!buffer.isUsingFiles());
    CHECK(buffer.toString() == "abc");
    CHECK(buffer.size() == 3);
}

TEST("files", "missing_file_and_invalid_directory_fail_cleanly")
{
    TempDirectory directory;
    B_MMAP buffer;
    CHECK(!buffer.referenceFile((directory.path / "missing").string(), true, false));
    CHECK(buffer.size() == 0);
    CHECK(!buffer.referenceFile((directory.path / "missing" / "data").string()));
    CHECK(buffer.size() == 0);
}

TEST("files", "streamable_file_binary_roundtrip_repeat_read_and_errors")
{
    TempDirectory directory;
    const auto path = (directory.path / "stream.bin").string();
    const auto data = binaryData(20001);
    {
        StreamableFile file(-1, -1);
        CHECK(file.open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600) >= 0);
        CHECK(file.writeString(data));
        for (int iteration = 0; iteration < 2; ++iteration)
        {
            StreamableString destination;
            CHECK(file.streamTo(&destination));
            CHECK(destination.getValue() == data);
            CHECK(!destination.writeStatus.finished);
        }
    }
    CHECK(readFile(path) == data);
    StreamableFile invalid(-1, -1);
    CHECK(!invalid.write("x", 1));
    CHECK(!invalid.writeStatus.succeed);
    StreamableString destination;
    CHECK(!invalid.streamTo(&destination));
}

TEST("files", "mapping_shrink_to_empty_reuse_and_close_delete_policy")
{
    TempDirectory directory;
    const auto path = directory.path / "empty.bin";
    B_MMAP buffer;
    CHECK(buffer.referenceFile(path.string()));
    CHECK(buffer.append("abc") == 3);
    CHECK(buffer.truncate(0) == 0);
    CHECK(buffer.toString() == "");
    CHECK(buffer.append("xyz") == 3);
    CHECK(buffer.displace(unlimited) == 3);
    CHECK(buffer.size() == 0);
    CHECK(buffer.append("next") == 4);
    CHECK(buffer.toString() == "next");
    CHECK(buffer.clear());
    FileMap map;
    CHECK(map.openFile(path.string(), false, false));
    map.setDeleteFileOnDestruction(true);
    CHECK(map.closeFile(false));
    CHECK(std::filesystem::exists(path));
    CHECK(map.openFile(path.string(), false, false));
    map.setDeleteFileOnDestruction(true);
    CHECK(map.closeFile());
    CHECK(!std::filesystem::exists(path));
}

TEST("files", "filemap_assignment_transfers_ownership_and_closes_previous_mapping")
{
    TempDirectory directory;
    const auto first = directory.path / "first.bin", second = directory.path / "second.bin";
    FileMap source, destination;
    CHECK(source.openFile(first.string(), false, true));
    CHECK(source.mmapAppend("first", 5) == 5);
    CHECK(destination.openFile(second.string(), false, true));
    destination.setDeleteFileOnDestruction(true);
    destination = source;
    CHECK(source.getMmapAddr() == nullptr);
    CHECK(source.getFileOpenSize() == 0);
    CHECK(!std::filesystem::exists(second));
    CHECK(std::string(destination.getMmapAddr(), destination.getFileOpenSize()) == "first");
    destination = destination;
    CHECK(destination.getFileOpenSize() == 5);
}
