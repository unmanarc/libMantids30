#ifndef ABSVAR_INT64_H
#define ABSVAR_INT64_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_INT64: public Abstract
{
public:
    A_INT64();
    A_INT64& operator=(const int64_t &value)
    {
        setValue(value);
        return *this;
    }

    int64_t getValue();
    bool setValue(const int64_t &value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<int64_t> value;
};

}}}

#endif // ABSVAR_INT64_H
