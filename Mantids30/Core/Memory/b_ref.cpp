#include "b_ref.h"

using namespace Mantids30::Core::Memory::Containers;

B_Ref::B_Ref(B_Base *bc, const size_t &offset, const size_t &maxBytes)
{
    m_storageMethod = StorageMethod::BCREF;
    referencedBC = nullptr;

    B_Ref::clear2();

    if (bc)
    {
        reference(bc, offset, maxBytes);
    }
}

B_Ref::~B_Ref()
{
    B_Ref::clear2();
}

B_Base *B_Ref::getReferencedBC() const
{
    return referencedBC;
}

bool B_Ref::reference(B_Base *bc, const size_t &offset, const size_t &maxBytes)
{
    // Reject invalid sources before storing a reference or accessing their size.
    if (!bc || bc == this || offset > bc->size())
    {
        return false;
    }

    m_readOnly = true; // :p

    referencedBC = bc;
    referencedOffset = offset;
    referencedMaxBytes = maxBytes;

    return true;
}

size_t B_Ref::size()
{
    // The source may shrink after the reference is created; avoid unsigned size underflow.
    if (!referencedBC || referencedOffset >= referencedBC->size())
    {
        return 0;
    }
    size_t availableBytes = referencedBC->size() - referencedOffset; // Available bytes.

    if (referencedMaxBytes == std::numeric_limits<size_t>::max() || referencedMaxBytes > availableBytes)
    {
        return availableBytes;
    }
    else
    {
        return referencedMaxBytes;
    }
}

std::optional<size_t> B_Ref::findChar(const int &c, const size_t &offset, size_t searchSpace, bool caseSensitive)
{
    // Restrict the delegated search to this view and translate its result back to a local offset.
    const size_t currentSize = size();
    if (!referencedBC || offset >= currentSize || searchSpace > currentSize - offset)
    {
        return std::nullopt;
    }
    if (searchSpace == 0)
    {
        searchSpace = currentSize - offset;
    }
    auto position = referencedBC->findChar(c, referencedOffset + offset, searchSpace, caseSensitive);
    return position ? std::optional<size_t>(*position - referencedOffset) : std::nullopt;
}

std::optional<size_t> B_Ref::truncate2(const size_t &bytes)
{
    referencedMaxBytes = bytes;
    return bytes;
}

std::optional<size_t> B_Ref::append2(const void *buf, const size_t &len, bool prependMode)
{
    if (!referencedBC)
    {
        return std::nullopt; // CANT APPEND TO NOTHING.
    }

    if (prependMode)
    {
        return referencedBC->prepend(buf, len);
    }
    else
    {
        return referencedBC->append(buf, len);
    }
}

std::optional<size_t> B_Ref::displace2(const size_t &bytes)
{
    size_t bytesToDisplace = bytes;
    if (!referencedBC)
    {
        return std::nullopt;
    }

    if (bytesToDisplace > size())
    {
        bytesToDisplace = size();
    }

    if (referencedMaxBytes != std::numeric_limits<size_t>::max())
    {
        referencedMaxBytes -= bytesToDisplace;
    }

    referencedOffset += bytesToDisplace;

    return bytesToDisplace;
}

bool B_Ref::clear2()
{
    if (referencedBC)
    {
        //  delete referencedBC;
        referencedBC = nullptr;
    }
    return true;
}

std::optional<size_t> B_Ref::copyToStream2(std::ostream &out, const size_t &bytes, const size_t &offset)
{
    if (!referencedBC)
    {
        return std::nullopt;
    }

    // CAN'T COPY BEYOND OFFSET.
    if (offset > size())
    {
        return std::nullopt;
    }

    size_t maxBytesToCopy = size() - offset;
    size_t bytesToCopy = std::min(bytes, maxBytesToCopy);

    return referencedBC->copyToStream(out, bytesToCopy, referencedOffset + offset);
}

std::optional<size_t> B_Ref::copyToStreamableObject2(StreamableObject &bc, const size_t &bytes, const size_t &offset)
{
    if (!referencedBC)
    {
        bc.writeStatus += -1;
        return std::nullopt;
    }

    // CAN'T COPY BEYOND OFFSET.
    if (offset > size())
    {
        bc.writeStatus += -1;
        return std::nullopt;
    }

    size_t maxBytesToCopy = size() - offset;
    size_t bytesToCopy = std::min(bytes, maxBytesToCopy);

    return referencedBC->appendTo(bc, bytesToCopy, referencedOffset + offset);
}

std::optional<size_t> B_Ref::copyToBuffer2(void *buf, const size_t &bytes, const size_t &offset)
{
    if (!referencedBC)
    {
        return std::nullopt;
    }

    // CAN'T COPY BEYOND OFFSET.
    if (offset > size())
    {
        return std::nullopt;
    }

    size_t maxBytesToCopy = size() - offset;
    size_t bytesToCopy = std::min(bytes, maxBytesToCopy);

    return referencedBC->copyOut(buf, bytesToCopy, referencedOffset + offset);
}

bool B_Ref::compare2(const void *buf, const size_t &len, bool caseSensitive, const size_t &offset)
{
    if (!referencedBC)
    {
        return false;
    }

    // CAN'T COPY BEYOND OFFSET.
    if (offset > size())
    {
        return false;
    }

    size_t maxBytesToCompare = size() - offset;
    size_t bytesToCompare = std::min(len, maxBytesToCompare);

    return referencedBC->compare(buf, bytesToCompare, caseSensitive, referencedOffset + offset);
}
