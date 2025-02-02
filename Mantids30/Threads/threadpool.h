#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <random>
#include <thread>
#include <queue>
#include <map>
#include <condition_variable>

namespace Mantids30 { namespace Threads {

namespace Pool {


// TODO: statistics
/**
 * @brief Advanced Thread Pool.
 */
class ThreadPool
{
public:

    struct Task
    {
        Task()
        {
            task = nullptr;
            data = nullptr;
        }

        bool isNull()
        {
            return task == nullptr && data == nullptr;
        }

        void (*task) (std::shared_ptr<void>);
        std::shared_ptr<void> data;
    };

    struct TasksQueue
    {
        TasksQueue()
        {
            init=true;
        }
        std::queue<Task> tasks;
        std::condition_variable cond_removedElement;
        bool init;
    };

    /**
     * @brief ThreadPool Initialize thread pool
     * @param threadsCount concurrent threads initialized
     * @param taskQueues available queues
     */
    ThreadPool(uint32_t threadsCount = 52, uint32_t taskQueues = 36);
    ~ThreadPool();
    /**
     * @brief start Start task consumer threads
     */
    void start();
    /**
     * @brief stop Terminate to process current tasks and stop task consumer threads (also called from destructor)
     */
    void stop();
    /**
     * @brief addTask Add Task
     * @param task task function
     * @param taskData taskData passed to task
     * @param timeoutMS timeout if insertion queue is full
     * @param key key used to determine the priority schema
     * @param priority value between (0-1] to determine how many queues are available for insertion
     * @return true if inserted, false if timed out or during stop
     */
    bool pushTask(void (*task)(std::shared_ptr<void>), std::shared_ptr<void> taskData , uint32_t timeoutMS = static_cast<uint32_t>(-1), const float & priority=0.5, const std::string & key = "");
    /**
     * @brief popTask function used by thread processor
     * @return task
     */
    Task popTask();

    /**
     * @brief getMaxTasksPerQueue Retrieves the maximum number of tasks a single queue can hold before it reaches capacity.
     *        Each queue operates in a wait mode and will reject additional tasks if this limit is exceeded.
     * @return Maximum number of tasks per queue.
     */
    uint32_t getMaxTasksPerQueue() const;
    /**
     * @brief setMaxTasksPerQueue Sets the maximum number of tasks that a single queue can store.
     *        This value determines the queue capacity and helps prevent overload by limiting queued tasks.
     * @param value Maximum number of tasks per queue.
     */
    void setMaxTasksPerQueue(const uint32_t &value);



private:

    static void taskProcessor(ThreadPool * tp);

    size_t getRandomQueueByKey(const std::string & key, const float & priority);
    TasksQueue * getRandomTaskQueueWithElements(  );

    // TERMINATION:
    bool terminate;

    // LIMITS:
    std::atomic<uint32_t> maxTasksPerQueue;

    // THREADS:
    std::map<size_t,std::thread> threads;
    uint32_t threadsCount;

    // QUEUE OPERATIONS/CONDITIONS:
    std::map<size_t,TasksQueue> queues;
    std::mutex mutexQueues;
    std::condition_variable cond_insertedElement;
    std::condition_variable empty;
    uint32_t queuedElements;

    // RANDOM:
    std::hash<std::string> hash_fn;
    std::mutex mutexRandom;
    std::minstd_rand0 lRand;
};

}

}}


// TODO: Failed task what to do?, using

