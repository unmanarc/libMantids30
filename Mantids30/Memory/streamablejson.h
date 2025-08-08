#pragma once

#include <Mantids30/Memory/streamableobject.h>
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableJSON : public Memory::Streams::StreamableObject
{
public:
    StreamableJSON() = default;

    bool streamTo(Memory::Streams::StreamableObject *out) override;
    std::optional<size_t> write(const void *buf, const size_t &count) override;

    void clear();

    /**
     * @brief processValue Proccess current string into m_root and return the internal json pointer if parsing succeed, otherwise return nullptr.
     * @return
     */
    json * processValue();
    /**
     * @brief getValue Return the pointer to the internal json.
     * @return
     */
    json * getValue();

    bool isEmpty();

    StreamableJSON& operator=(const Json::Value& value);

    void setValue(const json & value);
    bool setValue(const std::string & value);

    void setMaxSize(const size_t &value);

    bool getIsFormatted() const;
    void setIsFormatted(bool value);

private:

    size_t m_maxSize = std::numeric_limits<size_t>::max();
    std::string m_strValue;
    json m_root;
    bool m_isFormatted = true;
    bool m_isFull = false;

};

}}}


