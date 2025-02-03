#pragma once

#include "a_var.h"
#include <list>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

class STRINGLIST: public Var
{
public:
    STRINGLIST();
    STRINGLIST(const std::list<std::string> & value);
    STRINGLIST& operator=(const std::list<std::string> & value)
    {
        setValue(value);
        return *this;
    }
    std::list<std::string> getValue();
    bool setValue(const std::list<std::string> &value);

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::list<std::string> m_value;
    Threads::Sync::Mutex_Shared m_mutex;
};

}}}

