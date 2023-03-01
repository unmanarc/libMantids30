#include "lock_rw.h"

using namespace Mantids29::Threads::Sync;

Lock_RW::Lock_RW(Mutex_Shared &mutex, bool dontAcquireLock)
{
    this->m_sharedMutex = &mutex;
    this->m_dontAcquireLock = dontAcquireLock;
    if (!dontAcquireLock)
        mutex.lock();
}

Lock_RW::~Lock_RW()
{
    if (!m_dontAcquireLock)
        m_sharedMutex->unlock();
}
