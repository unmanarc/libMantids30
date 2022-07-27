#ifndef MIME_SUB_CONTENT_H
#define MIME_SUB_CONTENT_H

#include "mdz_mem_vars/streamableobject.h"
#include <mdz_mem_vars/subparser.h>

namespace Mantids { namespace Protocols { namespace MIME {


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

    Memory::Streams::StreamableObject *getContentContainer() const;
    void replaceContentContainer(Memory::Streams::StreamableObject *value);

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

    // TODO: implement using filesystem.
    uint64_t getMaxContentSizeUntilGoingToFS() const;
    void setMaxContentSizeUntilGoingToFS(const uint64_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
private:
    Memory::Streams::StreamableObject * contentContainer;

    std::string fsTmpFolder, boundary;
    uint64_t maxContentSize;
    uint64_t maxContentSizeUntilGoingToFS;
};

}}}

#endif // MIME_SUB_CONTENT_H
