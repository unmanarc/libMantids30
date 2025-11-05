#pragma once

#include <stdint.h>
#include <string>

namespace Mantids30::Network::Protocols::HTTP { namespace Headers {

class CacheControl
{
public:
    CacheControl() = default;

    std::string toString();
    void fromString(const std::string & str);

    /**
     * @brief Prevents caching of the response by any cache.
     */
    bool optionNoStore=true;

    /**
     * @brief Requires revalidation before using a cached response.
     */
    bool optionNoCache=false;

    /**
     * @brief Forces caches to revalidate stale responses with the origin server.
     */
    bool optionMustRevalidate=false;

    /**
     * @brief Restricts caching to the client only, disallowing intermediary caches.
     */
    bool optionPrivate=false;

    /**
     * @brief Allows the response to be cached by any cache, including shared ones.
     */
    bool optionPublic=false;

    /**
     * @brief Indicates the response will not change, allowing long-term caching.
     */
    bool optionImmutable=false;

    /**
     * @brief Prevents modification of the response by proxies or caches.
     */
    bool optionNoTransform=false;

    /**
     * @brief Requires shared caches to revalidate stale responses before serving.
     */
    bool optionProxyRevalidate=false;

    /**
     * @brief Maximum time (in seconds) the response can be cached before becoming stale.
     */
    uint32_t maxAge=0;

    /**
     * @brief Maximum time (in seconds) the response can be cached in shared caches.
     */
    uint32_t sMaxAge=0;

};

}}
