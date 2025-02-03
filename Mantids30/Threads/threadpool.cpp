#include "threadpool.h"

#include <Mantids30/Helpers/random.h>

using namespace Mantids30::Threads::Pool;

ThreadPool::ThreadPool(uint32_t threadsCount, uint32_t taskQueues)
{
    std::random_device rd;
    m_lRand.seed(rd());

    setMaxTasksPerQueue(100);

    m_terminate = false;
    m_queuedElements = 0;
    this->m_threadCount = threadsCount;
    for (size_t i =0; i<taskQueues;i++)
    {
        m_queues[i].init = true;
    }
}

ThreadPool::~ThreadPool()
{
    stop();
    for (uint32_t i =0; i<m_threadCount;i++)
        m_threads[i].join();
}

void ThreadPool::start()
{
    for (size_t i =0; i<m_threadCount;i++)
    {
        m_threads[i] = std::thread(taskProcessor, this);
    }
}

void ThreadPool::stop()
{
    std::unique_lock<std::mutex> lk(m_queuesMutex);
    m_terminate = true;
    lk.unlock();
    m_insertedElementCond.notify_all();
}

bool ThreadPool::pushTask(void (*task)(std::shared_ptr<void>), std::shared_ptr<void> taskData, uint32_t timeoutMS,  const float &priority, const std::string &key)
{
    size_t currentQueue = getRandomQueueByKey(key,priority);

    // TODO: put to best place first
    std::unique_lock<std::mutex> lk(m_queuesMutex);

    // Don't insert on termination...
    if (m_terminate)
        return false;

    // Check if the queue is up the limit
    while ( m_queues[currentQueue].tasks.size() > m_maxTasksPerQueue  )
    {
        if (timeoutMS == static_cast<uint32_t>(-1))
        {
            m_queues[currentQueue].cond_removedElement.wait(lk);
        }
        else
        {
            if (m_queues[currentQueue].cond_removedElement.wait_for(lk, std::chrono::milliseconds(timeoutMS)) == std::cv_status::timeout)
            {
                return false;
            }
        }
    }

    // Now is not full, insert it.
    Task toInsert;
    toInsert.data = taskData;
    toInsert.task = task;
    m_queues[currentQueue].tasks.push( toInsert );

    // Notify that there is one element in one of the lists...
    lk.unlock();
    m_insertedElementCond.notify_one();
    return true;
}

ThreadPool::Task ThreadPool::popTask()
{
#ifndef _WIN32
     pthread_setname_np(pthread_self(), "tp_poptask");
#endif

    // lock and wait for an incoming task
    std::unique_lock<std::mutex> lk(m_queuesMutex);

    TasksQueue * tq = getRandomTaskQueueWithElements();
    while ( tq == nullptr )
    {
        // No available elements...
        m_insertedElementCond.wait(lk);

        // On termination, empty queue means exit
        if ( m_terminate && (tq=getRandomTaskQueueWithElements()) == nullptr)
        {
            Task r;
            return r;
        }
        else if (!m_terminate)
            tq = getRandomTaskQueueWithElements();
    }

    Task r= tq->tasks.front();
    tq->tasks.pop();

    // Notify!
    lk.unlock();
    tq->cond_removedElement.notify_one();
    return r;
}

ThreadPool::TasksQueue *ThreadPool::getRandomTaskQueueWithElements()
{
    std::vector<size_t> fullVector;
    // Randomize the full vector
    for (size_t i=0; i<m_queues.size(); ++i) fullVector.push_back(i);
    std::uniform_int_distribution<size_t> dis;
    m_randomMutex.lock();
    Mantids30::Helpers::Random::safe_random_shuffle(fullVector.begin(), fullVector.end(),static_cast<size_t>(dis(m_lRand)));
    m_randomMutex.unlock();

    // Iterate full Vector...
    for ( size_t i : fullVector )
    {
        if (!m_queues[i].tasks.empty())
        {
            return &(m_queues[i]);
        }
    }
    return nullptr;
}

size_t ThreadPool::getRandomQueueByKey(const std::string &key, const float &priority)
{
    size_t x;
    std::vector<size_t> reducedVector;
    std::vector<size_t> fullVector;

    // Convert priority in vector elements...
    size_t elements = static_cast<size_t>(m_queues.size()*priority);
    if (elements==0) elements = 1;
    if (elements>m_queues.size()) elements = m_queues.size();

    // Randomize the full vector using the hash of key
    for (size_t i=0; i<m_queues.size(); ++i) fullVector.push_back(i);
    Mantids30::Helpers::Random::safe_random_shuffle(fullVector.begin(), fullVector.end(), m_hashFunction(key));

    // Copy the first n-elements (based on priority)
    for (size_t i=0;i<elements;i++) reducedVector.push_back(fullVector[i]);

    // Get random element from the reduced vector:
    std::uniform_int_distribution<> dis(0, static_cast<int>(elements-1));
    m_randomMutex.lock();
    x = reducedVector.at( static_cast<size_t>(dis(m_lRand)) );
    m_randomMutex.unlock();

    return x;
}

uint32_t ThreadPool::getMaxTasksPerQueue() const
{
    return m_maxTasksPerQueue;
}

void ThreadPool::setMaxTasksPerQueue(const uint32_t &value)
{
    m_maxTasksPerQueue = value;

    for (auto & i : m_queues)
        i.second.cond_removedElement.notify_all();
}

void ThreadPool::taskProcessor(ThreadPool *tp)
{
    for (Task task = tp->popTask();
         !task.isNull();
         task = tp->popTask())
    {
        //std::cout << "ejecutando " << task.data << std::endl << std::flush;
        task.task(task.data);
    }
}
