#include "b_base.h"
#include "b_mmap.h"

#include <Mantids30/Core/Helpers/safeint.h>

#include <cstring>
#include <cerrno>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

using namespace Mantids30::Core::Memory::Containers;

B_Base::B_Base()
{
    clear0();
}

B_Base::~B_Base()
{
    clear0();
}

B_Base &B_Base::operator=(B_Base &bc)
{
    // Clearing a container before copying from itself would discard its contents.
    if (&bc == this)
    {
        return *this;
    }
    clear();
    bc.appendTo(*this);
    return *this;
}

void B_Base::print(FILE *f)
{
    std::optional<std::string> str = toString();
    if (str)
    {
        fprintf(f, "%s", str->c_str());
    }
}

std::optional<size_t> B_Base::prepend(const void *buf)
{
    return prepend(buf, strlen(static_cast<const char *>(buf)));
}

std::optional<size_t> B_Base::append(const void *buf)
{
    return append(buf, strlen(static_cast<const char *>(buf)));
}

std::optional<size_t> B_Base::append(const void *data, size_t len)
{
    // Read only: can't append nothing.
    if (m_readOnly)
    {
        return std::nullopt;
    }

    size_t currentSize = size();

    // New size will overflow the counter...
    if (CHECK_UINT_OVERFLOW_SUM(len, currentSize))
    {
        return std::nullopt;
    }
    // out of bounds, fail.
    if (len + currentSize > m_maxSize)
    {
        return std::nullopt;
    }
    // zero to copy!
    if (!len)
    {
        return std::nullopt;
    }

    // Data modification should pass trough referenced pointer.
    return append2(data, len, false);
}

std::optional<size_t> B_Base::prepend(const void *data, size_t len)
{
    // Read only: can't append nothing.
    if (m_readOnly)
    {
        return std::nullopt;
    }

    size_t currentSize = size();

    // New size will overflow the counter...
    if (CHECK_UINT_OVERFLOW_SUM(len, currentSize))
    {
        return std::nullopt;
    }
    // out of bounds.
    if (len + currentSize > m_maxSize)
    {
        return std::nullopt;
    }
    // zero to copy!
    if (!len)
    {
        return 0;
    }

    return append2(data, len, true);
}

std::optional<size_t> B_Base::displace(const size_t &bytes)
{
    size_t currentSize = size();
    return displace2(std::min(bytes, currentSize));
}

std::optional<size_t> B_Base::truncate(const size_t &bytes)
{
    size_t currentSize = size();

    if (bytes >= currentSize)
    {
        return std::nullopt;
    }

    return truncate2(bytes);
}

bool B_Base::clear0()
{
    m_containerBytes = 0;

#ifdef _WIN32
    char tempPath[MAX_PATH + 1];
    GetTempPathA(MAX_PATH, tempPath);
    fsDirectoryPath = tempPath;
#else
    m_fsDirectoryPath = "/tmp";
#endif

    m_fsBaseFileName = "BinaryContainer-";
    return true;
}

bool B_Base::clear()
{
    clear0();
    return clear2();
}

int B_Base::copyUntil(B_Base &destination, const void *needle, const size_t &needleLenght, const size_t &maxCopySize, bool removeNeedle)
{
    // Locate the delimiter first so missing delimiters and oversized copies remain distinct errors.
    auto needlePos = find(needle, needleLenght, false);
    if (!needlePos)
    {
        return -1;
    }
    const size_t count = *needlePos + (removeNeedle ? 0 : needleLenght);
    if (count > maxCopySize)
    {
        return -2;
    }
    // A partial destination write cannot satisfy the requested delimiter copy.
    auto copied = appendTo(destination, count);
    return copied && *copied == count ? 0 : -1;
}

int B_Base::displaceUntil(B_Base &destination, const void *needle, const size_t &needleCount, const size_t &maxCopySize, bool removeNeedle)
{
    const size_t previousSize = destination.size();
    const int result = copyUntil(destination, needle, needleCount, maxCopySize, removeNeedle);
    if (result < 0)
    {
        return result;
    }
    // Consume only the newly copied bytes, plus the delimiter when it was omitted from the copy.
    auto displaced = displace(destination.size() - previousSize + (removeNeedle ? needleCount : 0));
    return displaced ? 0 : -1;
}

int B_Base::displaceUntil(B_Base &destination, const std::list<std::string> &needles, const size_t &maxCopySize, bool removeNeedle)
{
    for (const auto &needle : needles)
    {
        const int result = displaceUntil(destination, needle.data(), needle.size(), maxCopySize, removeNeedle);
        if (result == 0)
        {
            return 0;
        }
    }
    return -1;
}

// TODO: pass this to shared_ptr
/*
std::list<Mantids30::Core::Memory::Containers::B_Base *> B_Base::referencedSplit(const std::list<std::string> &needles,const size_t &maxSearchSize, const size_t &maxNeedles)
{
    std::list<std::string> skipBegWith;
    return referencedSplit2(needles,skipBegWith,maxSearchSize,maxNeedles);
}

std::list<Mantids30::Core::Memory::Containers::B_Base *> B_Base::referencedSplit2(const std::list<std::string> &needles, const std::list<std::string> &skipBegWith, const size_t &maxSearchSize, const size_t &roMaxNeedles)
{
    size_t maxNeedles = roMaxNeedles;
    std::list<Mantids30::Core::Memory::Containers::B_Base *> x;

    size_t currentOffset = 0;

    if (isNull()) 
        return x;
    if (maxNeedles == 0) maxNeedles = std::numeric_limits<size_t>::max();

    B_Ref * current = new B_Ref;
    current->reference(this,currentOffset); // reference the whole container.
    maxNeedles--;

    if (maxNeedles == 0)
    {
        x.push_back(current);
        return x;
    }

    size_t currentLocalOffset = 0;

    for (size_t i=0;i<maxNeedles;i++)
    {
        // TODO: repair maxSearchSize.
        std::string needleFound;
        std::optional<size_t> pos = current->find(needles,needleFound,currentLocalOffset,maxSearchSize);
        if (pos.first == false)
        {
            // needle not found... so this is the last element.
            x.push_back(current);
            return x;
        }
        else
        {
            // Displace the offset.
            currentOffset+=pos.second+needleFound.size();
            currentLocalOffset+=pos.second+needleFound.size();

            // Don't have more chunks. Get out.
            if (currentOffset>=size())
            {
                // Last element with another one at the end.
                current->truncate(current->size()-needleFound.size());
                x.push_back(current);
                x.push_back(nullptr);
                return x;
            }

            //////////////////////////////////////////////////////
            // Creates the next binary container.
            B_Ref * next = new B_Ref;
            next->reference(this,currentOffset);

            bool skipThis = false;
            for (std::string skip : skipBegWith)
            {
                if (next->compare(skip.c_str(),skip.size())) skipThis = true;
            }

            if (skipThis)
            {
                delete next;
                // skip to the next...
            }
            else
            {
                current->truncate(currentLocalOffset);
                x.push_back(current);
                current = next;
            }
        }
    }

    return x;
}*/

void B_Base::freeSplitList(const std::list<Mantids30::Core::Memory::Containers::B_Base *> &x)
{
    for (Mantids30::Core::Memory::Containers::B_Base *i : x)
    {
        delete i;
    }
}

std::optional<size_t> B_Base::copyToStream(std::ostream &out, size_t bytes, const size_t &offset)
{
    size_t currentSize = size();

    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
        {
            return std::nullopt; // invalid size.
        }
        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
    {
        return std::nullopt;
    }

    // No bytes to copy:
    if (!bytes)
    {
        return 0;
    }

    // out of bounds for sure.
    if (offset > currentSize)
    {
        return std::nullopt;
    }

    // out of bounds (last bytes):
    if (offset + bytes > currentSize)
    {
        bytes = currentSize - offset;
    }

    ////////////////////////////////////
    return copyToStream2(out, bytes, offset);
}

std::optional<size_t> B_Base::appendTo(StreamableObject &out, const size_t &bytes, const size_t &offset)
{
    size_t currentSize = size();
    size_t bytesToCopy = bytes;

    // Copy eveything...
    if (bytesToCopy == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
        {
            out.writeStatus += -1;
            return std::nullopt; // invalid pos.
        }
        bytesToCopy = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytesToCopy))
    {
        out.writeStatus += -1;
        return std::nullopt; // invalid pos.
    }

    // No bytes to copy:
    if (!bytesToCopy)
    {
        return 0; // Zero bytes to copy.
    }

    // out of bounds for sure.
    if (offset > currentSize)
    {
        out.writeStatus += -1;
        return std::nullopt; // invalid pos.
    }

    // out of bounds (last bytes):
    if (offset + bytesToCopy > currentSize)
    {
        bytesToCopy = currentSize - offset;
    }

    ////////////////////////////////////
    return copyToStreamableObject2(out, bytesToCopy, offset);
}

std::optional<size_t> B_Base::copyOut(void *buf, size_t bytes, const size_t &offset)
{
    size_t currentSize = size();

    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
        {
            return std::nullopt; // out of bounds.
        }

        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
    {
        return std::nullopt;
    }

    // No bytes to copy:
    if (!bytes)
    {
        return 0;
    }

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
    {
        return std::nullopt;
    }

    ////////////////////////////////////
    return copyToBuffer2(buf, bytes, offset);
}

std::vector<char> B_Base::copyToBuffer()
{
    size_t n = size();
    std::vector<char> buf(n);

    std::optional<size_t> copied = copyOut(buf.data(), n, 0);
    if (!copied || *copied != n)
    {
        return {};
    }

    return buf;
}

std::optional<size_t> B_Base::copyToString(std::string &outStr, size_t bytes, const size_t &roOffset)
{
    size_t currentSize = size();

    size_t offset = roOffset;
    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
        {
            return std::nullopt; // out of bounds
        }

        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
    {
        return std::nullopt;
    }

    // No bytes to copy:
    if (!bytes)
    {
        return 0;
    }

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
    {
        return std::nullopt;
    }

    char outmem[8192];
    ////////////////////////////////////
    while (bytes)
    {
        size_t copyBytes = std::min(bytes, static_cast<size_t>(8192));
        bytes -= copyBytes;

        std::optional<size_t> outBytes = copyOut(outmem, copyBytes, offset);

        if (outBytes != std::nullopt)
        {
            outStr.append(outmem, outBytes.value());
        }
        else
        {
            return outStr.size();
        }

        offset += copyBytes;
    }

    return outStr.size();
}

std::optional<std::string> B_Base::toString(size_t bytes, const size_t &roOffset)
{
    std::string r;

    size_t currentSize = size();
    size_t offset = roOffset;

    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
        {
            return std::nullopt; // negative bytes copied.
        }
        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
    {
        return std::nullopt; // negative bytes copied.
    }

    // No bytes to copy:
    if (!bytes)
    {
        return r;
    }

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
    {
        return std::nullopt; // negative bytes copied.
    }

    char outmem[8192];
    ////////////////////////////////////
    while (bytes)
    {
        size_t copyBytes = std::min(bytes, static_cast<size_t>(8192));
        bytes -= copyBytes;
        std::optional<size_t> outBytes = copyOut(outmem, copyBytes, offset);

        if (outBytes != std::nullopt)
        {
            r.append(outmem, outBytes.value());
        }
        else
        {
            return r;
        }

        offset += copyBytes;
    }

    return r;
}

std::optional<uint64_t> B_Base::toUInt64(int base, const size_t &bytes, const size_t &offset)
{
    // Validate the complete byte range before using the null-terminated numeric parser.
    auto text = toString(bytes, offset);
    if (!text || text->empty() || base < 0 || base == 1 || base > 36 || text->find('\0') != std::string::npos)
    {
        return std::nullopt;
    }
    // strtoull accepts negative input by wrapping it; unsigned conversion must reject it.
    const auto first = text->find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos || (*text)[first] == '-')
    {
        return std::nullopt;
    }
    errno = 0;
    char *end = nullptr;
    const auto result = strtoull(text->c_str(), &end, base);
    // Reject overflow and any bytes left unconsumed by the conversion.
    if (errno == ERANGE || end == text->c_str() || end != text->c_str() + text->size())
    {
        return std::nullopt;
    }
    return result;
}

std::optional<uint32_t> B_Base::toUInt32(int base, const size_t &bytes, const size_t &offset)
{
    // Validate at full width before narrowing to avoid silently truncating large values.
    const auto result = toUInt64(base, bytes, offset);
    if (!result || *result > UINT32_MAX)
    {
        return std::nullopt;
    }
    return static_cast<uint32_t>(*result);
}

bool B_Base::compare(const std::string &cmpString, bool caseSensitive, const size_t &offset)
{
    if (cmpString.size() != size())
    {
        return false;
    }
    return compare(cmpString.c_str(), cmpString.size(), caseSensitive, offset);
}

bool B_Base::compare(const void *mem, const size_t &len, bool caseSensitive, const size_t &offset)
{
    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, len))
    {
        return false;
    }

    // No data to compare...
    if (!len)
    {
        return true;
    }

    // out of bounds (fail to compare):
    if (offset + len > size())
    {
        return false;
    }

    /////////////////////////////

    return compare2(mem, len, caseSensitive, offset);
}

std::optional<size_t> B_Base::find(const void *needle, const size_t &needle_len, bool caseSensitive, const size_t &offset, size_t searchSpace)
{
    // Bound candidate positions so every complete match stays inside the requested search range.
    const size_t currentSize = size();
    if (offset > currentSize || CHECK_UINT_OVERFLOW_SUM(offset, searchSpace))
    {
        return std::nullopt;
    }
    const size_t available = currentSize - offset;
    searchSpace = searchSpace == 0 ? available : std::min(searchSpace, available);
    if (needle_len == 0)
    {
        return offset;
    }
    if (!needle || needle_len > searchSpace)
    {
        return std::nullopt;
    }
    const auto first = static_cast<const unsigned char *>(needle)[0];
    const size_t last = offset + searchSpace - needle_len;
    size_t position = offset;
    while (position <= last)
    {
        const auto found = findChar(first, position, last - position + 1, caseSensitive);
        if (!found)
        {
            return std::nullopt;
        }
        if (compare(needle, needle_len, caseSensitive, *found))
        {
            return found;
        }
        // Advance past a failed candidate so repeated prefix characters cannot stall the search.
        position = *found + 1;
    }
    return std::nullopt;
}

std::optional<size_t> B_Base::find(const std::list<std::string> &needles, std::string &needleFound, bool caseSensitive, const size_t &offset, const size_t &searchSpace)
{
    needleFound.clear();
    for (const std::string &needle : needles)
    {
        std::optional<size_t> f = find(needle.c_str(), needle.size(), caseSensitive, offset, searchSpace);
        if (f != std::nullopt)
        {
            needleFound = needle;
            return f;
        }
    }
    return std::nullopt;
}

size_t B_Base::size()
{
    return m_containerBytes;
}

bool B_Base::isNull()
{
    return size() == 0;
}

size_t B_Base::getMaxSize() const
{
    return m_maxSize;
}

void B_Base::setMaxSize(const size_t &value)
{
    m_maxSize = value;
}

size_t B_Base::getSizeLeft()
{
    // ERR
    if (size() > m_maxSize)
    {
        return 0;
    }
    // DEF
    return m_maxSize - size();
}

void B_Base::reduceMaxSizeBy(const size_t &value)
{
    if (value > getMaxSize())
    {
        throw std::runtime_error("Don't reduce the max size with a value greater than the current max.");
    }

    setMaxSize(getMaxSize() - value);
}

bool B_Base::streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out)
{
    std::optional<size_t> x = appendTo(*out);
    return x != std::nullopt && out->writeStatus.succeed;
}

std::optional<size_t> B_Base::writeImpl(const void *buf, const size_t &count)
{
    if (!count)
    {
        writeStatus += 0;
        return 0;
    }
    std::optional<size_t> bytesAppended = append(buf, count);

    if (bytesAppended != std::nullopt)
    {
        writeStatus += bytesAppended.value();
    }
    else
    {
        writeStatus += -1;
    }
    return bytesAppended;
}

std::shared_ptr<Mantids30::Core::Memory::Containers::B_Base> B_Base::copyToFS(const std::string &fileName, bool deleteFileOnDestruction)
{
    std::shared_ptr<B_MMAP> mmapbc = std::make_shared<B_MMAP>();
    mmapbc->setFsBaseFileName(m_fsBaseFileName);
    mmapbc->setFsDirectoryPath(m_fsDirectoryPath);
    // TODO: passing filename when passing from chunks/mem to file.
    if (!mmapbc->referenceFile(fileName))
    {
        return nullptr;
    }
    mmapbc->setDeleteFileOnDestruction(deleteFileOnDestruction);

    // dump this container into the mmaped binary container.
    std::optional<size_t> bytesAppended = appendTo(*mmapbc);
    if (!bytesAppended || *bytesAppended != size())
    {
        mmapbc->setDeleteFileOnDestruction(true);
        return nullptr;
    }

    return mmapbc;
}

std::string B_Base::getFsDirectoryPath() const
{
    return m_fsDirectoryPath;
}

void B_Base::setFsDirectoryPath(const std::string &value)
{
    m_fsDirectoryPath = value;
}

std::string B_Base::getFsBaseFileName() const
{
    return m_fsBaseFileName;
}

void B_Base::setFsBaseFileName(const std::string &value)
{
    m_fsBaseFileName = value;
}

std::string B_Base::getCurrentFileName() const
{
    return "";
}

void B_Base::incContainerBytesCount(const size_t &i)
{
    setContainerBytes(size() + i);
}

void B_Base::decContainerBytesCount(const size_t &i)
{
    setContainerBytes(size() - i);
}

void B_Base::setContainerBytes(const size_t &value)
{
    m_containerBytes = value;
}

std::optional<size_t> B_Base::copyToStreamUsingCleanVector(std::ostream &streamOut, const std::vector<BinaryContainerChunk> &copyChunks)
{
    size_t dataCopied = 0;

    // Appending mode.
    for (auto &copyChunk : copyChunks)
    {
        streamOut.write(copyChunk.rodata, copyChunk.rosize);
        if (!streamOut)
        {
            // Report failbit as well as badbit; either state means the write did not complete.
            return std::nullopt;
        }
        dataCopied += copyChunk.rosize;
    }

    return dataCopied;
}

std::optional<size_t> B_Base::copyToStreamableObjectUsingCleanVector(StreamableObject &bcOut, const std::vector<BinaryContainerChunk> &copyChunks)
{
    size_t dataCopied = 0;

    // Appending mode.
    for (const auto &copyChunk : copyChunks)
    {
        if (!bcOut.writeFullStream(copyChunk.rodata, copyChunk.rosize))
        {
            return std::nullopt;
        }

        dataCopied = safe_add(copyChunk.rosize, dataCopied);
    }

    return dataCopied;
}
