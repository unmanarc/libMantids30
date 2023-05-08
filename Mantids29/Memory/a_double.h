#pragma once
#include "a_var.h"

#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class DOUBLE: public Var
{
public:
    DOUBLE();
    DOUBLE(const double & value);
    DOUBLE& operator=(const double & value)
    {
        setValue(value);
        return *this;
    }

    double getValue();
    void setValue(const double & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    double value;
    Threads::Sync::Mutex_Shared mutex;


};

}}}

