#ifndef JSON_STREAMABLEOBJECT_H
#define JSON_STREAMABLEOBJECT_H

#include <mdz_mem_vars/streamable.h>
#include <mdz_hlp_functions/json.h>

namespace Mantids { namespace Memory { namespace Streams {

class JSON_Streamable : public Memory::Streams::Streamable
{
public:
    JSON_Streamable();

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrStatUpd) override;
    Memory::Streams::Status write(const void * buf, const size_t &count, Memory::Streams::Status & wrStatUpd)  override;
    void writeEOF(bool) override;

    void clear();

    std::string getString();

    json * processValue();
    json * getValue();

    void setValue(const json & value);

    void setMaxSize(const uint64_t &value);

    bool getFormatted() const;
    void setFormatted(bool value);

private:

    uint64_t maxSize;
    std::string strValue;
    json root;
    bool formatted;
};

}}}


#endif // JSON_STREAMABLEOBJECT_H
