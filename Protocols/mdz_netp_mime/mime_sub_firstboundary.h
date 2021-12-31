#ifndef MIME_SUB_ENDPOINT_H
#define MIME_SUB_ENDPOINT_H

#include <string>
#include <mdz_mem_vars/substreamparser.h>

namespace Mantids { namespace Network { namespace MIME {

class MIME_Sub_FirstBoundary : public Memory::Streams::Parsing::SubParser
{
public:
    MIME_Sub_FirstBoundary();

    bool stream(Memory::Streams::Status &wrStat) override;

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;
private:
    std::string boundary;
};

}}}

#endif // MIME_SUB_ENDPOINT_H
