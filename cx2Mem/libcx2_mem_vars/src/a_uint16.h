#ifndef A_UINT16_H
#define A_UINT16_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_UINT16: public Abstract
{
public:
    A_UINT16();
    A_UINT16& operator=(uint16_t value)
    {
        setValue(value);
        return *this;
    }

    uint16_t getValue();
    bool setValue(uint16_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<uint16_t> value;
};

}}}

#endif // A_UINT16_H
