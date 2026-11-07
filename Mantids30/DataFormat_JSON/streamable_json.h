#pragma once

#include <Mantids30/DataFormat_JSON/json.h>
#include <Mantids30/Memory/streamable_object.h>

namespace Mantids30::Memory::Streams {

class StreamableJSON : public StreamableObject
{
public:
    StreamableJSON() = default;

    bool streamTo(StreamableObject *out) override;
    std::optional<size_t> write(const void *buf, const size_t &count) override;
    size_t size() override;

    void clear();

    /**
     * @brief processValue Proccess current string into m_root and return the internal boost::json::value pointer if parsing succeed, otherwise return nullptr.
     * @return
     */
    boost::json::value *processValue();
    /**
     * @brief getValue Return the pointer to the internal boost::json::value.
     * @return
     */
    boost::json::value *getValue();

    bool isEmpty();

    StreamableJSON &operator=(const boost::json::value &value);

    void setValue(const boost::json::value &value);
    bool setValue(const std::string &value);

    void setMaxSize(const size_t &value);

    bool getIsFormatted() const;
    void setIsFormatted(bool value);

private:
    size_t m_maxSize = std::numeric_limits<size_t>::max();
    std::string m_strValue;
    boost::json::value m_root{boost::json::object{}};
    bool m_isFormatted = true;
    bool m_isFull = false;
};

} // namespace Mantids30::Memory::Streams
