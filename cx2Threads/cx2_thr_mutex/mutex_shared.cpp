#include "mutex_shared.h"
#include <stdexcept>
using namespace CX2::Threads::Sync;

Mutex_Shared::Mutex_Shared()
{
    int i=0;
    if ((i=pthread_rwlock_init(&mutex, nullptr)))
    {
        throw std::runtime_error("R/W Mutex Creating Failed with " + std::to_string(i));
    }
}

Mutex_Shared::~Mutex_Shared()
{
    pthread_rwlock_destroy(&mutex);
}

void Mutex_Shared::lock()
{
    int i=0;
    if ((i=pthread_rwlock_wrlock(&mutex)))
        throw std::runtime_error("Write Mutex Lock Failed with " + std::to_string(i));
}

void Mutex_Shared::unlock()
{
    int i=0;
    if ((i=pthread_rwlock_unlock(&mutex)))
        throw std::runtime_error("Mutex Unlock Failed with " + std::to_string(i) );
}

void Mutex_Shared::lock_shared()
{
    int i=0;
    if ((i=pthread_rwlock_rdlock(&mutex)))
        throw std::runtime_error("Read Mutex Lock Failed." + std::to_string(i));
}

void Mutex_Shared::unlock_shared()
{
    unlock();
}
