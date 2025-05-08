#pragma once

#include <Mantids30/Memory/subparser.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class URLVar_SubParser : public Memory::Streams::SubParser
{
public:
    URLVar_SubParser();
    ~URLVar_SubParser() override;

    bool streamToUpstream(Memory::Streams::WriteStatus &) override;

    void setVarType(bool varName = true);
    void setMaxObjectSize(const uint32_t &size);
    std::shared_ptr<Memory::Containers::B_Chunks> flushRetrievedContentAsBC();
    std::string flushRetrievedContentAsString();
    std::shared_ptr<Memory::Containers::B_Chunks> getPData();

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
    std::shared_ptr<Memory::Containers::B_Chunks> m_pData;

};

}}}}}

