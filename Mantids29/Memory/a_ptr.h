#pragma once

#include "a_var.h"
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class PTR: public Var
{
public:
    PTR();
    PTR(void *value);
    PTR& operator=(void *value)
    {
        setValue(value);
        return *this;
    }

    void * getValue();
    bool setValue(void * value);

    void * getDirectMemory() override { return value; }


    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    void * value;
    Threads::Sync::Mutex_Shared mutex;

};
}}}
