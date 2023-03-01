#ifndef THREADED_H
#define THREADED_H

#include <thread>
#include <atomic>

namespace Mantids29 { namespace Threads {

/**
 * The Threaded class provides a wrapper for creating and managing threads.
 */
class Threaded
{
public:
    Threaded();
    virtual ~Threaded();

    // THREAD ASSOCIATED:
    /**
     * Starts the thread with the specified Threaded object.
     * @param tc The shared pointer to the Threaded object to be passed in the new thread.
     */
    void start(const std::shared_ptr<Threaded> &threadedObject);
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
     * @param runnerArg The object to be passed to the function.
     */
    void setThreadRunner(void (*threadRunner)(void *runnerArg), void *runnerArg);
    /**
     * Sets the function to be called when stopping the thread (cleanups?).
     * @param threadStopper The function to be called.
     * @param stopperArg The object to be passed to the function.
     */
    void setThreadStopper(void (*threadStopper)(void *stopperArg), void *stopperArg);

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
    void (*threadRunner)(void *runnerArg);  // The function executed in the thread.
    void * runnerArg;  // The object passed to the threadRunner function.
    void (*threadStopper)(void *stopperArg);  // The function called when stopping the thread.
    void * stopperArg;  // The object passed to the threadStopper function.
    std::atomic<bool> running;  // Whether the thread is currently running.
    std::thread threadObj;  // The thread object.
};




}}

#endif // THREADED_H
