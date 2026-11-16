#include "pool.h"

#ifndef _WIN32
#include <algorithm>
#include <cerrno>

using namespace Mantids30::Runtime;
using Clock = std::chrono::steady_clock;
namespace JSON = Mantids30::DataFormat::JSON;

ExecPool::Error ExecPool::exchange(App &app, const std::shared_ptr<State> &state, boost::json::value &output)
{
    // Add correlation metadata to a copy so the original request remains intact.
    auto input = state->input;
    const uint64_t id = m_nextId.fetch_add(1);
    if (!m_config.idField.empty())
    {
        if (!input.is_object())
        {
            return Error::INVALID_INPUT;
        }
        JSON::setMember(input, m_config.idField, id);
    }
    // Enforce the byte limit on serialized JSON before adding the wire delimiter.
    // Input validation failures leave the process protocol untouched.
    auto line = JSON::toString(input);
    if (line.size() > m_config.maxLineBytes)
    {
        return Error::INPUT_TOO_LONG;
    }
    line.push_back('\n');
    size_t sent = 0;
    std::string response;
    while (m_running)
    {
        // Recompute the remaining budget after every partial transfer or signal.
        // Cap poll waits so shutdown is observed even when both streams are idle.
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(state->deadline - Clock::now()).count();
        if (remaining <= 0)
        {
            return Error::TIMEOUT;
        }
        // Read and write together to avoid pipe backpressure; short polls observe shutdown.
        pollfd descriptors[2] = {
            {sent < line.size() ? app.getStreamFD(STDIN_FILENO) : -1, POLLOUT, 0},
            {app.getStreamFD(STDOUT_FILENO), POLLIN, 0}};
        const int result = ::poll(descriptors, 2, static_cast<int>(std::min<int64_t>(remaining, 25)));
        if (result < 0)
        {
            // Signals do not consume a separate retry budget; the absolute deadline
            // still limits the overall exchange.
            if (errno == EINTR)
            {
                continue;
            }
            return Error::IO_ERROR;
        }
        if (sent < line.size() && (descriptors[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            return Error::PROCESS_EXITED;
        }
        if ((descriptors[0].revents & POLLOUT) != 0 && sent < line.size())
        {
            // Nonblocking writes can make partial progress. Retain the offset and
            // wait for POLLOUT again rather than retrying in a busy loop.
            const auto count = app.write(line.data() + sent, line.size() - sent);
            if (count > 0)
            {
                sent += static_cast<size_t>(count);
            }
            else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                return errno == EPIPE ? Error::PROCESS_EXITED : Error::IO_ERROR;
            }
        }
        if ((descriptors[1].revents & POLLNVAL) != 0)
        {
            return Error::IO_ERROR;
        }
        if ((descriptors[1].revents & (POLLIN | POLLHUP | POLLERR)) != 0)
        {
            char buffer[4096];
            // Drain readable data even with HUP: the child may have left final bytes
            // buffered in its pipe before closing the descriptor.
            const auto count = app.read(STDOUT_FILENO, buffer, sizeof(buffer));
            if (count == 0)
            {
                return Error::PROCESS_EXITED;
            }
            if (count < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                return Error::IO_ERROR;
            }
            // Bound response memory and accept exactly one complete response line.
            response.append(buffer, static_cast<size_t>(count));
            const auto newline = response.find('\n');
            if ((newline == std::string::npos && response.size() > m_config.maxLineBytes) ||
                (newline != std::string::npos && newline > m_config.maxLineBytes))
            {
                return Error::LINE_TOO_LONG;
            }
            if (newline != std::string::npos)
            {
                // Early responses or trailing bytes break the one-request/one-response contract.
                if (sent != line.size() || newline + 1 != response.size())
                {
                    return Error::DESYNCHRONIZED;
                }
                if (!JSON::parse(std::string_view(response.data(), newline + 1), output))
                {
                    return Error::INVALID_JSON;
                }
                // A mismatched or missing ID can indicate a stale response from
                // another exchange, so it invalidates this process generation.
                if (!m_config.idField.empty())
                {
                    if (JSON::ASUINT64(output, m_config.idField, id + 1) != id)
                    {
                        return Error::DESYNCHRONIZED;
                    }
                }
                // Parsing and validation also count toward the request deadline.
                return Clock::now() < state->deadline ? Error::SUCCESS : Error::TIMEOUT;
            }
        }
    }
    return Error::STOPPED;
}
#endif
