#ifndef JSON_STREAMABLEOBJECT_H
#define JSON_STREAMABLEOBJECT_H

#include <cx2_mem_streams/streamable.h>
#include <json/json.h>

namespace CX2 { namespace Memory { namespace Streams {

class JSON_Streamable : public Memory::Streams::Streamable
{
public:
    JSON_Streamable();

    static std::string jsonToString( const Json::Value & value );

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrStatUpd) override;
    Memory::Streams::Status write(const void * buf, const size_t &count, Memory::Streams::Status & wrStatUpd)  override;
    void writeEOF(bool) override;

    void clear();

    std::string getString();

    Json::Value * processValue();
    Json::Value * getValue();

    void setValue(const Json::Value & value);

    void setMaxSize(const uint64_t &value);

private:
    uint64_t maxSize;
    std::string strValue;
    Json::Value root;
};

}}}


#endif // JSON_STREAMABLEOBJECT_H
