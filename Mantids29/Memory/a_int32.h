#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class INT32: public Var
{
public:
    INT32();
    INT32(const int32_t &value);
    INT32& operator=(const int32_t & value)
    {
        setValue(value);
        return *this;
    }

    int32_t getValue();
    bool setValue(const int32_t & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    int32_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

