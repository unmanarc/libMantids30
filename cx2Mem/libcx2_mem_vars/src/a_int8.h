#ifndef A_INT8_H
#define A_INT8_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class INT8: public Var
{
public:
    INT8();
    INT8& operator=(int8_t value)
    {
        setValue(value);
        return *this;
    }

    int8_t getValue();
    bool setValue(int8_t value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    int8_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_INT8_H
