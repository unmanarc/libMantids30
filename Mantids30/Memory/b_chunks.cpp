#include "b_chunks.h"
#include <string.h>

using namespace Mantids30::Memory::Containers;


B_Chunks::B_Chunks()
{
    m_storeMethod = BC_METHOD_CHUNKS;

    m_mmapContainer = nullptr;
    m_maxChunkSize = 64*KB_MULT; // 64Kb.
    m_maxChunks = 256*KB_MULT; // 256K chunks (16Gb of RAM)

    // Don't go to FS by default.
    m_maxContainerSizeUntilGoingToFS = 0;
    //maxContainerSizeUntilGoingToFS = 32*MB_MULT; // 32Mb.

    B_Chunks::clear2();
}

B_Chunks::B_Chunks(const std::string &str)
{
    B_Chunks();
    append(str.c_str(),str.size());
}

B_Chunks::~B_Chunks()
{
    B_Chunks::clear2();
}

void B_Chunks::setMaxContainerSizeUntilGoingToFS(const uint64_t &value)
{
    m_maxContainerSizeUntilGoingToFS = value;
}

uint64_t B_Chunks::getMaxContainerSizeUntilGoingToFS() const
{
    return m_maxContainerSizeUntilGoingToFS;
}

bool B_Chunks::isUsingFiles()
{
    return m_mmapContainer!=nullptr;
}

size_t B_Chunks::getMaxChunks() const
{
    return m_maxChunks;
}

void B_Chunks::setMaxChunks(const size_t &value)
{
    if (value<UINT32_MAX)
    {
        m_maxChunks = value;
    }
    else
    {
        m_maxChunks = UINT32_MAX;
    }
}

void B_Chunks::setMaxChunkSize(const uint32_t &value)
{
    m_maxChunkSize = value;
}

uint64_t B_Chunks::size() const
{
    if (m_mmapContainer) return m_mmapContainer->size();
    //std::cout << "B_Chunks::  Getting size() " << containerBytes << std::endl << std::flush;
    return m_containerBytes;
}

std::pair<bool, uint64_t> B_Chunks::truncate2(const uint64_t &bytes)
{
    if (m_mmapContainer)
    {
        return m_mmapContainer->truncate(bytes);
    }

    size_t ival = I_Chunk_GetPosForOffset(bytes);
    if (ival==MAX_SIZE_T)
        return std::make_pair(false,(uint64_t)0);

    m_chunksVector[ival].truncate(bytes);

    for (int i=static_cast<int>(ival);i<static_cast<int>(m_chunksVector.size());i++)
    {
        m_chunksVector[ival].destroy();
        m_chunksVector.erase(m_chunksVector.begin()+i);
    }

    return std::make_pair(true,size());
}

std::pair<bool, uint64_t> B_Chunks::append2(const void *buf, const uint64_t &roLen, bool prependMode)
{
    uint64_t len = roLen;
    if (m_mmapContainer && !prependMode)
    {
        return m_mmapContainer->append(buf,len);
    }
    if (m_mmapContainer && prependMode)
    {
        return m_mmapContainer->prepend(buf,len);
    }

    std::pair<bool,uint64_t> appendedBytes = std::make_pair(true,0);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(len,size()))
    {
        return std::make_pair(false,(uint64_t)0);
    }

    if (m_maxContainerSizeUntilGoingToFS!=0 && len+size() > m_maxContainerSizeUntilGoingToFS)
    {
        m_mmapContainer = copyToFS("",true);
        if (!m_mmapContainer)
        {
            return std::make_pair(false,(uint64_t)0);
        }
        clearChunks(); // Clear this container leaving the mmap intact...
        if (prependMode)
        {
            return m_mmapContainer->prepend(buf,len);
        }
        else
        {
            return m_mmapContainer->append(buf,len);
        }
    }

    while (len)
    {
        uint64_t chunkSize = len<m_maxChunkSize? len:m_maxChunkSize;

        // Don't create new chunks if we can't handle them.
        if (m_chunksVector.size()+1>m_maxChunks)
        {
            appendedBytes.first = false;
            return appendedBytes;
        }

        ///////////////////////////////////////////////////////
        // Copy memory:
        BinaryContainerChunk bcc;
        if (!bcc.copy(buf,chunkSize))
        {
            if (prependMode)
            {
                recalcChunkOffsets();
            }
            appendedBytes.first = false;
            return appendedBytes; // not enough memory.
        }

        ///////////////////////////////////////////////////////
        // Append or prepend the data.
        if (!prependMode)
        {
            if (!m_chunksVector.size())
            {
                bcc.offset = 0;
            }
            else
            {
                bcc.offset = m_chunksVector[m_chunksVector.size()-1].nextOffset();
            }
            m_chunksVector.push_back(bcc);
        }
        else
        {
            m_chunksVector.emplace( m_chunksVector.begin(), bcc );
        }

        ///////////////////////////////////////////////////////
        // Update the size of the container
        incContainerBytesCount(bcc.size);
        appendedBytes.second+=bcc.size;

        ////////////////////////////
        // local counters update...
        buf=((const char *)buf)+chunkSize;
        len-=chunkSize;
    }

    if (prependMode)
    {
        recalcChunkOffsets();
    }
    return appendedBytes;
}

std::pair<bool,uint64_t> B_Chunks::displace2(const uint64_t &roBytesToDisplace)
{
    uint64_t bytesToDisplace = roBytesToDisplace;
    if (m_mmapContainer) return m_mmapContainer->displace(bytesToDisplace);

    std::pair<bool,uint64_t> displaced = std::make_pair(true,0);

    while (bytesToDisplace)
    {
        if (!m_chunksVector.size()) return displaced; // not completely displaced

        if (bytesToDisplace >= m_chunksVector[0].size)
        {
            // remove this chunk entirely
            displaced.second += m_chunksVector[0].size;
            bytesToDisplace-=m_chunksVector[0].size;
            decContainerBytesCount(m_chunksVector[0].size);
            m_chunksVector[0].destroy();
            m_chunksVector.erase(m_chunksVector.begin());
        }
        else
        {
            // displace the chunk partially.
            displaced.second += bytesToDisplace;
            m_chunksVector[0].displace(bytesToDisplace);
            decContainerBytesCount(bytesToDisplace);
            bytesToDisplace = 0;
        }
    }
    // Rearrange offsets here.
    recalcChunkOffsets();
    return displaced;
}

bool B_Chunks::clear2()
{
    return clearMmapedContainer() && clearChunks();
}

bool B_Chunks::clearMmapedContainer()
{
    if (m_mmapContainer) delete m_mmapContainer;
    m_mmapContainer = nullptr;
    return true;
}

bool B_Chunks::clearChunks()
{
    for (BinaryContainerChunk bcc : m_chunksVector)
        bcc.destroy();
    m_chunksVector.clear();
    return true;
}

std::pair<bool,uint64_t> B_Chunks::copyToStream2(std::ostream &bc, const uint64_t &roBytes, const uint64_t &roOffset)
{
    uint64_t bytes = roBytes;
    uint64_t offset = roOffset;
    if (m_mmapContainer) return m_mmapContainer->copyToStream(bc,bytes,offset);

    if (!bytes) return std::make_pair(true,0);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,(uint64_t)0);
    // No bytes to copy:
    if (offset>size()) return std::make_pair(false,(uint64_t)0);
    // Request exceed this container.
    if (offset+bytes>size()) bytes = size()-offset;

    uint64_t dataToCopy = bytes;
    std::vector<BinaryContainerChunk> copyChunks;

    // iterate over chunks and put that data on the new bc.
    for (auto & i : m_chunksVector)
    {
        BinaryContainerChunk currentChunk = i;

        // arrange from non-ro elements.
        if (currentChunk.rodata == nullptr)
        {
            currentChunk.rodata = currentChunk.data;
            currentChunk.rosize = currentChunk.size;
        }

        if (offset>0)
        {
            if (offset>currentChunk.rosize)
            {
                // pass this chunk...
                offset-=currentChunk.rosize;
                continue; // chunk consumed.
            }
            else
            {
                currentChunk.rosize-=offset;
                currentChunk.rodata+=offset;
                offset = 0;
            }
        }

        if (!offset)
        {
            currentChunk.rosize = dataToCopy>currentChunk.rosize?currentChunk.rosize:dataToCopy;
            copyChunks.push_back(currentChunk);
            dataToCopy-=currentChunk.rosize;
            if (!dataToCopy) break; // :)
        }
    }

    return std::make_pair(true,copyToStreamUsingCleanVector(bc,copyChunks));
}

std::pair<bool,uint64_t> B_Chunks::copyTo2(StreamableObject &bc, Streams::StreamableObject::Status & wrStatUpd, const uint64_t &roBytes, const uint64_t &roOffset)
{
    uint64_t bytes = roBytes;
    uint64_t offset = roOffset;
    if (m_mmapContainer) return m_mmapContainer->appendTo(bc,wrStatUpd,bytes,offset);

    if (!bytes) return std::make_pair(true,0);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,(uint64_t)0);
    // No bytes to copy:
    if (offset>size()) return std::make_pair(false,(uint64_t)0);
    // Request exceed this container.
    if (offset+bytes>size()) bytes = size()-offset;

    uint64_t dataToCopy = bytes;
    std::vector<BinaryContainerChunk> copyChunks;

    // iterate over chunks and put that data on the new bc.
    for (auto & i : m_chunksVector)
    {
        BinaryContainerChunk currentChunk = i;

        // arrange from non-ro elements.
        if (currentChunk.rodata == nullptr)
        {
            currentChunk.rodata = currentChunk.data;
            currentChunk.rosize = currentChunk.size;
        }

        if (offset>0)
        {
            if (offset>currentChunk.rosize)
            {
                // pass this chunk...
                offset-=currentChunk.rosize;
                continue; // chunk consumed.
            }
            else
            {
                currentChunk.rosize-=offset;
                currentChunk.rodata+=offset;
                offset = 0;
            }
        }

        if (!offset)
        {
            currentChunk.rosize = dataToCopy>currentChunk.rosize?currentChunk.rosize:dataToCopy;
            copyChunks.push_back(currentChunk);
            dataToCopy-=currentChunk.rosize;
            if (!dataToCopy) break; // :)
        }
    }

    return std::make_pair(true,copyToSOUsingCleanVector(bc,copyChunks,wrStatUpd));
}

std::pair<bool, uint64_t> B_Chunks::copyOut2(void *buf, const uint64_t &roBytes, const uint64_t &offset)
{
    uint64_t bytes = roBytes;
    if (m_mmapContainer) return m_mmapContainer->copyOut(buf,bytes,offset);

    uint64_t copiedBytes = 0;

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,bytes)) return std::make_pair(false,(uint64_t)0);

    // No bytes to copy:
    if (!bytes) return std::make_pair(true,0);

    // out of bounds (fail to copy):
    if (offset+bytes>size()) return std::make_pair(false,(uint64_t)0);

    ////////////////////////////////////

    size_t icurrentChunk = I_Chunk_GetPosForOffset(offset);
    if (icurrentChunk==MAX_SIZE_T) return std::make_pair(false,(uint64_t)0);

    BinaryContainerChunk currentChunk = m_chunksVector[icurrentChunk];
    currentChunk.moveToOffset(offset);

    while (bytes)
    {
        if (bytes > currentChunk.size)
        {
            // copy the whole chunk.
            memcpy(buf,currentChunk.data,currentChunk.size);

            // repos the vars...
            copiedBytes += currentChunk.size;
            bytes-=currentChunk.size;
            buf=((char *)buf)+currentChunk.size;
        }
        else if (bytes <= currentChunk.size)
        {
            // Copy part of the chunk.
            memcpy(buf,currentChunk.data,bytes);

            // ends here.
            return std::make_pair(true,copiedBytes+bytes);
        }

        // proceed to the next chunk...
        if (icurrentChunk==m_chunksVector.size()-1) break;
        icurrentChunk++;
        currentChunk = m_chunksVector[icurrentChunk];
    }

    return std::make_pair(true,copiedBytes);
}

bool B_Chunks::compare2(const void *buf, const uint64_t &len, bool caseSensitive, const uint64_t &roOffset)
{
    uint64_t offset = roOffset;
    if (m_mmapContainer) return m_mmapContainer->compare(buf,len,caseSensitive,offset);

    // Offset:bytes will overflow...
    if (CHECK_UINT_OVERFLOW_SUM(offset,len)) return false;

    // No bytes to copy:
    if (!len) return true;

    // out of bounds (fail to compare):
    if (offset+len>size()) return false;

    /////////////////////////////
    uint64_t dataToCompare = len, dataCompared = 0;

    // iterate over chunks and put that data on the new bc.

    //for (auto & i : chunksVector)
    size_t vpos=0, vsize = m_chunksVector.size();
    for (  ; vpos<vsize ; vpos++ )
    {
        BinaryContainerChunk * i = &(m_chunksVector[vpos]);
        BinaryContainerChunk currentChunk = *i; // copy the chunk..

        // TODO: CHECK.
        // if offset is >0...
        if (offset>0)
        {
            if (offset>i->size)
            {
                // pass this chunk...
                offset-=i->size;
                continue; // chunk consumed.
            }
            else
            {
                currentChunk.size-=offset;
                currentChunk.data+=offset;

                offset = 0;
            }
        }

        if (!offset)
        {
            size_t currentChunkSize = dataToCompare>currentChunk.size?currentChunk.size:dataToCompare;

            if (Mantids30::Helpers::Mem::memicmp2(currentChunk.data, buf,currentChunkSize,caseSensitive)) return false; // does not match!

            dataToCompare-=currentChunkSize;
            dataCompared+=currentChunkSize;
            buf=((const char *)buf)+currentChunkSize;

            // Ended.!
            if (!dataToCompare) return true;
        }
    }

    // TODO: check the logic here
    // If there is any data to compare left, return false.
    return dataToCompare==0;
}




// TODO: ICASE
std::pair<bool, uint64_t> B_Chunks::findChar(const int &c, const uint64_t &roOffset, uint64_t searchSpace, bool caseSensitive)
{
    if (caseSensitive && !(c>='A' && c<='Z') && !(c>='a' && c<='z') )
        caseSensitive = false;

    uint64_t offset = roOffset;
    if (m_mmapContainer) return m_mmapContainer->findChar(c,offset);

    ///////////////////////////
    size_t currentSize = size();
    if (CHECK_UINT_OVERFLOW_SUM(offset,searchSpace)) return std::make_pair(false,std::numeric_limits<uint64_t>::max());
    // out of bounds (fail to compare):
    if (offset>currentSize || offset+searchSpace>currentSize) return std::make_pair(false,std::numeric_limits<uint64_t>::max());

    size_t retpos = 0;
    size_t vpos=0, vsize = m_chunksVector.size();
    for (  ; vpos<vsize ; vpos++ )
    {
        BinaryContainerChunk * originalChunk = &(m_chunksVector[vpos]);
        BinaryContainerChunk currentChunk = *originalChunk; // copy the chunk..

        // if offset is >0...
        if (offset>0)
        {
            if (offset>originalChunk->size)
            {
                // pass this chunk...
                offset-=originalChunk->size;
                // chunk discarded.
                retpos+=originalChunk->size;

                continue; // chunk consumed.
            }
            else
            {
                currentChunk.size-=offset;
                currentChunk.data+=offset;

                offset = 0;
            }
        }

        if (offset==0)
        {
            char * pos = nullptr;

            if (!caseSensitive)
                pos = (char *)memchr(currentChunk.data, c, searchSpace>currentChunk.size? currentChunk.size : searchSpace);
            else
            {
                char *pos_upper = (char *)memchr(currentChunk.data, std::toupper(c), searchSpace>currentChunk.size? currentChunk.size : searchSpace);
                char *pos_lower = (char *)memchr(currentChunk.data, std::tolower(c), searchSpace>currentChunk.size? currentChunk.size : searchSpace);

                if      (pos_upper && pos_lower && pos_upper<=pos_lower) pos = pos_upper;
                else if (pos_upper && pos_lower && pos_lower<pos_upper) pos = pos_lower;
                else if (pos_upper) pos = pos_upper;
                pos = pos_lower;
            }

            if (pos)
            {
                // report the position.
                return std::make_pair(true,(uint64_t)(pos-(originalChunk->data))+retpos);
            }

            if (searchSpace>currentChunk.size)
                searchSpace-=currentChunk.size;
            else
                 return std::make_pair(false,(uint64_t)0);
        }

        // chunk discarded.
        retpos+=originalChunk->size;
    }
    return std::make_pair(false,(uint64_t)0);
}

void B_Chunks::recalcChunkOffsets()
{
    unsigned long long currentOffset = 0;
    size_t vpos=0, vsize = m_chunksVector.size();
    for ( BinaryContainerChunk * i = &(m_chunksVector[vpos]) ; vpos<vsize ; vpos++ )
    {
        i->offset = currentOffset;
        currentOffset = i->nextOffset();
    }
}

size_t B_Chunks::I_Chunk_GetPosForOffset(const uint64_t &offset,  size_t  curpos, size_t curmax, size_t curmin)
{
    // The Search Algorithm!
    if (!m_chunksVector.size()) return MAX_SIZE_T;

    // Boundaries definition:
    if (curpos == MAX_SIZE_T)
        curpos = m_chunksVector.size()==1?0:(m_chunksVector.size()/2)-1;
    if (curmax == MAX_SIZE_T)
        curmax = m_chunksVector.size()==1?0:m_chunksVector.size()-1;
    if (curmin == MAX_SIZE_T)
        curmin = 0;

    ////////////////////////////////////////////////////////////////
    // Verify current chunk. (VERIFICATION)
    if (m_chunksVector[curpos].containsOffset(offset))
    {
        return curpos;
    }
    ////////////////////////////////////////////////////////////////

    // If not found, continue searching:
    if ( offset < m_chunksVector[curpos].offset )
    {
        // search down. (from curmin to curpos-1)
        if (curpos == curmin) return MAX_SIZE_T; // Not any element down.
        ///////////////
        curmax = curpos-1;
        return I_Chunk_GetPosForOffset(offset, curmin==curmax? curmin : curmin+((curmax+1-curmin)/2)-(curmax-curmin)%2, curmax, curmin );
    }
    else
    {
        // search up. (from curpos+1 to curmax)
        if (curpos == curmax) return MAX_SIZE_T; // Not any element up.
        curmin = curpos+1;
        return I_Chunk_GetPosForOffset(offset, curmin==curmax? curmin : curmin+((curmax+1-curmin)/2)-(curmax-curmin)%2, curmax, curmin );
    }
}

B_MMAP *B_Chunks::getMmapContainer() const
{
    return ((B_MMAP *)m_mmapContainer);
}

