#pragma once

#include "a_var.h"
#include <shared_mutex>


namespace Mantids30::Memory::Abstract {

class STRING : public Var
{
public:
    STRING();
    STRING(const std::string &value);
    STRING(std::string_view value);
    STRING(const char *value);
    STRING &operator=(const std::string &value)
    {
        setValue(value);
        return *this;
    }
    STRING &operator=(std::string_view value)
    {
        setValue(std::string(value));
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);
    bool setValue(const char *value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string_view value) override;

    boost::json::value toJSON() override;
    bool fromJSON(const boost::json::value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::string m_value;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
