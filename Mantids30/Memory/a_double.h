#pragma once
#include "Mantids30/Helpers/json.h"
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

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    json toJSON() override;
    bool fromJSON(const json &value) override;


protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    double m_value = 0.0;
    Threads::Sync::Mutex_Shared m_mutex;


};

}}}

