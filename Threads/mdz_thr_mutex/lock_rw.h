#ifndef LOCK_MUTEX_RW_H
#define LOCK_MUTEX_RW_H

#include "mutex_shared.h"

namespace Mantids { namespace Threads { namespace Sync {

class Lock_RW
{
public:
    Lock_RW(Mutex_Shared & mutex, bool dontlock = false);
    ~Lock_RW();
private:
    Mutex_Shared * mutex;
    bool dontlock;
};

}}}

#endif // LOCK_MUTEX_RW_H
