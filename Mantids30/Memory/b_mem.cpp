#include "b_mem.h"

using namespace Mantids30::Memory::Containers;

B_MEM::B_MEM(const void *buf, const uint32_t & len)
{
    m_storeMethod = BC_METHOD_MEM;
    m_readOnly = true;
    setContainerBytes(0);
    B_MEM::clear2();
    if (buf && len)
    {
        reference(buf,len);
    }
}

B_MEM::~B_MEM()
{
    B_MEM::clear2();
}

void B_MEM::reference(const void *buf, const uint32_t &len)
{
    clear();
    linearMem = (static_cast<const char *>(buf));
    m_readOnly = true;
    setContainerBytes(len);
}


std::pair<bool, uint64_t> B_MEM::findChar(const int &charInt, const uint64_t &offset, uint64_t searchSpace, bool caseSensitive)
{
    // Convert the integer representation of the character to an unsigned char
    unsigned char ch = static_cast<unsigned char>(charInt);

    // If case sensitivity is disabled and the character is not a letter, enable case sensitivity
    if (!caseSensitive && !((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')))
        caseSensitive = true;

    // Get the current size of the memory container
    size_t currentSize = size();

    // If the container is empty, return not found
    if (!currentSize)
        return std::make_pair(false, static_cast<uint64_t>(0));

    // Check for potential overflow when calculating offset + searchSpace
    if (CHECK_UINT_OVERFLOW_SUM(offset, searchSpace))
        return std::make_pair(false, std::numeric_limits<uint64_t>::max());

    // If the specified range exceeds the container size, return not found
    if (offset + searchSpace > currentSize)
        return std::make_pair(false, std::numeric_limits<uint64_t>::max());

    // If no specific search space is provided, search until the end of the container
    if (searchSpace == 0)
        searchSpace = currentSize - offset;

    const char *cPos = nullptr;
    
    // Perform case-sensitive or case-insensitive search based on the flag
    if (caseSensitive)
    {
        // Use memchr for a direct comparison in case-sensitive mode
        cPos = static_cast<const char*>(memchr(linearMem + offset, ch, searchSpace));
    }
    else
    {
        // Convert the character to both upper and lower cases for comparison
        unsigned char upperCh = static_cast<unsigned char>(std::toupper(ch));
        unsigned char lowerCh = static_cast<unsigned char>(std::tolower(ch));

        // Find positions of both uppercase and lowercase versions
        const char *posUpper = static_cast<const char*>(memchr(linearMem + offset, upperCh, searchSpace));
        const char *posLower = static_cast<const char*>(memchr(linearMem + offset, lowerCh, searchSpace));

        // Determine the first occurrence position if both are found
        if (posUpper && posLower)
            cPos = (posUpper < posLower) ? posUpper : posLower;
        else if (posUpper)
            cPos = posUpper;
        else
            cPos = posLower;
    }

    // If the character was not found, return not found
    if (!cPos)
        return std::make_pair(false, static_cast<uint64_t>(0));

    // Calculate and return the position of the found character
    return std::make_pair(true, static_cast<uint64_t>(cPos - linearMem));
}


std::pair<bool, uint64_t> B_MEM::truncate2(const uint64_t &bytes)
{
    setContainerBytes(bytes);
    return std::make_pair(true,size());
}

std::pair<bool, uint64_t> B_MEM::append2(const void *, const uint64_t &, bool )
{
    return std::make_pair(false,static_cast<uint64_t>(0));
}

std::pair<bool, uint64_t> B_MEM::displace2(const uint64_t &bytesToDisplace)
{
    if (bytesToDisplace>size()) return std::make_pair(false,static_cast<uint64_t>(0));

    linearMem+=bytesToDisplace;
    decContainerBytesCount(bytesToDisplace);

    return std::make_pair(true,bytesToDisplace);
}

bool B_MEM::clear2()
{
    linearMem = nullptr;
    setContainerBytes(0);
    return true;
}

std::pair<bool,uint64_t> B_MEM::copyToStream2(std::ostream &bc, const uint64_t &roBytes, const uint64_t &offset)
{
    size_t currentBytes = size();
    uint64_t bytes = roBytes;
    // No bytes to copy.
    if (!bytes) return std::make_pair(true,0);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,static_cast<uint64_t>(0));
    // No bytes to copy:
    if (offset>currentBytes) return std::make_pair(false,static_cast<uint64_t>(0));
    // Request exceed this container, bytes should only copy what's right...
    if (offset+bytes>currentBytes) bytes = currentBytes-offset;

    uint64_t dataToCopy = bytes;
    std::vector<BinaryContainerChunk> copyChunks;

    const char * transmitMem = linearMem+offset;

    while (dataToCopy)
    {
        BinaryContainerChunk bcx;

        bcx.rosize = dataToCopy>64*KB_MULT?64*KB_MULT:dataToCopy;
        bcx.rodata = transmitMem;

        copyChunks.push_back(bcx);

        transmitMem+=bcx.rosize;
        dataToCopy-=bcx.rosize;
    }

    return std::make_pair(true,copyToStreamUsingCleanVector(bc,copyChunks));
}

std::pair<bool,uint64_t> B_MEM::copyTo2(StreamableObject &bc, Streams::StreamableObject::Status & wrStatUpd, const uint64_t &roBytes, const uint64_t &offset)
{
    uint64_t bytes = roBytes;
    // No bytes to copy.
    if (!bytes) return std::make_pair(true,0);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,static_cast<uint64_t>(0));
    // No bytes to copy:
    if (offset>size()) return std::make_pair(false,static_cast<uint64_t>(0));
    // Request exceed this container, bytes should only copy what's right...
    if (offset+bytes>size()) bytes = size()-offset;

    uint64_t dataToCopy = bytes;
    std::vector<BinaryContainerChunk> copyChunks;

    const char * transmitMem = linearMem+offset;

    while (dataToCopy)
    {
        BinaryContainerChunk bcx;

        bcx.rosize = dataToCopy>64*KB_MULT?64*KB_MULT:dataToCopy;
        bcx.rodata = transmitMem;

        copyChunks.push_back(bcx);

        transmitMem+=bcx.rosize;
        dataToCopy-=bcx.rosize;
    }

    return  std::make_pair(true,copyToSOUsingCleanVector(bc,copyChunks,wrStatUpd));
}

std::pair<bool,uint64_t> B_MEM::copyOut2(void *buf, const uint64_t &bytes, const uint64_t &offset)
{
    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,static_cast<uint64_t>(0));

    // No bytes to copy:
    if (!bytes) return std::make_pair(true,0);

    // out of bounds (fail to copy):
    if (offset+bytes>size()) return std::make_pair(false,static_cast<uint64_t>(0));

    ////////////////////////////////////

    // error: offset exceed the container size (try another offset)
    if (offset>size()) return std::make_pair(false,static_cast<uint64_t>(0));

    const char * linearMemOffseted = linearMem+offset;
    uint64_t containerBytesOffseted = size()-offset;
    if (containerBytesOffseted==0)
        return std::make_pair(true,0); // no data left to copy. (copy 0 bytes)
    uint64_t copiedBytes = containerBytesOffseted<bytes?containerBytesOffseted:bytes;

    memcpy(buf,linearMemOffseted,copiedBytes);

    return std::make_pair(true,copiedBytes);
}

bool B_MEM::compare2(const void *buf, const uint64_t &len, bool caseSensitive, const uint64_t &offset)
{
    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,len)) return false;

    // No bytes to copy:
    if (!len) return true;

    // out of bounds (fail to compare):
    if (offset+len>size()) return false;

    /////////////////////////////

    const char * memoryToBeCompared = linearMem + offset;
    return !Mantids30::Helpers::Mem::memicmp2(memoryToBeCompared, buf, len,caseSensitive);
}

