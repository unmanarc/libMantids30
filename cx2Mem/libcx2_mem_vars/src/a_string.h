#ifndef A_STRING_H
#define A_STRING_H

#include "abstract.h"
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Vars {

class A_STRING: public Abstract
{
public:
    A_STRING();
    ~A_STRING() override;
    A_STRING& operator=(const std::string & value)
    {
        setValue(value);
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::string value;
    Threads::Sync::Mutex_Shared mutex;
};

}}}

#endif // A_STRING_H
