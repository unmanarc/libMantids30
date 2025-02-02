#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

class UINT16: public Var
{
public:
    UINT16();
    UINT16(const uint16_t &value);
    UINT16& operator=(const uint16_t & value)
    {
        setValue(value);
        return *this;
    }

    uint16_t getValue();
    bool setValue(const uint16_t & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    uint16_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

