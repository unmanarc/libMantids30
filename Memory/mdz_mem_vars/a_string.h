#ifndef A_STRING_H
#define A_STRING_H

#include "a_var.h"
#include <mdz_thr_mutex/mutex_shared.h>

namespace Mantids { namespace Memory { namespace Abstract {

class STRING: public Var
{
public:
    STRING();
    STRING(const std::string & value);
    STRING& operator=(const std::string & value)
    {
        setValue(value);
        return *this;
    }
    std::string getValue();
    bool setValue(const std::string &value);
    bool setValue(const char * value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    std::string value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_STRING_H
