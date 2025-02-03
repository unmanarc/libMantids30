#pragma once

#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Mantids30 { namespace Threads { namespace Safe {

/**
 * @brief The Thread Safe Queue class
 */
template <class T>
class Queue
{
public:
    Queue()
    {
        m_maxItems = &m_localMaxItems;
        m_localMaxItems = 0xFFFFFFFF;
    }
    ~Queue()
    {
        std::unique_lock<std::mutex> lock(m_mQueue);
        while (m_queue.size())
        {
            delete m_queue.front();
            m_queue.pop();
        }
    }
    /**
     * @brief setMaxItemsAsExternalPointer Set Max Queue Items as External Pointer
     * @param maxItems Max Queue Items Pointer.
     */
    void setMaxItemsAsExternalPointer(std::atomic<size_t> * maxItems);
    /**
     * @brief setMaxItems Set the value of the current max items object
     * @param maxItems value
     */
    void setMaxItems(const size_t & maxItems);
    /**
     * @brief getMaxItems Get max items allowed
     * @return integer with max items allowed.
     */
    size_t getMaxItems();
    /**
     * @brief push Push a new item
     * @param item item to be pushed to the queue
     * @param tmout_msecs Timeout in milliseconds if the queue is full (default: 100 seconds), or zero if you don't want to wait
     * @return
     */
    bool push(T * item ,const uint32_t & tmout_msecs = 100000);
    /**
     * @brief pop Pop the item, you should delete/remove it after using "destroyItem" function
     * @param tmout_msecs Timeout in milliseconds if the queue is empty (default: 100 seconds), or zero if you don't want to wait
     * @return the first item in the queue
     */
    T * pop(const uint32_t & tmout_msecs = 100000);
    /**
     * @brief size Current Queue Size
     * @return size in size_t format...
     */
    size_t size();

private:
    std::atomic<size_t> * m_maxItems;
    std::atomic<size_t> m_localMaxItems;
    std::mutex m_mQueue;
    std::condition_variable m_notEmptyCond, m_notFullCond;
    std::queue<T *> m_queue;
};



template<class T>
void Queue<T>::setMaxItemsAsExternalPointer(std::atomic<size_t> *maxItems)
{
    this->m_maxItems = maxItems;
}

template<class T>
void Queue<T>::setMaxItems(const size_t &maxItems)
{
    (*this->m_maxItems) = maxItems;
}

template<class T>
size_t Queue<T>::getMaxItems()
{
    return *m_maxItems;
}

template<class T>
bool Queue<T>::push(T *item, const uint32_t &tmout_msecs)
{
    std::unique_lock<std::mutex> lock(m_mQueue);
    while (m_queue.size()>=*m_maxItems)
    {
        if ( tmout_msecs == 0 )
            return false;
        if (m_notFullCond.wait_for(lock, std::chrono::milliseconds(tmout_msecs)) == std::cv_status::timeout)
            return false;
    }
    m_queue.push(item);
    lock.unlock();
    m_notEmptyCond.notify_one();
    return true;
}

template<class T>
T *Queue<T>::pop(const uint32_t &tmout_msecs)
{
    std::unique_lock<std::mutex> lock(m_mQueue);
    while (m_queue.empty())
    {
        if ( tmout_msecs == 0 )
            return nullptr;
        if (m_notEmptyCond.wait_for(lock, std::chrono::milliseconds(tmout_msecs)) == std::cv_status::timeout)
            return nullptr;
    }
    T * r = m_queue.front();
    m_queue.pop();
    lock.unlock();
    m_notFullCond.notify_one();
    return r;
}

template<class T>
size_t Queue<T>::size()
{
    std::unique_lock<std::mutex> lock(m_mQueue);
    return m_queue.size();
}

}}}

