#ifndef STREAM_WRITER_H
#define STREAM_WRITER_H

#include <stdint.h>
#include <string>
#include <limits>

namespace Mantids { namespace Network { namespace Streams {

class StreamSocketWriter
{
public:
    StreamSocketWriter();
    virtual ~StreamSocketWriter();

    template<typename T>
    bool writeU(const T & c)
    {
        bool r = false;

        switch(sizeof(T))
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

        if (!r) writeDeSync();

        return r;
    }

    /**
         * Write data block of maximum of 16384Pbytes-1 (teotherical), and depends on the data type selected (uint8_t,uint16_t,uint32_t,uint64_t)
         * @param data data block bytes
         * @param datalen data length to be sent.
         * @return true if succeed to send, otherwise false.
         */
    template<typename T>
    bool writeBlockEx(const void * data,const T & datalen)
    {
        if (sizeof(T) == 1 && !writeU8(datalen))
            return false;
        if (sizeof(T) == 2 && !writeU16(datalen))
            return false;
        if (sizeof(T) == 4 && !writeU32(datalen))
            return false;
        if (sizeof(T) == 8 && !writeU64(datalen))
            return false;
        else // Not supported
        {
            writeDeSync();
            return false;
        }

        if (!datalen)
            return true;

        bool r = (writeFull(data, datalen));

        if (!r)
            writeDeSync();

        return r;
    }

    template<typename T>
    bool writeStringEx(const std::string & str,const uint64_t & maxSize = std::numeric_limits<T>::max()-1)
    {
        if (str.size()>maxSize)
        {
            writeDeSync();
            return false;
        }
        bool r = writeBlockEx<T>(str.c_str(), str.size());
        return r;
    }

protected:

    virtual bool writeFull(const void * data, const uint64_t & datalen) = 0;
    virtual void writeDeSync()=0;
private:
    /**
         * Write unsigned char
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU8(const unsigned char & c);
    /**
         * Write unsigned short (16bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU16(const uint16_t & c);
    /**
         * Write unsigned integer (32bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU32(const uint32_t & c);
    /**
         * Write unsigned integer (64bit)
         * @param c char to write into the socket.
         * @return true if succeed.
         * */
    bool writeU64(const uint64_t & c);
};

}}}

#endif // STREAM_WRITER_H
