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

    json * processValue();
    json * getValue();


    StreamableJSON& operator=(const Json::Value& value);

    void setValue(const json & value);

    void setMaxSize(const size_t &value);

    bool getFormatted() const;
    void setFormatted(bool value);

private:

    size_t m_maxSize = std::numeric_limits<size_t>::max();
    std::string m_strValue;
    json m_root;
    bool m_formatted = true;
    bool m_isFull = false;

};

}}}


