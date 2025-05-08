#pragma once

#include <limits>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string>

namespace Mantids30 { namespace Memory { namespace Streams {

struct WriteStatus {

    WriteStatus()
    {
        bytesWritten = 0;
        succeed = true;
        finish = false;
    }

    WriteStatus(const uint64_t & x)
    {
        bytesWritten = x;
        succeed = true;
        finish = false;
    }

    WriteStatus & operator=(const uint64_t & x)
    {
        bytesWritten = x;
        return *this;
    }

    WriteStatus & operator+=(const uint64_t & x)
    {
        bytesWritten += x;
        return *this;
    }

    WriteStatus & operator+=(const WriteStatus & x)
    {
        bytesWritten += x.bytesWritten;
        if (!x.succeed) succeed = false;
        if (x.finish) finish = x.finish;
        return *this;
    }

    bool succeed, finish;
    uint64_t bytesWritten;
};

/**
 * StreamableObject base class
 * This is a base class for streamable objects that can be retrieved or parsed trough read/write functions.
 */
class StreamableObject : public std::enable_shared_from_this<StreamableObject>
{
public:
    StreamableObject() = default;
    virtual ~StreamableObject() = default;

    // TODO: what if the protocol reached std::numeric_limits<uint64_t>::max() ? enforce 64bit max. (on streamTo)
    // TODO: report percentage completed

    virtual std::string getPeerName() const { return  ""; }

    virtual std::string toString();
    /**
     * @brief writeEOF proccess the end of the stream (should be implemented on streamTo)
     */
    virtual void writeEOF(bool);
    /**
     * @brief size return the size of the sub-container if it's fixed size.
     * @return std::numeric_limits<uint64_t>::max() if the stream is not fixed size
     */
    virtual uint64_t size() const { return std::numeric_limits<uint64_t>::max(); }
    virtual bool streamTo(Memory::Streams::StreamableObject * out, WriteStatus & wrStatUpd)=0;
    virtual WriteStatus write(const void * buf, const size_t &count, WriteStatus & wrStatUpd)=0;

    WriteStatus writeFullStream(const void *buf, const size_t &count, WriteStatus & wrStatUpd);
    /**
     * @brief writeStream Write into stream using std::strings
     * @param buf data to be streamed.
     * @return true if succeed (all bytes written)
     */
    WriteStatus writeString(const std::string & buf, WriteStatus & wrStatUpd);
    WriteStatus writeString(const std::string & buf);

    WriteStatus strPrintf(const char * format, ...);
    WriteStatus strPrintf(WriteStatus & wrStatUpd, const char * format, ...);

    uint16_t getFailedWriteState() const;

protected:
    bool setFailedWriteState(const uint16_t &value = 1);
    uint16_t m_failedWriteState = 0;

#ifdef _WIN32
private:
    static int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

};

}}}



