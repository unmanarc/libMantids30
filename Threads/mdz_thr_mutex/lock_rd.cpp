#include "lock_rd.h"

using namespace Mantids::Threads::Sync;

Lock_RD::Lock_RD(Mutex_Shared &mutex, bool dontlock)
{
    this->mutex = &mutex;
    this->dontlock = dontlock;
    if (!dontlock) mutex.lock_shared();
}

Lock_RD::~Lock_RD()
{
    if (!dontlock) mutex->unlock_shared();
}
