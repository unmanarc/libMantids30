#ifndef A_UINT32_H
#define A_UINT32_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class UINT32: public Var
{
public:
    UINT32();
    UINT32& operator=(uint32_t value)
    {
        setValue(value);
        return *this;
    }

    uint32_t getValue();
    bool setValue(uint32_t value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    uint32_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_UINT32_H
