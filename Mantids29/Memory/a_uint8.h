#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class UINT8: public Var
{
public:
    UINT8();
    UINT8(const uint8_t & value);
    UINT8& operator=(const uint8_t & value)
    {
        setValue(value);
        return *this;
    }

    uint8_t getValue();
    bool setValue(const uint8_t & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;
private:
    uint8_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

