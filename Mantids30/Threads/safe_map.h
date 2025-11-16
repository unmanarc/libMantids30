#pragma once

#include <mutex>
#include <condition_variable>
#include <map>
#include <set>
#include <atomic>

#include <unistd.h>
#include <stdexcept>

#include "safe_mapitem.h"

namespace Mantids30 { namespace Threads { namespace Safe {

/**
 * @brief The Map class provides a thread-safe map data structure.
 *
 * This class uses std::map as the underlying data structure for key-value pairs.
 * The class provides functions for adding, checking, and removing elements from the map.
 * The class also provides functions for waiting until the map is empty.
 *
 * @tparam T The type of the keys in the map.
 */
template <class T>
class Map
{
public:
    /**
     * @brief Constructs a new Map object.
     */
    Map() = default;

    /**
     * @brief Gets the set of keys in the map.
     *
     * @return A set containing all the keys in the map.
     */
    std::set<T> getKeys();

    /**
     * @brief Checks if the given key exists in the map.
     *
     * @param key The key to check.
     * @return true if the key exists in the map, false otherwise.
     */
    bool isMember(const T& key);

    /**
     * @brief Adds an element with the given key to the map.
     *
     * @param key The key for the element.
     * @param element The element to add.
     * @return true if the element was added successfully, false otherwise.
     */
    bool addElement(const T& key, MapItem* element);

    /**
     * @brief Opens the element with the given key for reading.
     *
     * Multiple readers can read the element simultaneously.
     *
     * @param key The key for the element.
     * @return A pointer to the MapItem object associated with the key, or nullptr if the key is not found.
     */
    MapItem* openElement(const T& key);

    /**
     * @brief Releases the element with the given key after reading.
     *
     * @param key The key for the element.
     * @return true if the element was released successfully, false otherwise.
     */
    bool releaseElement(const T& key);

    /**
     * @brief Destroys the element with the given key.
     *
     * @param key The key for the element.
     * @return true if the element was destroyed successfully, false otherwise.
     */
    bool destroyElement(const T key);

    /**
     * @brief Waits until the map is empty.
     */
    void waitForEmptyMap();

private:
    /**
     * @brief The MapElement struct stores information about a map element.
     *
     * Each MapElement stores a MapItem pointer, the number of readers currently
     * accessing the element, and a condition variable to wait for the number of
     * readers to become zero.
     */
    struct MapElement
    {
        MapElement()
        {
            numReaders = 0;
            item = nullptr;
        }
        MapItem* item;
        std::atomic<uint32_t> numReaders;
        std::condition_variable noReadersCondition;
    };


    std::map<T, MapElement> m_keyValueMap; ///< The map storing key-value pairs.
    std::condition_variable m_noItemsOnMapCondition; ///< The condition variable to wait for the map to become empty.
    std::mutex m_keyValueMapMutex; ///< The mutex to ensure thread safety when accessing the map.
};

template<class T>
std::set<T> Map<T>::getKeys()
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    std::set<T> ret;
    for (const auto & i : m_keyValueMap) ret.insert(i.first);
    return ret;
}

template<class T>
bool Map<T>::isMember(const T &key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    return (m_keyValueMap.find(key) != m_keyValueMap.end());
}

template<class T>
bool Map<T>::addElement(const T &key, MapItem *element)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    if (m_keyValueMap.find(key) == m_keyValueMap.end())
    {
        m_keyValueMap[key].item = element;
        return true;
    }
    return false;
}

// FAST
template<class T>
MapItem *Map<T>::openElement(const T &key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    if (m_keyValueMap.find(key) != m_keyValueMap.end() && m_keyValueMap[key].item)
    {
        m_keyValueMap[key].numReaders++;
        return m_keyValueMap[key].item;;
    }
    return nullptr;
}

// FAST
template<class T>
bool Map<T>::releaseElement(const T &key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);

    if (m_keyValueMap.find(key) != m_keyValueMap.end())
    {
        if (m_keyValueMap[key].numReaders==0)
            throw std::runtime_error("Invalid close on Mutex MAP");

        m_keyValueMap[key].numReaders--;
        // if no more readers... emit the signal to notify it:
        if (m_keyValueMap[key].numReaders == 0)
        {
            m_keyValueMap[key].noReadersCondition.notify_one();
        }
        return true;
    }
    return false;
}

template<class T>
bool Map<T>::destroyElement(const T key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);

    if (    m_keyValueMap.find(key) != m_keyValueMap.end()
            && m_keyValueMap[key].item != nullptr )
    {
        // No more open readers and destroy element.. (inaccesible for openElement and for destroyElement)
        MapItem * delElement = m_keyValueMap[key].item;
        m_keyValueMap[key].item = nullptr;

        for (;m_keyValueMap[key].numReaders != 0;)
        {
            delElement->stopReaders();
            // unlock and retake the lock until signal is emited.
            m_keyValueMap[key].noReadersCondition.wait(lock);
        }

        // Now is time to delete and remove.
        delete delElement;
        m_keyValueMap.erase(key);
        if (m_keyValueMap.empty())
        {
            m_noItemsOnMapCondition.notify_one();
        }
        return true;
    }
    return false;
}

template<class T>
void Map<T>::waitForEmptyMap()
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    m_noItemsOnMapCondition.wait(lock, [this]{ return m_keyValueMap.empty(); });
}

}}}

