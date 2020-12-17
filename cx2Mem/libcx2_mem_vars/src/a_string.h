#ifndef A_STRING_H
#define A_STRING_H

#include "a_var.h"
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class STRING: public Var
{
public:
    STRING();
    ~STRING() override;
    STRING& operator=(const std::string & value)
    {
        setValue(value);
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    std::string value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_STRING_H
