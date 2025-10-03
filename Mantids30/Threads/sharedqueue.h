#pragma once
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>

namespace Mantids30 { namespace Threads { namespace Safe {

/**
* @brief The Thread Safe Queue class with shared_ptr
*/
template <class T>
class SharedQueue
{
public:
    SharedQueue()
    {
        m_maxItems = &m_localMaxItems;
        m_localMaxItems = 0xFFFFFFFF;
    }

    ~SharedQueue()
    {
        // No need for manual cleanup with shared_ptr
        std::unique_lock<std::mutex> lock(m_mQueue);
        while (!m_queue.empty())
        {
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
     * @param item shared_ptr to item to be pushed to the queue
     * @param tmout_msecs Timeout in milliseconds if the queue is full (default: 100 seconds), or zero if you don't want to wait
     * @return true if pushed successfully, false if timeout or no wait
     */
    bool push(std::shared_ptr<T> item, const uint32_t & tmout_msecs = 100000);

    /**
     * @brief pop Pop the item (automatically managed by shared_ptr)
     * @param tmout_msecs Timeout in milliseconds if the queue is empty (default: 100 seconds), or zero if you don't want to wait
     * @return shared_ptr to the first item in the queue or nullptr if timeout/empty
     */
    std::shared_ptr<T> pop(const uint32_t & tmout_msecs = 100000);

    /**
     * @brief size Current Queue Size
     * @return size in size_t format
     */
    size_t size();

    /**
     * @brief empty Check if queue is empty
     * @return true if empty, false otherwise
     */
    bool empty();

private:
    std::atomic<size_t> * m_maxItems;
    std::atomic<size_t> m_localMaxItems;
    std::mutex m_mQueue;
    std::condition_variable m_notEmptyCond, m_notFullCond;
    std::queue<std::shared_ptr<T>> m_queue;
};

template<class T>
void SharedQueue<T>::setMaxItemsAsExternalPointer(std::atomic<size_t> *maxItems)
{
    this->m_maxItems = maxItems;
}

template<class T>
void SharedQueue<T>::setMaxItems(const size_t &maxItems)
{
    (*this->m_maxItems) = maxItems;
}

template<class T>
size_t SharedQueue<T>::getMaxItems()
{
    return *m_maxItems;
}

template<class T>
bool SharedQueue<T>::push(std::shared_ptr<T> item, const uint32_t &tmout_msecs)
{
    if (!item) // Don't allow null shared_ptr
        return false;

    std::unique_lock<std::mutex> lock(m_mQueue);

    while (m_queue.size() >= *m_maxItems)
    {
        if (tmout_msecs == 0)
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
std::shared_ptr<T> SharedQueue<T>::pop(const uint32_t &tmout_msecs)
{
    std::unique_lock<std::mutex> lock(m_mQueue);

    while (m_queue.empty())
    {
        if (tmout_msecs == 0)
            return nullptr;

        if (m_notEmptyCond.wait_for(lock, std::chrono::milliseconds(tmout_msecs)) == std::cv_status::timeout)
            return nullptr;
    }

    std::shared_ptr<T> result = m_queue.front();
    m_queue.pop();
    lock.unlock();
    m_notFullCond.notify_one();
    return result;
}

template<class T>
size_t SharedQueue<T>::size()
{
    std::unique_lock<std::mutex> lock(m_mQueue);
    return m_queue.size();
}

template<class T>
bool SharedQueue<T>::empty()
{
    std::unique_lock<std::mutex> lock(m_mQueue);
    return m_queue.empty();
}

}}} // namespace Mantids30::Threads::Safe
