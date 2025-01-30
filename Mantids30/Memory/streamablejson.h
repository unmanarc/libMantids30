#pragma once

#include <Mantids30/Memory/streamableobject.h>
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableJSON : public Memory::Streams::StreamableObject
{
public:
    StreamableJSON();

    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>  out, Memory::Streams::StreamableObject::Status & wrStatUpd) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Memory::Streams::StreamableObject::Status & wrStatUpd)  override;
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

    uint64_t maxSize;
    std::string strValue;
    json root;
    bool formatted;
};

}}}


