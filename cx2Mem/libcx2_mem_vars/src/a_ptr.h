#ifndef A_NULL_H
#define A_NULL_H

#include "a_var.h"
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

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
#endif // A_NULL_H
