#ifndef PASSWORD_MODES_H
#define PASSWORD_MODES_H

#include <stdint.h>
namespace Mantids { namespace Authentication {

enum Function
{
    FN_NOTFOUND=500,
    FN_PLAIN=0,
    FN_SHA256=1,
    FN_SHA512=2,
    FN_SSHA256=3,
    FN_SSHA512=4,
    FN_GAUTHTIME=5
};

}}

#endif // PASSWORD_MODES_H
