#pragma once

#include "a_var.h"

#include <ctime>
#include <shared_mutex>

namespace Mantids30::Memory::Abstract {

class DATETIME : public Var
{
public:
    DATETIME();
    DATETIME(const time_t &value);
    DATETIME &operator=(const time_t &value)
    {
        setValue(value);
        return *this;
    }

    bool isInFuture();

    time_t getValue();
    bool setValue(const time_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toStringLcl();
    std::string toString() override;
    bool fromString(const std::string &value) override;

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::string getPlainLclTimeStr(time_t v);
    std::string getISOTimeStr(const time_t &v);
    time_t fromISOTimeStr(const std::string &v);

    time_t m_value = 0;
    std::shared_mutex m_mutex;
};
} // namespace Mantids30::Memory::Abstract
