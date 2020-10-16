#ifndef A_STRINGLIST_H
#define A_STRINGLIST_H

#include "abstract.h"
#include <list>
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Vars {

class A_STRINGLIST: public Abstract
{
public:
    A_STRINGLIST();
    ~A_STRINGLIST() override;
    A_STRINGLIST& operator=(const std::list<std::string> & value)
    {
        setValue(value);
        return *this;
    }
    std::list<std::string> getValue();
    bool setValue(const std::list<std::string> &value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::list<std::string> value;
    Threads::Sync::Mutex_Shared mutex;
};

}}}

#endif // A_STRINGLIST_H
