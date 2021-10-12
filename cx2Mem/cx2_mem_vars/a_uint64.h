#ifndef A_UINT64_H
#define A_UINT64_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class UINT64: public Var
{
public:
    UINT64();
    UINT64(const uint64_t &value);
    UINT64& operator=(const uint64_t & value)
    {
        setValue(value);
        return *this;
    }

    uint64_t getValue();

    int64_t getIValueTruncatedOrZero();

    bool setValue(const uint64_t &value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    uint64_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_UINT64_H
