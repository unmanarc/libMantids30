#include "mem.h"
#include <boost/algorithm/string/predicate.hpp>
#include <cstdlib>
#include <locale>
#include <string_view>
#include <cstring>
#include <stdexcept>

using namespace std;
using namespace Mantids30::Core::Helpers;

Mem::BinaryDataContainer::BinaryDataContainer(const char *_data, const size_t &len)
{
    if (len)
    {
        this->data = malloc(len);
    }
    else
    {
        this->data = nullptr;
    }

    if (this->data)
    {
        this->length = len;
        memcpy(this->data, _data, len);
    }
    else
    {
        this->length = 0;
    }

    cur = 0;
}

Mem::BinaryDataContainer::BinaryDataContainer(const size_t &len)
{
    if (len)
    {
        data = malloc(len);
    }
    else
    {
        data = nullptr;
    }

    if (data)
    {
        this->length = len;
        memset(data, 0, len);
    }
    else
    {
        this->length = 0;
    }

    cur = 0;
}

Mem::BinaryDataContainer::~BinaryDataContainer()
{
    if (data)
    {
        free(data);
    }
    data = nullptr;
    length = 0;
    cur = 0;
}

void Mem::BinaryDataContainer::operator+=(const unsigned char &c)
{
    if (cur >= length)
    {
        throw runtime_error("Attempt to append to a full BinaryDataContainer");
    }
    unsigned char *data_c = reinterpret_cast<unsigned char *>(data);
    data_c[cur++] = c;
}

bool Mem::icharcmp(unsigned char c1, unsigned char c2)
{
    return std::tolower(static_cast<char>(c1), std::locale::classic()) ==
           std::tolower(static_cast<char>(c2), std::locale::classic());
}

int Mem::memcmp64(const void *s1, const void *s2, size_t numBytes)
{
    return numBytes ? std::memcmp(s1, s2, numBytes) : 0;
}

int Mem::memicmp2(const void *s1, const void *s2, const size_t &numBytes, const bool &caseSensitive)
{
    if (!numBytes)
    {
        return 0;
    }
    if (caseSensitive)
    {
        return memcmp64(s1, s2, numBytes);
    }
    // Preserve ASCII folding, embedded NULs, and bytes outside ASCII regardless of the global locale.
    return boost::iequals(std::string_view(static_cast<const char *>(s1), numBytes),
                          std::string_view(static_cast<const char *>(s2), numBytes),
                          std::locale::classic()) ? 0 : -1;
}

void *Mem::memcpy64(void *dest, const void *src, size_t numBytes)
{
    return numBytes ? std::memcpy(dest, src, numBytes) : dest;
}

void *Mem::memmove64(void *dest, const void *src, size_t numBytes)
{
    return numBytes ? std::memmove(dest, src, numBytes) : dest;
}
