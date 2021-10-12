#include "mutex_shared.h"

using namespace CX2::Threads::Sync;

Mutex_Shared::Mutex_Shared()
{
    mutex = PTHREAD_RWLOCK_INITIALIZER;
}

Mutex_Shared::~Mutex_Shared()
{
    pthread_rwlock_destroy(&mutex);
}

void Mutex_Shared::lock()
{
    pthread_rwlock_wrlock(&mutex);
}

void Mutex_Shared::unlock()
{
    pthread_rwlock_unlock(&mutex);
}

void Mutex_Shared::lock_shared()
{
    pthread_rwlock_rdlock(&mutex);
}

void Mutex_Shared::unlock_shared()
{
    unlock();
}
