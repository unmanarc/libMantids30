#pragma once

#include "a_var.h"

#include <cstdint>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class UINT32 : public Var
{
public:
    UINT32();
    UINT32(const uint32_t &value);
    UINT32 &operator=(const uint32_t &value)
    {
        setValue(value);
        return *this;
    }

    uint32_t getValue();
    bool setValue(const uint32_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    uint32_t m_value = 0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
