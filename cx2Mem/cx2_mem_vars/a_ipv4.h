#ifndef A_IPV4_H
#define A_IPV4_H

#include "a_var.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class IPV4: public Var
{
public:
    IPV4();
    IPV4(const in_addr & value);
    IPV4(const std::string & value);
    IPV4& operator=(const in_addr & value)
    {
        setValue(value);
        return *this;
    }
    in_addr getValue();
    bool setValue(const in_addr & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    static std::string _toString(const in_addr& value);
    static in_addr _fromString(const std::string &value, bool * ok = nullptr);
protected:
    Var * protectedCopy() override;

private:
    in_addr value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_IPV4_H
