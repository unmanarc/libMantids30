#ifndef MUTEX_RW_H
#define MUTEX_RW_H

#include <pthread.h>

namespace Mantids29 { namespace Threads { namespace Sync {

/**
 * @brief The Mutex_Shared class provides a wrapper for a shared mutex using the pthread library.
 *
 * The class provides functions for locking and unlocking the mutex in exclusive and shared mode.
 */
class Mutex_Shared
{
public:
    /**
     * @brief Constructs a new Mutex_Shared object.
     */
    Mutex_Shared();

    /**
     * @brief Destructs the Mutex_Shared object and releases the mutex.
     */
    ~Mutex_Shared();

    /**
     * @brief Locks the mutex in exclusive mode.
     *
     * This function blocks until the mutex can be locked in exclusive mode.
     */
    void lock();

    /**
     * @brief Unlocks the mutex locked in exclusive mode.
     */
    void unlock();

    /**
     * @brief Locks the mutex in shared mode.
     *
     * This function blocks until the mutex can be locked in shared mode.
     */
    void lockShared();

    /**
     * @brief Unlocks the mutex locked in shared mode.
     */
    void unlockShared();

private:
    pthread_rwlock_t m_sharedMutex; ///< The shared mutex.
};


}}}

#endif // MUTEX_RW_H
