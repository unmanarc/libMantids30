#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

class INT8: public Var
{
public:
    INT8();
    INT8(const int8_t &value);
    INT8& operator=(const int8_t & value)
    {
        setValue(value);
        return *this;
    }

    int8_t getValue();
    bool setValue(const int8_t & value);

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    int8_t m_value;
    Threads::Sync::Mutex_Shared m_mutex;

};

}}}

