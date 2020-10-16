#ifndef A_UINT32_H
#define A_UINT32_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_UINT32: public Abstract
{
public:
    A_UINT32();
    A_UINT32& operator=(uint32_t value)
    {
        setValue(value);
        return *this;
    }

    uint32_t getValue();
    bool setValue(uint32_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<uint32_t> value;
};

}}}

#endif // A_UINT32_H
