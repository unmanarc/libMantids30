#ifndef LOCK_MUTEX_RD_H
#define LOCK_MUTEX_RD_H

#include "mutex_shared.h"

namespace Mantids29 { namespace Threads { namespace Sync {

/**
 * @brief A class that provides read-only locking mechanism using a shared mutex.
 *
 * The `Lock_RD` class provides a read-only locking mechanism using a shared mutex. When constructed,
 * the class takes a reference to a `Mutex_Shared` object and acquires a shared lock on it by default.
 * A shared lock allows multiple threads to simultaneously read from the protected resource but
 * prevents writing to it. If the `dontlock` parameter is set to `true` during construction, the
 * `Lock_RD` object is created without acquiring the lock.
 *
 * When the `Lock_RD` object is destroyed, the lock is released automatically.
 */
class Lock_RD
{
public:
    /**
     * @brief Constructs a `Lock_RD` object and acquires a shared lock on the given mutex.
     * @param mutex A reference to a `Mutex_Shared` object to lock.
     * @param dontlock If set to `true`, the lock is not acquired during construction.
     */
    Lock_RD(Mutex_Shared& mutex, bool dontAcquireLock = false);

    /**
     * @brief Destroys the `Lock_RD` object and releases the lock.
     */
    ~Lock_RD();

private:
    Mutex_Shared* m_sharedMutex; ///< A pointer to the shared mutex.
    bool m_dontAcquireLock; ///< Whether to acquire the lock during construction.
};

}}}


#endif // LOCK_MUTEX_RD_H
