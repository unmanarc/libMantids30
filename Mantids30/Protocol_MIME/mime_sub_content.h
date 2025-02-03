#pragma once

//#include <Mantids30/Memory/streamableobject.h>
#include <Mantids30/Memory/subparser.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {


class MIME_Sub_Content : public Memory::Streams::SubParser
{
public:
    MIME_Sub_Content();
    ~MIME_Sub_Content();

    bool stream(Memory::Streams::StreamableObject::Status &wrStat) override;

    uint64_t getMaxContentSize() const;
    void setMaxContentSize(const uint64_t &value);

    std::string getFsTmpFolder() const;
    void setFsTmpFolder(const std::string &value);

    std::shared_ptr<Memory::Streams::StreamableObject> getContentContainer() const;
    void replaceContentContainer(std::shared_ptr<Memory::Streams::StreamableObject> value);

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

    // TODO: implement using filesystem.
    uint64_t getMaxContentSizeUntilGoingToFS() const;
    void setMaxContentSizeUntilGoingToFS(const uint64_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
private:
    std::shared_ptr<Memory::Streams::StreamableObject>  m_contentContainer;

    std::string m_fsTmpFolder, m_boundary;
    uint64_t m_maxContentSize;
    uint64_t m_maxContentSizeUntilGoingToFS;
};

}}}}

