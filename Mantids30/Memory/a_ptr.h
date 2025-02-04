#pragma once

#include "a_var.h"
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

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

    void * getDirectMemory() override { return m_value; }


    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    void * m_value = nullptr;
    Threads::Sync::Mutex_Shared m_mutex;

};
}}}
