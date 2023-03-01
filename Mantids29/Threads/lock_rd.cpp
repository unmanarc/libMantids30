#include "lock_rd.h"

using namespace Mantids29::Threads::Sync;

Lock_RD::Lock_RD(Mutex_Shared &mutex, bool dontAcquireLock)
{
    this->m_sharedMutex = &mutex;
    this->m_dontAcquireLock = dontAcquireLock;
    if (!dontAcquireLock) mutex.lockShared();
}

Lock_RD::~Lock_RD()
{
    if (!m_dontAcquireLock) m_sharedMutex->unlockShared();
}
