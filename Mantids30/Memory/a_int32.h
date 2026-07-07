#pragma once

#include "a_var.h"
#include <shared_mutex>
#include <cstdint>

namespace Mantids30::Memory::Abstract {

class INT32 : public Var
{
public:
    INT32();
    INT32(const int32_t &value);
    INT32 &operator=(const int32_t &value)
    {
        setValue(value);
        return *this;
    }

    int32_t getValue();
    bool setValue(const int32_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    int32_t m_value = 0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
