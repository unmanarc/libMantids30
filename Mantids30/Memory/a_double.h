#pragma once
#include "a_var.h"
#include <Mantids30/Helpers/json.h>

#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class DOUBLE : public Var
{
public:
    DOUBLE();
    DOUBLE(const double &value);
    DOUBLE &operator=(const double &value)
    {
        setValue(value);
        return *this;
    }

    double getValue();
    void setValue(const double &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    boost::json::value toJSON() override;
    bool fromJSON(const boost::json::value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    double m_value = 0.0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
