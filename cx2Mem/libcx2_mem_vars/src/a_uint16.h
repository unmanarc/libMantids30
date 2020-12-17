#ifndef A_UINT16_H
#define A_UINT16_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class UINT16: public Var
{
public:
    UINT16();
    UINT16& operator=(uint16_t value)
    {
        setValue(value);
        return *this;
    }

    uint16_t getValue();
    bool setValue(uint16_t value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    uint16_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_UINT16_H
