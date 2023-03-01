#include "mutex_shared.h"
#include <stdexcept>
using namespace Mantids29::Threads::Sync;

Mutex_Shared::Mutex_Shared()
{
    int i=0;
    if ((i=pthread_rwlock_init(&m_sharedMutex, nullptr)))
    {
        throw std::runtime_error("R/W Mutex Creating Failed with " + std::to_string(i));
    }
}

Mutex_Shared::~Mutex_Shared()
{
    pthread_rwlock_destroy(&m_sharedMutex);
}

void Mutex_Shared::lock()
{
    int i=0;
    if ((i=pthread_rwlock_wrlock(&m_sharedMutex)))
        throw std::runtime_error("Write Mutex Lock Failed with " + std::to_string(i));
}

void Mutex_Shared::unlock()
{
    int i=0;
    if ((i=pthread_rwlock_unlock(&m_sharedMutex)))
        throw std::runtime_error("Mutex Unlock Failed with " + std::to_string(i) );
}

void Mutex_Shared::lockShared()
{
    int i=0;
    if ((i=pthread_rwlock_rdlock(&m_sharedMutex)))
        throw std::runtime_error("Read Mutex Lock Failed." + std::to_string(i));
}

void Mutex_Shared::unlockShared()
{
    unlock();
}
