#pragma once

#include "a_var.h"

#include <cstdint>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class UINT64 : public Var
{
public:
    UINT64();
    UINT64(const uint64_t &value);
    UINT64 &operator=(const uint64_t &value)
    {
        setValue(value);
        return *this;
    }

    uint64_t getValue();

    int64_t getIValueTruncatedOrZero();

    bool setValue(const uint64_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string_view value) override;

    boost::json::value toJSON() override;
    bool fromJSON(const boost::json::value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    uint64_t m_value = 0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
