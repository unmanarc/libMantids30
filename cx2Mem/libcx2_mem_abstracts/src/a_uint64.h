#ifndef ABSVAR_UINT64_H
#define ABSVAR_UINT64_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_UINT64: public Abstract
{
public:
    A_UINT64();
    A_UINT64& operator=(uint64_t value)
    {
        setValue(value);
        return *this;
    }

    uint64_t getValue();
    bool setValue(const uint64_t &value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<uint64_t> value;
};

}}}

#endif // ABSVAR_UINT64_H
