#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class INT64: public Var
{
public:
    INT64();
    INT64(const int64_t &value);
    INT64& operator=(const int64_t &value)
    {
        setValue(value);
        return *this;
    }

    int64_t getValue();
    bool setValue(const int64_t &value);

    void * getDirectMemory() override { return &value; }
    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    int64_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

