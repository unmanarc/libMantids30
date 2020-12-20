#ifndef A_BOOL_H
#define A_BOOL_H

#include "a_var.h"
#include <stdint.h>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {
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
#endif // A_BOOL_H
