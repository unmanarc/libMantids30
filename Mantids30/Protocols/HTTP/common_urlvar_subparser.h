#pragma once

#include <Mantids30/Core/Memory/subparser.h>
#include <memory>

namespace Mantids30::Protocols::HTTP {

class URLVarContent : public Mantids30::Core::Memory::Streams::SubParser
{
public:
    URLVarContent();
    ~URLVarContent() override;

    void setVarType(bool varName = true);
    void setMaxObjectSize(const size_t &size);
    std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> getContentAndFlush();
    std::string getContentAsStringAndFlush();
    std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> getCurrentContentData();

protected:
    Mantids30::Core::Memory::Streams::SubParser::ParseResult parse() override;
    std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> m_pData;
};

} // namespace Mantids30::Protocols::HTTP
