#ifndef ABSVAR_DOUBLE_H
#define ABSVAR_DOUBLE_H
#include "abstract.h"

#include <atomic>

namespace CX2 { namespace Memory { namespace Vars {

class A_DOUBLE: public Abstract
{
public:
    A_DOUBLE();
    A_DOUBLE& operator=(double value)
    {
        setValue(value);
        return *this;
    }

    double getValue();
    void setValue(const double & value);

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Abstract * protectedCopy() override;

private:
    std::atomic<double> value;
};

}}}

#endif // ABSVAR_DOUBLE_H
