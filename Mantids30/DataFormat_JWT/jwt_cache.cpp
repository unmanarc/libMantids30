#include "jwt.h"
#include <boost/thread/pthread/shared_mutex.hpp>

using namespace Mantids30::DataFormat;

void JWT::Cache::setCacheMaxByteCount(std::size_t maxByteCount)
{
    {
        boost::unique_lock<boost::shared_mutex> writeLock(m_cachedTokensMutex);

        bool doEvict = false;
        if (m_cacheMaxByteCount > maxByteCount)
            doEvict = true;

        m_cacheMaxByteCount = maxByteCount;
    }
    evictCache();
}

std::size_t JWT::Cache::getCacheMaxByteCount()
{
    boost::shared_lock<boost::shared_mutex> readLock(m_cachedTokensMutex);
    return m_cacheMaxByteCount;
}

void JWT::Cache::clear()
{
    boost::unique_lock<boost::shared_mutex> writeLock(m_cachedTokensMutex);
    while (!m_cacheQueue.empty()) m_cacheQueue.pop();
    m_tokenCache.clear();
}

bool JWT::Cache::isEnabled()
{
    boost::shared_lock<boost::shared_mutex> readLock(m_cachedTokensMutex);
    return m_enabled;
}

void JWT::Cache::setEnabled(bool newEnabled)
{
    boost::unique_lock<boost::shared_mutex> writeLock(m_cachedTokensMutex);
    if (!newEnabled)
    {
        // Destroy the cache...
        while (!m_cacheQueue.empty()) m_cacheQueue.pop();
        m_tokenCache.clear();
    }
    m_enabled = newEnabled;
}

bool JWT::Cache::checkToken(const std::string &payload)
{
    boost::shared_lock<boost::shared_mutex> readLock(m_cachedTokensMutex);

    // Don't report any token
    if (!m_enabled)
        return false;

    auto it = m_tokenCache.find(payload);
    return it != m_tokenCache.end();
}

void JWT::Cache::add(const std::string &payload)
{
    {
        boost::unique_lock<boost::shared_mutex> writeLock(m_cachedTokensMutex);

        // TODO: use hash instead? / payload can be huge.

        // Don't add anything to the cache...
        if (!m_enabled)
            return;

        m_cacheQueue.push(payload);
        m_tokenCache[payload] = true;
        m_cacheCurrentByteCount += payload.size();
    }
    evictCache();
}

void JWT::Cache::evictCache()
{
    boost::unique_lock<boost::shared_mutex> writeLock(m_cachedTokensMutex);

    while (m_cacheCurrentByteCount > m_cacheMaxByteCount)
    {
        const std::string& evictedToken = m_cacheQueue.front();
        m_cacheCurrentByteCount -= evictedToken.size();
        m_tokenCache.erase(evictedToken);
        m_cacheQueue.pop();
    }
}
