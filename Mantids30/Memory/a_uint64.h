#pragma once

#include "a_var.h"
#include <stdint.h>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30::Memory::Abstract {

class UINT64: public Var
{
public:
    UINT64();
    UINT64(const uint64_t &value);
    UINT64& operator=(const uint64_t & value)
    {
        setValue(value);
        return *this;
    }

    uint64_t getValue();

    int64_t getIValueTruncatedOrZero();

    bool setValue(const uint64_t &value);

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    json toJSON() override;
    bool fromJSON(const json & value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    uint64_t m_value = 0;
    Threads::Sync::Mutex_Shared m_mutex;

};

}

