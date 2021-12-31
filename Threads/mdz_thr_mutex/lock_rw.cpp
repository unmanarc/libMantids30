#include "lock_rw.h"

using namespace Mantids::Threads::Sync;

Lock_RW::Lock_RW(Mutex_Shared &mutex, bool dontlock)
{
    this->mutex = &mutex;
    this->dontlock = dontlock;
    if (!dontlock)
        mutex.lock();
}

Lock_RW::~Lock_RW()
{
    if (!dontlock)
        mutex->unlock();
}
