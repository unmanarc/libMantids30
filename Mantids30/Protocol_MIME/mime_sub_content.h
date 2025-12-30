#pragma once

//#include <Mantids30/Memory/streamableobject.h>
#include <Mantids30/Memory/subparser.h>

namespace Mantids30::Network::Protocols {
namespace MIME {

class MIME_Sub_Content : public Memory::Streams::SubParser
{
public:
    MIME_Sub_Content();
    ~MIME_Sub_Content() = default;

    bool streamToUpstream() override;

    size_t getMaxContentSize() const;
    void setMaxContentSize(const size_t &value);

    std::string getFsTmpFolder() const;
    void setFsTmpFolder(const std::string &value);

    std::shared_ptr<Memory::Streams::StreamableObject> getContentContainer() const;
    void replaceContentContainer(std::shared_ptr<Memory::Streams::StreamableObject> value);

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

    // TODO: implement using filesystem.
    size_t getMaxContentSizeUntilGoingToFS() const;
    void setMaxContentSizeUntilGoingToFS(const size_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    std::shared_ptr<Memory::Streams::StreamableObject> m_contentContainer;

    std::string m_fsTmpFolder, m_boundary;
    size_t m_maxContentSize;
    size_t m_maxContentSizeUntilGoingToFS;
};

} // namespace MIME
} // namespace Mantids30::Network::Protocols
