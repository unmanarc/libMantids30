#ifndef A_IPV4_H
#define A_IPV4_H

#include "abstract.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Vars {

class A_IPV4: public Abstract
{
public:
    A_IPV4();
    ~A_IPV4() override;
    A_IPV4& operator=(const in_addr & value)
    {
        setValue(value);
        return *this;
    }
    in_addr getValue();
    bool setValue(const in_addr & value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    in_addr value;
    Threads::Sync::Mutex_Shared mutex;
};

}}}

#endif // A_IPV4_H
