#ifndef LOCK_MUTEX_RW_H
#define LOCK_MUTEX_RW_H

#include "mutex_shared.h"

namespace Mantids29 { namespace Threads { namespace Sync {

/**
 * @brief A class that provides read-write locking mechanism using a shared mutex.
 *
 * The `Lock_RW` class provides a read-write locking mechanism using a shared mutex. When constructed,
 * the class takes a reference to a `Mutex_Shared` object and acquires a shared lock on it by default.
 * A shared lock allows multiple threads to simultaneously read from the protected resource but
 * prevents writing to it. If the `dontAcquireLock` parameter is set to `true` during construction, the
 * `Lock_RW` object is created without acquiring the lock.
 *
 * When the `Lock_RW` object is destroyed, the lock is released automatically.
 */
class Lock_RW
{
public:
    /**
     * @brief Constructs a `Lock_RW` object and acquires a shared lock on the given mutex.
     * @param mutex A reference to a `Mutex_Shared` object to lock.
     * @param dontAcquireLock If set to `true`, the lock is not acquired during construction.
     */
    Lock_RW(Mutex_Shared& mutex, bool dontAcquireLock = false);

    /**
     * @brief Destroys the `Lock_RW` object and releases the lock.
     */
    ~Lock_RW();

private:
    Mutex_Shared* m_sharedMutex; ///< A pointer to the shared mutex.
    bool m_dontAcquireLock; ///< Whether to acquire the lock on construction.
};


}}}

#endif // LOCK_MUTEX_RW_H
