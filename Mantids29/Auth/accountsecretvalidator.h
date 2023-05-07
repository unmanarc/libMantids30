#ifndef ACCOUNT_SECRET_VALIDATOR_H
#define ACCOUNT_SECRET_VALIDATOR_H

#include <Mantids29/Threads/garbagecollector.h>

#include <mutex>
#include <string>
#include <set>
#include <unordered_map>

#include "ds_auth_mode.h"
#include "ds_auth_reason.h"
#include "ds_auth_function.h"
#include "ds_auth_secret.h"

#include <Mantids29/Threads/mapitem.h>

namespace Mantids29 { namespace Authentication {

struct TokenCacheKey
{
    /*   bool operator<(const TokenCacheKey & x) const
        {
            if (x.accountName < accountName) return true;
            else if (x.accountName == accountName && x.token < token) return true;
            else return false;
        }*/
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
// Define the specialization of std::hash for TokenCacheKey inside the Mantids29::Authentication namespace
namespace std {
template <>
struct hash<Mantids29::Authentication::TokenCacheKey> {
    size_t operator()(const Mantids29::Authentication::TokenCacheKey& key) const {
        return key.hash();
    }
};
}

namespace Mantids29 { namespace Authentication {

struct ApplicationAttribute {
    bool operator<(const ApplicationAttribute & x) const
    {
        if (x.appName < appName) return true;
        else if (x.appName == appName && x.attribName < attribName) return true;
        else return false;
    }
    std::string appName,attribName;
};

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


class AccountSecretValidator : public Mantids29::Threads::Safe::MapItem
{
public:
    AccountSecretValidator();
    virtual ~AccountSecretValidator() = default;

    /**
     * @brief Returns the account confirmation token for a given account name.
     * @param accountName The name of the account to get the confirmation token for.
     * @return The confirmation token for the account.
     */
    virtual std::string getAccountConfirmationToken(const std::string& accountName) = 0;

    /**
     * @brief Returns the public data associated with an account's secret for a given account name and password index.
     * @param accountName The name of the account to get the secret public data for.
     * @param passwordIndex The password index to use for retrieving the account secret.
     * @return The public data associated with the account's secret.
     */
    virtual Secret_PublicData getAccountSecretPublicData(const std::string& accountName, uint32_t passwordIndex = 0) = 0;

    /**
     * @brief Authenticates a user's credentials for a given application, client, and account.
     * @param appName The name of the application that the user is authenticating for.
     * @param clientDetails Details about the incoming client connection.
     * @param accountName The name of the account to authenticate.
     * @param password The password to use for authentication.
     * @param passwordIndex The password index to use for authentication.
     * @param authMode The mode to use for authentication.
     * @param challengeSalt The salt to use for the authentication challenge.
     * @param accountPassIndexesUsedForLogin A map of password indexes that were used for login.
     * @return A reason indicating whether authentication was successful or not.
     */
    virtual Reason authenticate(const std::string& appName, const ClientDetails& clientDetails, const std::string& accountName, const std::string& password, uint32_t passwordIndex = 0, Mode authMode = MODE_PLAIN, const std::string& challengeSalt = "", std::map<uint32_t, std::string>* accountPassIndexesUsedForLogin = nullptr) = 0;

    /**
     * @brief Validates an account attribute for a given account name and application attribute.
     * @param accountName The name of the account to validate the attribute for.
     * @param applicationAttribute The application attribute to validate.
     * @return true if the account attribute is valid, false otherwise.
     */
    virtual bool validateAccountAttribute(const std::string& accountName, const ApplicationAttribute& applicationAttribute) = 0;

    // Cleanup function to remove expired google authenticator tokens
    void cleanupExpiredTokens();
    // Static function to be called from the garbage collector.
    static void cleanupExpiredTokens(void * asv);



    bool getUseTokenCache();
    void setUseTokenCache(bool newUseTokenCache);

protected:
    /**
     * @brief Validates a stored secret against an input password and challenge salt.
     * @param accountName any unique descriptor for the account (eg. UUID, UID, username)
     * @param storedSecret The stored secret to validate.
     * @param passwordInput The input password to use for validation.
     * @param challengeSalt The challenge salt to use for validation.
     * @param authMode The mode to use for authentication.
     * @return A reason indicating whether the stored secret was valid or not.
     */
    Reason validateStoredSecret(const std::string &accountName, const Secret& storedSecret, const std::string& passwordInput, const std::string& challengeSalt, Mode authMode);

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




#endif // ACCOUNT_SECRET_VALIDATOR_H
