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

    virtual bool streamTo(Memory::Streams::StreamableObject * out) override;

    virtual size_t write(const void *buf, const size_t &count) override;

    StreamableString& operator=(const std::string& str);

    const std::string &getValue() const;
    void setValue(const std::string &newValue);

private:
    std::string m_value;
};

}}}

