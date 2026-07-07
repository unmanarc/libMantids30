#pragma once

#include "a_var.h"

#include <cstdint>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class INT16 : public Var
{
public:
    INT16();
    INT16(const int16_t &value);
    INT16 &operator=(const int16_t &value)
    {
        setValue(value);
        return *this;
    }

    int16_t getValue();
    bool setValue(const int16_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    int16_t m_value = 0;
    std::shared_mutex m_mutex;
};
} // namespace Mantids30::Memory::Abstract
