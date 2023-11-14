#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {
class BOOL: public Var
{
public:
    BOOL();
    BOOL(const bool & value);
    BOOL& operator=(const bool & value)
    {
        setValue(value);
        return *this;
    }

    bool getValue();
    bool setValue(bool value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;
private:
    bool value;
    Threads::Sync::Mutex_Shared mutex;

};
}}}
