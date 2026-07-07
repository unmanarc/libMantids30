#pragma once

#include "a_var.h"
#include <shared_mutex>


namespace Mantids30::Memory::Abstract {

class STRING : public Var
{
public:
    STRING();
    STRING(const std::string &value);
    STRING &operator=(const std::string &value)
    {
        setValue(value);
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);
    bool setValue(const char *value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::string m_value;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
