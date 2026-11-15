#pragma once

#include "a_var.h"

#include <cstdint>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class INT64 : public Var
{
public:
    INT64();
    INT64(const int64_t &value);
    INT64 &operator=(const int64_t &value)
    {
        setValue(value);
        return *this;
    }

    int64_t getValue();
    bool setValue(const int64_t &value);

    void *getDirectMemory() override { return &m_value; }
    std::string toString() override;
    bool fromString(const std::string_view value) override;

    boost::json::value toJSON() override;
    bool fromJSON(const boost::json::value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    int64_t m_value = 0;
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
