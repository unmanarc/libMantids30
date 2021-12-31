#ifndef A_UINT32_H
#define A_UINT32_H

#include "a_var.h"
#include <stdint.h>
#include <mdz_thr_mutex/mutex_shared.h>

namespace Mantids { namespace Memory { namespace Abstract {

class UINT32: public Var
{
public:
    UINT32();
    UINT32(const uint32_t &value);
    UINT32& operator=(const uint32_t & value)
    {
        setValue(value);
        return *this;
    }

    uint32_t getValue();
    bool setValue(const uint32_t & value);

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
