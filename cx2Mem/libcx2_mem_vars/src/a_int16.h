#ifndef A_INT16_H
#define A_INT16_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_INT16: public Abstract
{
public:
    A_INT16();
    A_INT16& operator=(int16_t value)
    {
        setValue(value);
        return *this;
    }

    int16_t getValue();
    bool setValue(int16_t value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<int16_t> value;
};
}}}

#endif // A_INT16_H
