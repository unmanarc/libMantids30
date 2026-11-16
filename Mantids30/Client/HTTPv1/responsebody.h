#pragma once

#include <Mantids30/Core/Memory/streamable_object.h>
#include <functional>

namespace Mantids30::Client::Detail {

/** @brief Internal write adapter forwarding received bytes to the caller destination without buffering. */
class HTTPv1ResponseBody : public Mantids30::Core::Memory::Streams::StreamableObject
{
public:
    using DataCallback = std::function<bool(const void *, size_t)>;
    using ProgressCallback = std::function<bool(size_t, std::optional<size_t>)>;

    HTTPv1ResponseBody(const std::shared_ptr<StreamableObject> &destination, size_t maxSize,
                 DataCallback onData = {}, ProgressCallback onProgress = {});

    std::optional<size_t> writeImpl(const void *buf, const size_t &count) override;

    void setExpectedSize(std::optional<size_t> expectedSize);
    bool isCancelled() const { return m_cancelled; }
    bool isLimitExceeded() const { return m_limitExceeded; }

private:
    std::shared_ptr<StreamableObject> m_destination;
    size_t m_maxSize;
    std::optional<size_t> m_expectedSize;
    DataCallback m_onData;
    ProgressCallback m_onProgress;
    bool m_cancelled = false;
    bool m_limitExceeded = false;
};

} // namespace Mantids30::Client::Detail
