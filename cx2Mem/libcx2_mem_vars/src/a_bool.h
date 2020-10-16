#ifndef A_BOOL_H
#define A_BOOL_H

#include "abstract.h"
#include <stdint.h>
#include <atomic>


namespace CX2 { namespace Memory { namespace Vars {
class A_BOOL: public Abstract
{
public:
    A_BOOL();
    A_BOOL& operator=(bool value)
    {
        setValue(value);
        return *this;
    }

    bool getValue();
    bool setValue(bool value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;
private:
    std::atomic<bool> value;
};
}}}
#endif // A_BOOL_H
