#ifndef MIME_SUB_CONTENT_H
#define MIME_SUB_CONTENT_H

#include <cx2_mem_streamparser/substreamparser.h>

namespace CX2 { namespace Network { namespace HTTP {


class MIME_Sub_Content : public Memory::Streams::Parsing::SubParser
{
public:
    MIME_Sub_Content();
    ~MIME_Sub_Content();

    bool stream(Memory::Streams::Status &wrStat) override;

    uint64_t getMaxContentSize() const;
    void setMaxContentSize(const uint64_t &value);

    std::string getFsTmpFolder() const;
    void setFsTmpFolder(const std::string &value);

    Memory::Containers::B_Base *getContentContainer() const;
    void replaceContentContainer(Memory::Containers::B_Base *value);

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

    // TODO: implement using filesystem.
    uint64_t getMaxContentSizeUntilGoingToFS() const;
    void setMaxContentSizeUntilGoingToFS(const uint64_t &value);

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;
private:
    Memory::Containers::B_Base * contentContainer;

    std::string fsTmpFolder, boundary;
    uint64_t maxContentSize;
    uint64_t maxContentSizeUntilGoingToFS;
};

}}}

#endif // MIME_SUB_CONTENT_H
