#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class INT16: public Var
{
public:
    INT16();
    INT16(const int16_t & value);
    INT16& operator=(const int16_t & value)
    {
        setValue(value);
        return *this;
    }

    int16_t getValue();
    bool setValue(const int16_t & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    int16_t value;
    Threads::Sync::Mutex_Shared mutex;

};
}}}

