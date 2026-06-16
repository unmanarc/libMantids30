#pragma once

#include <Mantids30/Memory/streamable_object.h>

#include <limits>
#include <cstdint>
#include <string>

namespace Mantids30::Network::Sockets {

class Socket_Stream_Writer
{
public:
    Socket_Stream_Writer() = default;
    virtual ~Socket_Stream_Writer() = default;

    template<typename T>
    bool writeU(const T &c)
    {
        bool r = false;

        switch (sizeof(T))
        {
        case 1:
            r = writeU8(c);
            break;
        case 2:
            r = writeU16(c);
            break;
        case 4:
            r = writeU32(c);
            break;
        case 8:
            r = writeU64(c);
            break;
        default:
            r = false;
        }

        if (!r)
        {
            writeDeSync();
        }

        return r;
    }

    /**
         * Write data block of maximum of 16384Pbytes-1 (teotherical), and depends on the data type selected (uint8_t,uint16_t,uint32_t,uint64_t)
         * @param data data block bytes
         * @param datalen data length to be sent.
         * @return true if succeed to send, otherwise false.
         */
    template<typename T>
    bool writeBlockEx(const void *data, const T &datalen)
    {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported length variable size");

        bool wr = false;

        if constexpr (sizeof(T) == 1)
        {
            wr = writeU8(datalen);
        }
        else if constexpr (sizeof(T) == 2)
        {
            wr = writeU16(datalen);
        }
        else if constexpr (sizeof(T) == 4)
        {
            wr = writeU32(datalen);
        }
        else if constexpr (sizeof(T) == 8)
        {
            wr = writeU64(datalen);
        }

        if (!wr)
        {
            return false;
        }

        if (datalen == 0)
        {
            return true;
        }

        const bool ok = writeFull(data, datalen);

        if (!ok)
        {
            writeDeSync();
        }

        return ok;
    }

    template<typename T>
    bool writeStringEx(const std::string &str, const size_t &maxSize = std::numeric_limits<T>::max() - 1)
    {
        if (str.size() > maxSize)
        {
            writeDeSync();
            return false;
        }
        bool r = writeBlockEx<T>(str.c_str(), str.size());
        return r;
    }

protected:
    virtual bool writeFull(const void *data, const size_t &datalen) = 0;
    virtual void writeDeSync() = 0;

private:
    /**
         * Write unsigned char
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU8(const unsigned char &c);
    /**
         * Write unsigned short (16bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU16(const uint16_t &c);
    /**
         * Write unsigned integer (32bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU32(const uint32_t &c);
    /**
         * Write unsigned integer (64bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU64(const uint64_t &c);
};

} // namespace Mantids30::Network::Sockets
