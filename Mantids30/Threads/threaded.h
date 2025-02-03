#pragma once

#include <thread>
#include <atomic>
#include <memory>

namespace Mantids30 { namespace Threads {

/**
 * The Threaded class provides a wrapper for creating and managing threads.
 */
class Threaded : public std::enable_shared_from_this<Threaded>
{
public:
    Threaded();
    virtual ~Threaded();

    // THREAD ASSOCIATED:
    /**
     * Starts the thread with the specified Threaded object. (this class should always be declared as a shared_ptr)
     */
    void startInBackground();
    /**
     * Stops the thread.
     */
    void stop();

    // STATUS:
    /**
     * Returns whether the thread is currently running.
     */
    bool isRunning() const;

    // CALLEABLE FROM THREAD:
    /**
     * The function to be executed in the thread (internal).
     */
    void execRun();
    /**
     * Detaches the thread from the calling thread.
     */
    void detach();
    /**
     * Sets the function to be executed in the thread.
     * @param threadRunner The function to be executed.
     * @param context The object to be passed to the function.
     */
    void setThreadRunner(void (*threadRunner)(void *context), void *context);
    /**
     * Sets the function to be called when stopping the thread (cleanups?).
     * @param threadStopper The function to be called.
     * @param context The object to be passed to the function.
     */
    void setThreadStopper(void (*threadStopper)(void *context), void *context);

private:
    /**
     * The function executed in the thread.
     */
    static void bgRunner(const std::shared_ptr<Threaded> &t);

    /**
     * Waits for the thread to finish.
     */
    void join();

    // Member variables:
    void (*m_threadRunner)(void *runnerArg);  // The function executed in the thread.
    void * m_contextRunner;  // The object passed to the threadRunner function.
    void (*m_threadStopper)(void *stopperArg);  // The function called when stopping the thread.
    void * m_contextStopper;  // The object passed to the threadStopper function.
    std::atomic<bool> m_isRunning;  // Whether the thread is currently running.
    std::thread m_threadObj;  // The thread object.
};




}}

