#ifndef MUTEX_RW_H
#define MUTEX_RW_H

#include <pthread.h>

namespace CX2 { namespace Threads { namespace Sync {

class Mutex_Shared
{
public:
    Mutex_Shared();
    ~Mutex_Shared();

    void lock();
    void unlock();

    void lock_shared();
    void unlock_shared();

private:
    pthread_rwlock_t mutex;

};

}}}

#endif // MUTEX_RW_H
