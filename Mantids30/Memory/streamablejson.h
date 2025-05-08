#pragma once

#include <Mantids30/Memory/streamableobject.h>
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableJSON : public Memory::Streams::StreamableObject
{
public:
    StreamableJSON();

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus & wrStatUpd) override;
    Memory::Streams::WriteStatus write(const void * buf, const size_t &count, Memory::Streams::WriteStatus & wrStatUpd)  override;
    void writeEOF(bool) override;

    void clear();

    std::string getString();

    json * processValue();
    json * getValue();


    StreamableJSON& operator=(const Json::Value& value);

    void setValue(const json & value);

    void setMaxSize(const uint64_t &value);

    bool getFormatted() const;
    void setFormatted(bool value);

private:

    uint64_t m_maxSize;
    std::string m_strValue;
    json m_root;
    bool m_formatted;
};

}}}


