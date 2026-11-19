#pragma once

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

#include "safe_mapitem.h"

namespace Mantids30::Core::Threads::Safe {

/**
 * @brief A thread-safe registry with shared ownership of MapItem objects.
 *
 * The mutex protects membership, not the contents of individual items.
 * Removing an entry stops its readers but does not wait for shared owners.
 * @tparam T The key type.
 */
template<class T>
class Map2
{
public:
    Map2() = default;

    /** @return A snapshot of the registered keys. */
    std::set<T> getKeys();

    bool isMember(const T &key);

    /**
     * @brief Registers a non-null item without replacing an existing key.
     * @return false for null items or duplicate keys.
     */
    bool addElement(const T &key, const std::shared_ptr<MapItem> &element);

    /**
     * @return Shared ownership of the item, or nullptr if the key is absent.
     * Ownership is released automatically when the returned shared_ptr is reset.
     */
    std::shared_ptr<MapItem> openElement(const T &key);

    /**
     * @brief Removes the entry and calls stopReaders() outside the map lock.
     * Existing owners keep the item alive and must synchronize access to its data.
     * @return false if the key is absent.
     */
    bool destroyElement(const T &key);

    /**
     * @brief Waits for an empty registry, not for item destruction or stop callbacks.
     */
    void waitForEmptyMap();

private:
    std::map<T, std::shared_ptr<MapItem>> m_keyValueMap;
    std::condition_variable m_noItemsOnMapCondition;
    std::mutex m_keyValueMapMutex;
};

template<class T>
std::set<T> Map2<T>::getKeys()
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    std::set<T> ret;
    for (const auto &i : m_keyValueMap)
    {
        ret.insert(i.first);
    }
    return ret;
}

template<class T>
bool Map2<T>::isMember(const T &key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    return m_keyValueMap.find(key) != m_keyValueMap.end();
}

template<class T>
bool Map2<T>::addElement(const T &key, const std::shared_ptr<MapItem> &element)
{
    if (!element)
    {
        return false;
    }
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    return m_keyValueMap.emplace(key, element).second;
}

template<class T>
std::shared_ptr<MapItem> Map2<T>::openElement(const T &key)
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    const auto it = m_keyValueMap.find(key);
    return it == m_keyValueMap.end() ? nullptr : it->second;
}

template<class T>
bool Map2<T>::destroyElement(const T &key)
{
    std::shared_ptr<MapItem> item;
    {
        std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
        const auto it = m_keyValueMap.find(key);
        if (it == m_keyValueMap.end())
        {
            return false;
        }
        item = std::move(it->second);
        m_keyValueMap.erase(it);
        if (m_keyValueMap.empty())
        {
            m_noItemsOnMapCondition.notify_all();
        }
    }
    item->stopReaders();
    return true;
}

template<class T>
void Map2<T>::waitForEmptyMap()
{
    std::unique_lock<std::mutex> lock(m_keyValueMapMutex);
    m_noItemsOnMapCondition.wait(lock, [this] { return m_keyValueMap.empty(); });
}

} // namespace Mantids30::Core::Threads::Safe
