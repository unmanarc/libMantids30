#pragma once

#include <Mantids30/Core/Helpers/safeint.h>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace Mantids30::Core::Memory::Streams {

struct WriteStatus
{
    WriteStatus() { bytesWritten = 0; }

    // ssize because we consider the write() output with errors.
    WriteStatus(const ssize_t &x)
    {
        bytesWritten = 0;
        *this += x;
    }

    WriteStatus &operator=(const size_t &x)
    {
        bytesWritten = x;
        return *this;
    }

    WriteStatus &operator-(const WriteStatus &x)
    {
        if (x.bytesWritten > bytesWritten)
        {
            // you should not be writting anymore here.
            throw std::runtime_error("Invalid WriteStatus Substract Operation.");
        }

        bytesWritten -= x.bytesWritten;

        succeed = x.succeed;
        finished = x.finished;
        writeError = x.writeError;

        return *this;
    }

    WriteStatus &operator+=(const ssize_t &x)
    {
        if ((!succeed || finished) && x >= 0)
        {
            // you should not be writting anymore here.
            throw std::runtime_error("Trying to write into an already finished write object.");
        }

        if (x > 0)
        {
            bytesWritten = safe_add(bytesWritten, static_cast<size_t>(x));
        }
        else if (x == 0)
        {
            succeed = true;
            finished = true;
        }
        else
        {
            succeed = false;
            finished = true;
            writeError = x;
        }
        return *this;
    }

    WriteStatus &operator+=(const WriteStatus &x)
    {
        bytesWritten = safe_add(bytesWritten, x.bytesWritten);

        if (!x.succeed)
        {
            succeed = false;
        }

        if (x.finished)
        {
            finished = true;
        }

        return *this;
    }

    void reset()
    {
        succeed = true;
        finished = false;
        bytesWritten = 0;
        writeError = 0;
    }

    bool succeed = true;
    bool finished = false;
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

    struct WriteProgress
    {
        size_t bytesWritten;
        std::optional<size_t> totalSize;
        // Percentage in [0, 100], absent when the total is unknown.
        std::optional<double> percentage;
    };
    using WriteProgressCallback = std::function<void(const WriteProgress &)>;

    /**
     * Report accepted bytes synchronously after successful writes. With a known total,
     * notify when either enabled interval is reached (percentage points or bytes),
     * and on completion/EOF. Zero disables an interval; both zero reports every write.
     * Without a total, report every nonempty write. Large writes produce one notification.
     * An empty callback disables notifications. Exceptions propagate to the caller.
     * Configure and write from the same thread; callbacks must not write to this object.
     */
    void setWriteProgressCallback(WriteProgressCallback callback, double percentageInterval = 0.1, size_t byteInterval = 0);
    /** Set the expected incoming size and start a new progress count, independent of size()/writeStatus. */
    void setWriteTargetSize(std::optional<size_t> totalSize);
    std::optional<size_t> getWriteTargetSize() const { return m_writeTargetSize; }

    virtual std::string getPeerName() const { return ""; }
    virtual std::string toString();
    /**
     * @brief writeEOF proccess the end of the stream (should be implemented on streamTo)
     */
    bool writeEOF();

    /**
     * @brief size return the size of the sub-container if it's fixed size.
     * @return std::numeric_limits<uint64_t>::max() if the stream is not fixed size
     */
    virtual size_t size() { return std::numeric_limits<size_t>::max(); }

    /**
     * @brief streamTo Streams this object to another streamable object, will not send the EOF.
     * @param out
     * @return true if streamed, false otherwise
     */
    virtual bool streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out) { return true; }

    bool writeFullStreamWithEOF(const void *buf, const size_t &count);
    bool writeFullStream(const void *buf, const size_t &count);

    // Partial Write...
    std::optional<size_t> write(const void *buf, const size_t &count);
    /**
     * @brief writeStream Write into stream using std::strings
     * @param buf data to be streamed.
     * @return true if succeed (all bytes written)
     */
    bool writeString(std::string_view buf);

    size_t strPrintf(const char *format, ...);

    WriteStatus writeStatus;

protected:
    // Implement partial writes here; keep updating writeStatus as usual.
    virtual std::optional<size_t> writeImpl(const void *buf, const size_t &count) = 0;

private:
    WriteProgressCallback m_writeProgressCallback;
    std::optional<size_t> m_writeTargetSize;
    size_t m_progressBytes = 0;
    size_t m_lastProgressBytes = 0;
    double m_percentageInterval = 0.1;
    size_t m_byteInterval = 0;
    double m_lastProgressPercentage = 0;
    bool m_completionReported = false;

#ifdef _WIN32
private:
    static int vasprintf(char **strp, const char *fmt, va_list ap);
#endif
};

} // namespace Mantids30::Core::Memory::Streams
