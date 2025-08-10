#pragma once

#include "a_var.h"
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {
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

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    json toJSON() override;
    bool fromJSON(const json &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;
private:
    bool m_value = false;
    Threads::Sync::Mutex_Shared m_mutex;

};
}}}
