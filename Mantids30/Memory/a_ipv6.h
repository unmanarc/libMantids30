#pragma once

#include "a_var.h"
#ifdef _WIN32
#include <in6addr.h>
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif
#include <shared_mutex>


namespace Mantids30::Memory::Abstract {

class IPV6 : public Var
{
public:
    IPV6();
    IPV6(const in6_addr &value);
    IPV6(const std::string &value);
    IPV6 &operator=(const in6_addr &value)
    {
        setValue(value);
        return *this;
    }

    in6_addr getValue();
    bool setValue(const in6_addr &value);

    void *getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string &value) override;

    static std::string _toString(const in6_addr &value);
    static in6_addr _fromString(const std::string &value, bool *ok = nullptr);

    Json::Value toJSON() override;
    bool fromJSON(const Json::Value &value) override;

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    in6_addr m_value{0};
    std::shared_mutex m_mutex;
};

} // namespace Mantids30::Memory::Abstract
