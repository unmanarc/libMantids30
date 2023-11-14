#pragma once

#include <Mantids30/Threads/garbagecollector.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "ds_authentication.h"

namespace Mantids30 { namespace Auth {

struct TokenCacheKey
{
    bool operator==(const TokenCacheKey & x) const
    {
        return (x.accountName == accountName && x.token == token);
    }
    size_t hash() const {
        return std::hash<std::string>{}(accountName) ^ std::hash<std::string>{}(token);
    }

    std::string accountName;
    std::string token;
};
}}
// Define the specialization of std::hash for TokenCacheKey inside the Mantids30::Auth namespace
namespace std {
template <>
struct hash<Mantids30::Auth::TokenCacheKey> {
    size_t operator()(const Mantids30::Auth::TokenCacheKey& key) const {
        return key.hash();
    }
};
}

namespace Mantids30 { namespace Auth {

struct ApplicationPermission {
    bool operator<(const ApplicationPermission & x) const
    {
        if (x.appName < appName) return true;
        else if (x.appName == appName && x.permissionId < permissionId) return true;
        else return false;
    }
    std::string appName,permissionId;
};
/*
struct ApplicationActivity {
    bool operator<(const ApplicationActivity & x) const
    {
        if (x.appName < appName) return true;
        else if (x.appName == appName && x.activityId < activityId) return true;
        else return false;
    }
    std::string appName,activityId;
};
*/

/**
 * @brief The ClientDetails struct contains details about an incoming client connection.
 */
struct ClientDetails {
    /**
     * @brief The IP address of the incoming client connection.
     */
    std::string ipAddress;

    /**
     * @brief Additional data associated with the incoming client connection.
     */
    std::string extraData;

    /**
     * @brief The common name associated with the client's TLS certificate.
     */
    std::string tlsCommonName;

    /**
     * @brief The user agent string associated with the incoming client connection.
     */
    std::string userAgent;
};


class CredentialValidator
{
public:
    CredentialValidator();
    virtual ~CredentialValidator() = default;

    /**
     * @brief Returns the account confirmation token for a given account name.
     * @param accountName The name of the account to get the confirmation token for.
     * @return The confirmation token for the account.
     */
    virtual std::string getAccountConfirmationToken(const std::string& accountName) = 0;

    /**
     * @brief Returns the public data associated with an account's credential for a given account name and auth slot id.
     * @param accountName The name of the account to get the credential public data for.
     * @param slotId The password index to use for retrieving the account credential.
     * @return The public data associated with the account's credential.
     */
    virtual Credential getAccountCredentialPublicData(const std::string& accountName, uint32_t slotId) = 0;

    /**
     * @brief Authenticates a user's credentials for a given application, client, and account.
     * @param appName The name of the application that the user is authenticating for.
     * @param clientDetails Details about the incoming client connection.
     * @param accountName The name of the account to authenticate.
     * @param password The password to use for authentication.
     * @param slotId The password index to use for authentication.
     * @param authMode The mode to use for authentication.
     * @param challengeSalt The salt to use for the authentication challenge.
     * @param getAccountAuthenticationSlotsUsedForLogin A map of password indexes that were used for login.
     * @return A reason indicating whether authentication was successful or not.
     */
    virtual Reason authenticateCredential(const ClientDetails& clientDetails, const std::string& accountName, const std::string& password, uint32_t slotId, Mode authMode = MODE_PLAIN, const std::string& challengeSalt = "") = 0;

    /**
     * @brief Validates an account permissions for a given account name and application permission.
     * @param accountName The name of the account to validate the permission for.
     * @param applicationPermission The application permission to validate.
     * @return true if the account permission is valid, false otherwise.
     */
    virtual bool validateAccountApplicationPermission(const std::string& accountName, const ApplicationPermission& applicationPermission) = 0;

    // Cleanup function to remove expired google authenticator tokens
    void cleanupExpiredTokens();
    // Static function to be called from the garbage collector.
    static void cleanupExpiredTokens(void * asv);



    bool getUseTokenCache();
    void setUseTokenCache(bool newUseTokenCache);

protected:
    /**
     * @brief Validates a stored credential against an input password and challenge salt.
     * @param accountName any unique descriptor for the account (eg. UUID, UID, username)
     * @param storedCredential The stored credential to validate.
     * @param passwordInput The input password to use for validation.
     * @param challengeSalt The challenge salt to use for validation.
     * @param authMode The mode to use for authentication.
     * @return A reason indicating whether the stored credential was valid or not.
     */
    Reason validateStoredCredential(const std::string &accountName, const Credential& storedCredential, const std::string& passwordInput, const std::string& challengeSalt, Mode authMode);

private:
    /**
     * @brief validateChallenge Validate the Challenge (SHA256(Pass+Salt))
     * @param passwordFromDB Incomming password from DB
     * @param challengeInput Challenge Input SHA256(Pass+Salt)
     * @param challengeSalt Challenge Salt (Random Value generated by your app, take the security considerations)
     * @return Authentication Response Reason (authenticated or bad password)
     */
    Reason validateChallenge(const std::string & passwordFromDB, const std::string & challengeInput, const std::string &challengeSalt);

    /**
    * @brief Validates a Google Authenticator token for a given account and seed.
    *
    * This function validates the Google Authenticator token by comparing it with the expected token
    * generated using the account name and seed. If the token matches, the function returns true; otherwise, it returns false.
    *
    * @param accountName The account name associated with the token.
    * @param seed        The secret seed used for generating the token.
    * @param tokenInput  The Google Authenticator token to be validated.
    *
    * @return True if the token is valid, false otherwise.
    */
    Reason validateGAuth(const std::string &accountName,const std::string & seed, const std::string & tokenInput);

    // Add a cache to store used tokens with timestamps
    std::unordered_map<TokenCacheKey, time_t> usedTokensCache;

    // Ordered data structure to efficiently remove expired tokens
    std::multimap<time_t, TokenCacheKey> expirationQueue;

    // Mutex for synchronizing access to the cache and expirationQueue
    std::mutex cacheMutex;

    // Garbage collector for authentication tokens cache...
    Threads::GarbageCollector usedTokensCacheGC;

    bool useTokenCache = true;
};

}}




