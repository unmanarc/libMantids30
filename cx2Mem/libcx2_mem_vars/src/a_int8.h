#ifndef A_INT8_H
#define A_INT8_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_INT8: public Abstract
{
public:
    A_INT8();
    A_INT8& operator=(int8_t value)
    {
        setValue(value);
        return *this;
    }

    int8_t getValue();
    bool setValue(int8_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<int8_t> value;
};

}}}

#endif // A_INT8_H
