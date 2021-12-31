#ifndef LOCK_MUTEX_RD_H
#define LOCK_MUTEX_RD_H

#include "mutex_shared.h"

namespace Mantids { namespace Threads { namespace Sync {

class Lock_RD
{
public:
    Lock_RD(Mutex_Shared & mutex, bool dontlock = false);
    ~Lock_RD();
private:
    Mutex_Shared * mutex;
    bool dontlock;
};

}}}


#endif // LOCK_MUTEX_RD_H
