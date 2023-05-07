#include "accountsecretvalidator.h"
#include "Mantids29/Helpers/googleauthenticator.h"

#include "ds_auth_reason.h"

#include <Mantids29/Helpers/encoders.h>
#include <Mantids29/Helpers/crypto.h>

using namespace Mantids29::Authentication;


AccountSecretValidator::AccountSecretValidator()
{
    usedTokensCacheGC.setGarbageCollectorInterval( 5000 );
    usedTokensCacheGC.startGarbageCollector( cleanupExpiredTokens, this, "GC:TokensCache" );
}

void AccountSecretValidator::cleanupExpiredTokens()
{
    std::unique_lock<std::mutex> lock(cacheMutex);

    if (useTokenCache)
    {
        auto now = time(nullptr);
        while (!expirationQueue.empty() && (now - expirationQueue.begin()->first) >= 90)
        {
            usedTokensCache.erase(expirationQueue.begin()->second);
            expirationQueue.erase(expirationQueue.begin());
        }
    }
}

void AccountSecretValidator::cleanupExpiredTokens(void *asv)
{
    AccountSecretValidator * _asv = (AccountSecretValidator *)asv;
    _asv->cleanupExpiredTokens();
}

Reason AccountSecretValidator::validateStoredSecret(const std::string &accountName, const Secret &storedSecret, const std::string &passwordInput, const std::string &challengeSalt, Mode authMode)
{
    Reason r =REASON_NOT_IMPLEMENTED;
  //  bool saltedHash = false;
    std::string toCompare;

    switch (storedSecret.passwordFunction)
    {
    case FN_NOTFOUND:
        return REASON_INTERNAL_ERROR;
    case FN_PLAIN:
    {
        toCompare = passwordInput;
    } break;
    case FN_SHA256:
    {
        toCompare = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case FN_SHA512:
    {
        toCompare = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case FN_SSHA256:
    {
        toCompare = Helpers::Crypto::calcSSHA256(passwordInput, storedSecret.ssalt);
       // saltedHash = true;
    } break;
    case FN_SSHA512:
    {
        toCompare = Helpers::Crypto::calcSSHA512(passwordInput, storedSecret.ssalt);
        //saltedHash = true;
    } break;
    case FN_GAUTHTIME:
        r = validateGAuth(accountName,storedSecret.hash,passwordInput); // GAuth Time Based Token comparisson (seed,token)
        goto skipAuthMode;
    }

    switch (authMode)
    {
    case MODE_PLAIN:
        r = storedSecret.hash==toCompare? REASON_AUTHENTICATED:REASON_BAD_PASSWORD; // 1-1 comparisson
        break;
    case MODE_CHALLENGE:
        r = validateChallenge(storedSecret.hash, passwordInput, challengeSalt);
        break;
    }

skipAuthMode:;

    if (storedSecret.isExpired() && r==REASON_AUTHENTICATED)
        r = REASON_EXPIRED_PASSWORD;

    return r;
}

Reason AccountSecretValidator::validateChallenge(const std::string &passwordFromDB, const std::string &challengeInput, const std::string &challengeSalt)
{
    return challengeInput == Helpers::Crypto::calcSHA256(passwordFromDB + challengeSalt) ?
                 REASON_AUTHENTICATED:REASON_BAD_PASSWORD;
}

Reason AccountSecretValidator::validateGAuth(const std::string &accountName, const std::string &seed, const std::string &tokenInput)
{
    // Use the mutex to synchronize access to the cache and expirationQueue
    std::unique_lock<std::mutex> lock(cacheMutex);
    TokenCacheKey accountTokenKey = {accountName, tokenInput};

    // Check if the token is already in the cache and within the time limit

    if (useTokenCache)
    {
        auto cacheEntry = usedTokensCache.find(accountTokenKey);
        if (cacheEntry != usedTokensCache.end())
        {
            auto tokenTimestamp = cacheEntry->second;
            auto now = time(nullptr);
            auto elapsedSeconds = now - tokenTimestamp;

            if (elapsedSeconds < 90)
            {
                // Token Already Used.
                return REASON_BAD_PASSWORD;
            }
        }
    }

    // Verify the token and update the cache and expirationQueue if successful
    if (Helpers::TOTP::GoogleAuthenticator::verifyToken(seed, tokenInput))
    {
        // Add token to cache:
        if (useTokenCache)
        {
            auto now = time(nullptr);
            usedTokensCache[accountTokenKey] = now;
            expirationQueue.insert({now, accountTokenKey});
        }
        return REASON_AUTHENTICATED;
    }
    else
    {
        return REASON_BAD_PASSWORD;
    }
}

bool AccountSecretValidator::getUseTokenCache()
{
    std::unique_lock<std::mutex> lock(cacheMutex);
    return useTokenCache;
}

void AccountSecretValidator::setUseTokenCache(bool newUseTokenCache)
{
    std::unique_lock<std::mutex> lock(cacheMutex);
    useTokenCache = newUseTokenCache;
    if (!useTokenCache)
    {
        usedTokensCache.clear();
        expirationQueue.clear();
    }
}
