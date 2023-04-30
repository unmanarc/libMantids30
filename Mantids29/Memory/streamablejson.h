#ifndef JSON_STREAMABLEOBJECT_H
#define JSON_STREAMABLEOBJECT_H

#include <Mantids29/Memory/streamableobject.h>
#include <Mantids29/Helpers/json.h>

namespace Mantids29 { namespace Memory { namespace Streams {

class StreamableJSON : public Memory::Streams::StreamableObject
{
public:
    StreamableJSON();

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::StreamableObject::Status & wrStatUpd) override;
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


#endif // JSON_STREAMABLEOBJECT_H
