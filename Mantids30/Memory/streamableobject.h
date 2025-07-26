#pragma once

#include "Mantids30/Helpers/safeint.h"
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>
#include <string>

namespace Mantids30 { namespace Memory { namespace Streams {

struct WriteStatus {

    WriteStatus()
    {
        bytesWritten = 0;
    }

    // ssize because we consider the write() output with errors.
    WriteStatus(const ssize_t & x)
    {
        bytesWritten = 0;
        *this+=x;
    }

    WriteStatus & operator=(const size_t & x)
    {
        bytesWritten = x;
        return *this;
    }


    WriteStatus & operator-(const WriteStatus & x)
    {
        if ( x.bytesWritten > bytesWritten )
        {
            // you should not be writting anymore here.
            throw std::runtime_error("Invalid WriteStatus Substract Operation.");
        }

        bytesWritten-=x.bytesWritten;

        succeed = x.succeed;
        finish = x.finish;
        writeError = x.writeError;

        return *this;
    }

    WriteStatus & operator+=(const ssize_t & x)
    {
        if (succeed == false || finish == true)
        {
            // you should not be writting anymore here.
            throw std::runtime_error("Trying to write into an already finished write object.");
        }

        if (x>0)
        {
            bytesWritten = safeAdd(bytesWritten,(size_t)x);
        }
        else if (x == 0)
        {
            succeed = true;
            finish = true;
        }
        else
        {
            succeed = false;
            finish = false;
            writeError = x;
        }
        return *this;
    }

    WriteStatus & operator+=(const WriteStatus & x)
    {
        bytesWritten = safeAdd(bytesWritten, x.bytesWritten);

        if (!x.succeed)
            succeed = false;

        if (x.finish)
            finish = true;

        return *this;
    }

    void reset()
    {
        succeed = true;
        finish = false;
        bytesWritten = 0;
        writeError = 0;
    }

    bool succeed = true;
    bool finish = false;
    size_t bytesWritten = 0;
    ssize_t writeError = 0;
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

    // TODO: what if the protocol reached std::numeric_limits<size_t>::max() ? enforce 64bit max. (on streamTo)
    // TODO: report percentage completed

    virtual std::string getPeerName() const
    {
        return  "";
    }
    virtual std::string toString();
    /**
     * @brief writeEOF proccess the end of the stream (should be implemented on streamTo)
     */
    size_t writeEOF();

    /**
     * @brief size return the size of the sub-container if it's fixed size.
     * @return std::numeric_limits<uint64_t>::max() if the stream is not fixed size
     */
    virtual size_t size()
    {
        return std::numeric_limits<size_t>::max();
    }
    /**
     * @brief streamTo Stream this container to another streamable object
     * @param out
     * @return optional size of bytes taken out of this container, std::nullopt if the source failed to provide the data.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out)
    {
        writeEOF();
        return true;
    }

    std::optional<size_t> writeFullStreamWithEOF(const void *buf, const size_t &count);
    size_t writeFullStream(const void *buf, const size_t &count);

    // Partial Write...
    virtual size_t write(const void * buf, const size_t &count)=0;
    /**
     * @brief writeStream Write into stream using std::strings
     * @param buf data to be streamed.
     * @return true if succeed (all bytes written)
     */
    size_t writeString(const std::string & buf);

    size_t strPrintf(const char * format, ...);

    WriteStatus writeStatus;


protected:

#ifdef _WIN32
private:
    static int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

};

}}}



