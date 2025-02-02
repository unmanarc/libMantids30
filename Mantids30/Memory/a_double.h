#pragma once
#include "a_var.h"

#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

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
    std::shared_ptr<Var> protectedCopy() override;

private:
    double value;
    Threads::Sync::Mutex_Shared mutex;


};

}}}

