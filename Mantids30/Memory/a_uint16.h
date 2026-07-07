#pragma once

#include "a_var.h"

#include <cstdint>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class UINT16 : public Var
{
public:
    UINT16();
    UINT16(const uint16_t &value);
    UINT16 &operator=(const uint16_t &value)
    {
        setValue(value);
        return *this;
    }

    uint16_t getValue();
    bool setValue(const uint16_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    uint16_t m_value = 0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
