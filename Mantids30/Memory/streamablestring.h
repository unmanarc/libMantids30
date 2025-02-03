#pragma once

#include "streamableobject.h"
#include <string>

namespace Mantids30 { namespace Memory { namespace Streams {

/**
 * @brief The StreamableString class (NOTE: not thread-safe for R/W)
 */
class StreamableString : public Memory::Streams::StreamableObject
{
public:
    StreamableString() = default;
    /**
     * @brief streamTo Stream all the content to another StreamableObject
     * @param out output
     * @param wrStatUpd write status counter
     * @return true if streamed ok
     */
    virtual bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>  out, Status & wrStatUpd) override;
    /**
     * @brief write Write/Append into the file
     * @param buf buffer to be written
     * @param count size of the buffed to be written
     * @param wrStatUpd Overall Write Status
     * @return Write Status from the operation
     */
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

    StreamableString& operator=(const std::string& str);

    const std::string &getValue() const;
    void setValue(const std::string &newValue);

private:
    std::string m_value;
};

}}}

