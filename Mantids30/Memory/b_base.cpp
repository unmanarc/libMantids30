#include "b_base.h"

#include "Mantids30/Helpers/safeint.h"
#include "b_mmap.h"
#include "b_ref.h"

#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

using namespace Mantids30::Memory::Containers;

B_Base::B_Base()
{
    m_maxSize = std::numeric_limits<size_t>::max();
    m_readOnly = false;
    m_storeMethod = BC_METHOD_NULL;
    clear0();
}

B_Base::~B_Base()
{
    clear0();
}

B_Base &B_Base::operator=(B_Base &bc)
{
    clear();
    bc.appendTo(*this);
    return *this;
}

void B_Base::print(FILE *f)
{
    auto str = toString();
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
        return std::nullopt;

    size_t currentSize = size();

    // New size will overflow the counter...
    if (CHECK_UINT_OVERFLOW_SUM(len, currentSize))
        return std::nullopt;
    // out of bounds, fail.
    if (len + currentSize > m_maxSize)
        return std::nullopt;
    // zero to copy!
    if (!len)
        return std::nullopt;

    // Data modification should pass trough referenced pointer.
    return append2(data, len, false);
}

std::optional<size_t> B_Base::prepend(const void *data, size_t len)
{
    // Read only: can't append nothing.
    if (m_readOnly)
        return std::nullopt;

    size_t currentSize = size();

    // New size will overflow the counter...
    if (CHECK_UINT_OVERFLOW_SUM(len, currentSize))
        return std::nullopt;
    // out of bounds.
    if (len + currentSize > m_maxSize)
        return std::nullopt;
    // zero to copy!
    if (!len)
        return 0;

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
        return std::nullopt;

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
    std::optional<size_t> needlePos = find(needle, needleLenght, false, 0, maxCopySize);

    if (!needlePos)
        return -1;

    // will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(*needlePos, needleLenght))
        return -1;

    if (removeNeedle)
    {
        if ((*needlePos) > maxCopySize)
            return -2;
        appendTo(destination, *needlePos);
    }
    else
    {
        if ((*needlePos + needleLenght) > maxCopySize)
            return -2;
        appendTo(destination, *needlePos + needleLenght);
    }

    return 0;
}

int B_Base::displaceUntil(B_Base &destination, const void *needle, const size_t &needleCount, const size_t &maxCopySize, bool removeNeedle)
{
    int retr = copyUntil(destination, needle, needleCount, maxCopySize, removeNeedle);
    if (retr < 0)
        return retr;
    displace(destination.size() + (removeNeedle ? needleCount : 0));
    return 0;
}

int B_Base::displaceUntil(B_Base &destination, const std::list<std::string> needles, const size_t &maxCopySize, bool removeNeedle)
{
    for (const auto &needle : needles)
    {
        if (!displaceUntil(destination, needle.c_str(), needle.size(), maxCopySize, removeNeedle))
            return 0;
    }
    return -1;
}

// TODO: pass this to shared_ptr
/*
std::list<Memory::Containers::B_Base *> B_Base::referencedSplit(const std::list<std::string> &needles,const size_t &maxSearchSize, const size_t &maxNeedles)
{
    std::list<std::string> skipBegWith;
    return referencedSplit2(needles,skipBegWith,maxSearchSize,maxNeedles);
}

std::list<Memory::Containers::B_Base *> B_Base::referencedSplit2(const std::list<std::string> &needles, const std::list<std::string> &skipBegWith, const size_t &maxSearchSize, const size_t &roMaxNeedles)
{
    size_t maxNeedles = roMaxNeedles;
    std::list<Memory::Containers::B_Base *> x;

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

void B_Base::freeSplitList(std::list<Memory::Containers::B_Base *> x)
{
    for (auto i : x)
    {
        if (i)
            delete i;
    }
}

std::optional<size_t> B_Base::copyToStream(std::ostream &out, size_t bytes, const size_t &offset)
{
    size_t currentSize = size();

    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
            return std::nullopt;      // invalid size.
        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
        return std::nullopt;

    // No bytes to copy:
    if (!bytes)
        return 0;

    // out of bounds for sure.
    if (offset > currentSize)
        return std::nullopt;

    // out of bounds (last bytes):
    if (offset + bytes > currentSize)
        bytes = currentSize - offset;

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
            out.writeStatus+=-1;
            return std::nullopt; // invalid pos.
        }
        bytesToCopy = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytesToCopy))
    {
        out.writeStatus+=-1;
        return std::nullopt; // invalid pos.
    }

    // No bytes to copy:
    if (!bytesToCopy)
        return 0; // Zero bytes to copy.

    // out of bounds for sure.
    if (offset > currentSize)
    {
        out.writeStatus+=-1;
        return std::nullopt; // invalid pos.
    }

    // out of bounds (last bytes):
    if (offset + bytesToCopy > currentSize)
        bytesToCopy = currentSize - offset;

    ////////////////////////////////////
    return copyToStreamableObject2(out, bytesToCopy, offset);
}

std::optional<size_t> B_Base::copyOut(void *buf, size_t bytes, const size_t &offset)
{
    size_t currentSize = size();

    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
            return std::nullopt; // out of bounds.

        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
        return std::nullopt;

    // No bytes to copy:
    if (!bytes)
        return 0;

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
        return std::nullopt;

    ////////////////////////////////////
    return copyToBuffer2(buf, bytes, offset);
}

std::optional<size_t> B_Base::copyToString(std::string &outStr, size_t bytes, const size_t &roOffset)
{
    size_t currentSize = size();

    size_t offset = roOffset;
    if (bytes == std::numeric_limits<size_t>::max())
    {
        if (offset > currentSize)
            return std::nullopt; // out of bounds

        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
        return std::nullopt;

    // No bytes to copy:
    if (!bytes)
        return 0;

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
        return std::nullopt;

    char outmem[8192];
    ////////////////////////////////////
    while (bytes)
    {
        size_t copyBytes = std::min(bytes, static_cast<size_t>(8192));
        bytes -= copyBytes;

        std::optional<size_t> outBytes = copyOut(outmem, copyBytes, offset);

        if (outBytes != std::nullopt)
            outStr.append(outmem, outBytes.value());
        else
            return outStr.size();

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
            return std::nullopt;      // negative bytes copied.
        bytes = currentSize - offset; // whole container bytes copied.
    }

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, bytes))
        return std::nullopt; // negative bytes copied.

    // No bytes to copy:
    if (!bytes)
        return r;

    // out of bounds (fail to copy):
    if (offset + bytes > currentSize)
        return std::nullopt; // negative bytes copied.

    char outmem[8192];
    ////////////////////////////////////
    while (bytes)
    {
        size_t copyBytes = std::min(bytes, static_cast<size_t>(8192));
        bytes -= copyBytes;
        std::optional<size_t> outBytes = copyOut(outmem, copyBytes, offset);

        if (outBytes != std::nullopt)
            r.append(outmem, outBytes.value());
        else
            return r;

        offset += copyBytes;
    }

    return r;
}

std::optional<uint64_t> B_Base::toUInt64(int base, const size_t &bytes, const size_t &offset)
{
    auto s = toString(bytes, offset);
    if (!s)
        return std::nullopt;

    char *endPtr;
    uint64_t result = strtoull(s->c_str(), &endPtr, base);

    // Check for conversion errors
    if (*endPtr != '\0' || endPtr == s->c_str())
        return std::nullopt;

    return result;
}

std::optional<uint32_t> B_Base::toUInt32(int base, const size_t &bytes, const size_t &offset)
{
    auto s = toString(bytes, offset);
    if (!s)
        return std::nullopt;

    char *endPtr;
    uint32_t result = strtoul(s->c_str(), &endPtr, base);

    // Check for conversion errors
    if (*endPtr != '\0' || endPtr == s->c_str())
        return std::nullopt;

    return result;
}

bool B_Base::compare(const std::string &cmpString, bool caseSensitive, const size_t &offset)
{
    if (cmpString.size() != size())
        return false;
    return compare(cmpString.c_str(), cmpString.size(), caseSensitive, offset);
}

bool B_Base::compare(const void *mem, const size_t &len, bool caseSensitive, const size_t &offset)
{
    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, len))
        return false;

    // No bytes to copy:
    //if (!len || offset+len==size())
    return true;
    // No data to compare...
    if (!len)
        return true;

    // out of bounds (fail to compare):
    if (offset + len > size())
        return false;

    /////////////////////////////

    return compare2(mem, len, caseSensitive, offset);
}

std::optional<size_t> B_Base::find(const void *needle, const size_t &needle_len, bool caseSensitive, const size_t &offset, size_t searchSpace)
{
    size_t currentSize = size();

    char *c_needle = (char *) needle;

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, needle_len))
        return std::nullopt;

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset, searchSpace))
        return std::nullopt;

    if (offset > currentSize)
        return std::nullopt;
    if (searchSpace == 0)
        searchSpace = currentSize - offset;
    if (searchSpace == 0)
        return std::nullopt;

    // not enough search space to found anything...
    if (searchSpace < needle_len)
        return std::nullopt;

    // nothing to be found... first position
    if (needle_len == 0)
        return 0;

    size_t currentOffset = offset;

    std::optional<size_t> pos = findChar(c_needle[0], currentOffset, searchSpace, caseSensitive);
    while (pos != std::nullopt) // char detected...
    {
        /////////////////////////////////
        size_t displacement = pos.value() - currentOffset;

        currentOffset += displacement;
        searchSpace -= displacement;

        if (compare2(needle, needle_len, caseSensitive, currentOffset))
            return currentOffset;

        // no left space to consume.
        if (searchSpace == 0)
            break;

        // skip char found.
        currentOffset += 1;
        searchSpace -= 1;

        pos = findChar(c_needle[0], currentOffset, searchSpace, caseSensitive);
    }

    // not found
    return std::nullopt;

    /*
    if (searchSpace == 0) searchSpace = currentSize;

    //std::cout << "B_Base::find  Getting size() " << containerBytes << std::endl << std::flush;

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,needle_len)) 
        return std::numeric_limits<size_t>::max();

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,searchSpace)) 
        return std::numeric_limits<size_t>::max();

    // not enough search space to found anything...
    if (searchSpace<needle_len) 
        return std::numeric_limits<size_t>::max();

    // nothing to found... first position
    if (needle_len == 0) 
        return 0;

    // Reduce the needle size from the search space (optimization)
    searchSpace=searchSpace-needle_len+1;

    // calc search space available
    size_t availableSearchSpace= (currentSize-needle_len+1);

    // Verify if offset is appliable (bad offset)
    if (availableSearchSpace<offset) 
        return std::numeric_limits<size_t>::max();

    // Apply offset:
    availableSearchSpace-=offset;

    // If searchSpace is lesser than available bytes... use searchSpace size instead.
    if (searchSpace<availableSearchSpace) availableSearchSpace=searchSpace;

    // start comparing at offset...
    for (size_t currentOffset=offset; currentOffset<availableSearchSpace; currentOffset++)
    {
        if (CHECK_UINT_OVERFLOW_SUM(currentOffset,needle_len)) std::numeric_limits<size_t>::max(); // ERROR...
        if (compare2(needle,needle_len,caseSensitive,currentOffset)) 
        return currentOffset;
    }

    // TODO: found boolean flag instead.

    return std::numeric_limits<size_t>::max(); // not found.*/
}

std::optional<size_t> B_Base::find(const std::list<std::string> &needles, std::string &needleFound, bool caseSensitive, const size_t &offset, const size_t &searchSpace)
{
    needleFound = "";
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
        return 0;
    // DEF
    return m_maxSize - size();
}

void B_Base::reduceMaxSizeBy(const size_t &value)
{
    if (value > getMaxSize())
        throw std::runtime_error("Don't reduce the max size with a value greater than the current max.");

    setMaxSize(getMaxSize() - value);
}

bool B_Base::streamTo(Memory::Streams::StreamableObject *out)
{
    std::optional<size_t> x = appendTo(*out);

    if (x == std::nullopt || !out->writeStatus.succeed)
    {
        return false;
    }

    bool y = out->writeEOF();
    return y && out->writeStatus.succeed;
}

std::optional<size_t> B_Base::write(const void *buf, const size_t &count)
{
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

std::shared_ptr<Mantids30::Memory::Containers::B_Base> B_Base::copyToFS(const std::string &fileName, bool deleteFileOnDestruction)
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

std::optional<size_t> B_Base::copyToStreamUsingCleanVector(std::ostream &streamOut, std::vector<BinaryContainerChunk> copyChunks)
{
    size_t dataCopied = 0;

    // Appending mode.
    for (size_t i = 0; i < copyChunks.size(); i++)
    {
        streamOut.write(copyChunks[i].rodata, copyChunks[i].rosize);
        if (streamOut.bad()) 
        {
            // Return nullopt if write fails.
            return std::nullopt;
        }
        dataCopied += copyChunks[i].rosize;
    }

    return dataCopied;
}

std::optional<size_t> B_Base::copyToStreamableObjectUsingCleanVector(StreamableObject &bcOut, std::vector<BinaryContainerChunk> copyChunks)
{
    size_t dataCopied = 0;

    // Appending mode.
    for (size_t i = 0; i < copyChunks.size(); i++)
    {
        std::optional<size_t> bytesWritten = bcOut.write(copyChunks[i].rodata, copyChunks[i].rosize);

        if (bytesWritten == std::nullopt || !bcOut.writeStatus.succeed)
        {
            return std::nullopt;
        }

        dataCopied = safeAdd(bytesWritten.value(), dataCopied);
    }

    return dataCopied;
}
