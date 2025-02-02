#pragma once

#include "a_var.h"
#ifdef _WIN32
#include <winsock2.h>
#include <in6addr.h>
#else
#include <netinet/in.h>
#endif
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

class IPV6: public Var
{
public:
    IPV6();
    IPV6(const in6_addr &value);
    IPV6(const std::string &value);
    IPV6& operator=(const in6_addr & value)
    {
        setValue(value);
        return *this;
    }

    in6_addr getValue();
    bool setValue(const in6_addr & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    static std::string _toString(const in6_addr & value);
    static in6_addr _fromString(const std::string & value, bool * ok = nullptr);

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    in6_addr value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

