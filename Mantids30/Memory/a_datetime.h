#pragma once

#include "a_var.h"
#include "json/config.h"
#include <Mantids30/Threads/mutex_shared.h>
#include <time.h>

namespace Mantids30 {
namespace Memory {
namespace Abstract {

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

    time_t getValue();
    bool setValue(const time_t &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toStringLcl();
    std::string toString() override;
    bool fromString(const std::string &value) override;

    json toJSON() override;
    bool fromJSON(const json &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    std::string getPlainLclTimeStr(time_t v);
    std::string getISOTimeStr(const time_t &v);
    time_t fromISOTimeStr(const std::string &v);

    time_t m_value = 0;
    Threads::Sync::Mutex_Shared m_mutex;
};
} // namespace Abstract
} // namespace Memory
} // namespace Mantids30
