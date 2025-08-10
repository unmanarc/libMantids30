#pragma once

#include "a_var.h"
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

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

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;


    json toJSON() override;
    bool fromJSON(const json & value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::string m_value;
    Threads::Sync::Mutex_Shared m_mutex;

};

}}}

