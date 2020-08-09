#ifndef ABSVAR_UINT8_H
#define ABSVAR_UINT8_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_UINT8: public Abstract
{
public:
    A_UINT8();
    A_UINT8& operator=(uint8_t value)
    {
        setValue(value);
        return *this;
    }

    uint8_t getValue();
    bool setValue(uint8_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;
private:
    std::atomic<uint8_t> value;
};

}}}

#endif // ABSVAR_UINT8_H
