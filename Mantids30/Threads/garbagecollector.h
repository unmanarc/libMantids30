#pragma once

#include "threaded.h"

#include <condition_variable>
#include <mutex>

namespace Mantids30 { namespace Threads {

/**
 * @brief The GarbageCollector class provides a simple garbage collector base class for managing memory in a program.
 *
 * The class provides a background thread that intervalically runs the garbage collection function provided by the user.
 * The interval between each garbage collection can be configured using setInterval function.
 */
class GarbageCollector
{
public:
    /**
     * @brief Constructs a new GarbageCollector object with the given garbage collection interval.
     *
     * @param intervalMS The interval between each garbage collection in milliseconds.
     */
    GarbageCollector(const uint32_t& intervalMS = 3000);
    /**
     * @brief Destructs the GarbageCollector object and stops the garbage collector thread.
     */
    virtual ~GarbageCollector();

    /**
     * @brief Starts the garbage collector thread with the given garbage collection function and object.
     *
     * @param gcFunction The garbage collection function to run intervalically.
     * @param parameter The object to pass to the garbage collection function.
     * @param threadName The name of the garbage collector thread.
     */
    void startGarbageCollector(void (*garbageCollectorFunction)(void* parameter), void* parameter, const char* threadName = "GC:Generic");

    /**
     * @brief Loops the garbage collector function until it completes. (internal function, don't call)
     */
    void loopGarbageCollector();

    /**
     * @brief Sets the garbage collection interval in milliseconds to the given value.
     *
     * @param msInterval The interval between each garbage collection in milliseconds.
     */
    void setGarbageCollectorInterval(const uint32_t& msInterval);

    /**
     * @brief stopGarbageCollector Stop the garbage collector (manually)
     */
    void stopGarbageCollector();

private:
    /**
     * @brief The gcLoop function runs the garbage collector thread loop.
     *
     * @param gcThreadClass A pointer to the GarbageCollector object.
     * @param threadName The name of the garbage collector thread.
     */
    static void backgroundGarbageCollectorLoop(GarbageCollector* gcThreadClass, const char* threadName);

    std::mutex m_endNotificationMutex; ///< The mutex to ensure thread safety when notifying the end of the garbage collector thread loop.
    std::condition_variable m_endNotificationCondition; ///< The condition variable to wait for the end of the garbage collector thread loop.

    void (*m_gcFunction)(void* parameter); ///< The garbage collection function to run intervalically.
    void* m_gcParameter; ///< The object to pass to the garbage collection function.

    bool m_gcFinished; ///< Whether the garbage collector thread has finished.
    std::thread m_gcThreadObject; ///< The garbage collector thread object.
    std::atomic<uint32_t> m_gcIntervalMs; ///< The interval between each garbage collection in milliseconds.
};



}}

