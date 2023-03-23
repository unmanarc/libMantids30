#ifndef HTTP_SUB_URLVARPARSER_H
#define HTTP_SUB_URLVARPARSER_H

#include <Mantids29/Memory/subparser.h>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class URLVar_SubParser : public Memory::Streams::SubParser
{
public:
    URLVar_SubParser();
    ~URLVar_SubParser() override;

    bool stream(Memory::Streams::StreamableObject::Status &) override;

    void setVarType(bool varName = true);
    void setMaxObjectSize(const uint32_t &size);
    Memory::Containers::B_Chunks *flushRetrievedContentAsBC();
    std::string flushRetrievedContentAsString();
    Memory::Containers::B_Chunks *getPData();

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
    Memory::Containers::B_Chunks * pData;

};

}}}}}

#endif // HTTP_SUB_URLVARPARSER_H
