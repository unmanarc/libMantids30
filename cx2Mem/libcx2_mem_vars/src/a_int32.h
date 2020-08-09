#ifndef ABSVAR_INT32_H
#define ABSVAR_INT32_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_INT32: public Abstract
{
public:
    A_INT32();
    A_INT32& operator=(int32_t value)
    {
        setValue(value);
        return *this;
    }

    int32_t getValue();
    bool setValue(int32_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<int32_t> value;
};

}}}

#endif // ABSVAR_INT32_H
