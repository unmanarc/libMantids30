#ifndef A_INT64_H
#define A_INT64_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

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

#endif // A_INT64_H
