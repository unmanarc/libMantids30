#ifndef A_STRINGLIST_H
#define A_STRINGLIST_H

#include "a_var.h"
#include <list>
#include <mdz_thr_mutex/mutex_shared.h>

namespace Mantids { namespace Memory { namespace Abstract {

class STRINGLIST: public Var
{
public:
    STRINGLIST();
    STRINGLIST(const std::list<std::string> & value);
    STRINGLIST& operator=(const std::list<std::string> & value)
    {
        setValue(value);
        return *this;
    }
    std::list<std::string> getValue();
    bool setValue(const std::list<std::string> &value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    std::list<std::string> value;
    Threads::Sync::Mutex_Shared mutex;
};

}}}

#endif // A_STRINGLIST_H
