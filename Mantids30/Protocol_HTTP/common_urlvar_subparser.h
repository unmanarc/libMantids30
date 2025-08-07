#pragma once

#include <Mantids30/Memory/subparser.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP {

class URLVarContent : public Memory::Streams::SubParser
{
public:
    URLVarContent();
    ~URLVarContent() override;


    void setVarType(bool varName = true);
    void setMaxObjectSize(const uint32_t &size);
    std::shared_ptr<Memory::Containers::B_Chunks> getContentAndFlush();
    std::string getContentAsStringAndFlush();
    std::shared_ptr<Memory::Containers::B_Chunks> getCurrentContentData();

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
    std::shared_ptr<Memory::Containers::B_Chunks> m_pData;

};

}}}}

