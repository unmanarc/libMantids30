#pragma once

#include "a_var.h"
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class STRING: public Var
{
public:
    STRING();
    STRING(const std::string & value);
    STRING& operator=(const std::string & value)
    {
        setValue(value);
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);
    bool setValue(const char * value);

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

