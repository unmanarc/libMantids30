#pragma once

#include "a_var.h"
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class PTR : public Var
{
public:
    PTR();
    PTR(void *value);
    PTR &operator=(void *value)
    {
        setValue(value);
        return *this;
    }

    void *getValue();
    bool setValue(void *value);

    void *getDirectMemory() override { return m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    void *m_value = nullptr;
    std::shared_mutex m_mutex;
};
} // namespace Mantids30::Memory::Abstract
