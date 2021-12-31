#ifndef A_NULL_H
#define A_NULL_H

#include "a_var.h"
#include <mdz_thr_mutex/mutex_shared.h>

namespace Mantids { namespace Memory { namespace Abstract {

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
