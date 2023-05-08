#pragma once

#include <ctime>
#include <string>
#include <set>
#include <thread>
#include <boost/thread/shared_mutex.hpp>
#include <condition_variable>
#include <json/json.h>
#include <queue>
#include <unordered_map>

namespace Mantids29 { namespace DataFormat {

/**
 * @brief Class for creating and verifying JSON Web Tokens (JWT)
 *
 */
class JWT {
public:
    /**
     * @brief Enumeration for supported algorithms
     *
     */
    enum Algorithm {
        HS256, /**< HMAC with SHA-256 */
        HS384, /**< HMAC with SHA-384 */
        HS512, /**< HMAC with SHA-512 */
        RS256, /**< RSA with SHA-256 */
        RS384, /**< RSA with SHA-384 */
        RS512  /**< RSA with SHA-512 */
    };

    /**
     * @brief Struct that contains information about a JWT algorithm.
     *
     * This struct is used to hold the details of a JWT algorithm, including
     * its name, OpenSSL NID, and whether it uses HMAC or RSA encryption.
     */
    struct AlgorithmDetails {
        /**
         * @brief Construct a new AlgorithmDetails object from an algorithm enum.
         *
         * @param algorithm The algorithm enum value to construct from.
         */
        AlgorithmDetails( Algorithm algorithm );

        /**
         * @brief Construct a new AlgorithmDetails object from an algorithm name.
         *
         * @param algorithm The name of the algorithm to construct from.
         */
        AlgorithmDetails( const char * algorithm );

        int m_nid; ///< The OpenSSL NID of the algorithm.
        bool m_usingHMAC; ///< True if the algorithm uses HMAC encryption, false otherwise.
        bool m_usingRSA; ///< True if the algorithm uses RSA encryption, false otherwise.
        char m_algorithmStr[16]; ///< The name of the algorithm, as a null-terminated string.
        Algorithm m_algorithm; ///< The algorithm enum value.
    };

    class Token {
    public:

        Token()
        {
        }

        Token(const std::string &payload);

        void setIssuer(const std::string &issuer);

        void setSubject(const std::string &subject);

        void setAudience(const std::string &audience);

        void setExpirationTime(std::time_t exp);

        void setNotBefore(std::time_t nbf);

        void setIssuedAt(std::time_t iat);

        void setJwtId(const std::string &jti);

        void addClaim(const std::string &name, const Json::Value &value);

        std::string exportPayload() const;

        bool decodePayload(const std::string &payload);

        // Getter functions for standard claims
        std::string getIssuer() const;

        std::string getSubject() const;

        std::string getAudience() const;

        std::time_t getExpirationTime() const;

        std::time_t getNotBefore() const;

        std::time_t getIssuedAt() const;

        std::string getJwtId() const;

        std::set<std::string> getAllAttributes();

        void addAttribute(const std::string & attributeName);

        bool hasAttribute(const std::string &attributeName) const;

        std::map<std::string,Json::Value> getAllClaims();

        // Getter function for custom claims
        Json::Value getClaim(const std::string &name) const;

        // Function to check if claim exist...
        bool hasClaim(const std::string &name) const;

        // Function to validate if the token is still valid and verified...
        bool isValid() const;

        // Internal function to set the verified status...
        void setVerified(bool newVerified);

        Json::Value *getClaimsPTR();

        bool isRevoked() const;

        void setRevoked(bool newRevoked);

    private:
        Json::Value m_claims;
        bool m_verified = false;
        bool m_revoked = false;
    };

    class Cache {
    public:
        // Cache functions:
        bool checkToken(const std::string& token);
        void add(const std::string& token);
        void evictCache();

        void setCacheMaxByteCount(std::size_t maxByteCount);
        std::size_t getCacheMaxByteCount();
        void clear();

        bool isEnabled();
        void setEnabled(bool newEnabled);

    private:
        std::size_t m_cacheMaxByteCount = 1*1024*1024;
        std::size_t m_cacheCurrentByteCount = 0;

        bool m_enabled;

        std::unordered_map<std::string, bool> m_tokenCache;
        std::queue<std::string> m_cacheQueue;
        boost::shared_mutex m_cachedTokensMutex;
    };

    class Revocation {
    public:
        Revocation();
        ~Revocation();

        // Revokation functions...
        void addToRevocationList(const std::string& signature, std::time_t expirationTime);
        bool isSignatureRevoked(const std::string& signature);
        void removeExpiredTokensFromRevocationList();
        void clear();

        std::chrono::seconds garbageCollectorInterval() const;
        void setGarbageCollectorInterval(const std::chrono::seconds &newGarbageCollectorInterval);

    private:
        void garbageCollector();

        std::multimap<std::time_t, std::string> m_expirationSignatures;
        std::set<std::string> m_revokedTokens;
        boost::shared_mutex m_revokedTokensMutex;

        std::thread m_garbageCollectorThread;
        std::atomic_bool m_stopGarbageCollector;
        std::condition_variable m_garbageCollectorCondition;
        std::mutex m_garbageCollectorMutex;
        std::chrono::seconds m_garbageCollectorInterval;
    };

    /**
     * @brief Struct for holding the result and signature of createSignature()
     *
     */
    struct RAWSignature {
        RAWSignature()
        {
            m_digest = nullptr;
            m_digestSize = 0;
        }
        ~RAWSignature()
        {
            if (m_digest) delete [] m_digest;
            m_digestSize = 0;
            m_digest = nullptr;
        }

        /**
         * @brief Enumeration for possible results of createSignature() or createHMACSignature() or createRSASignature
         *
         */
        enum Result {
            SIG_OK = 0,                   /**< Signature created successfully */
            SIG_EMPTY_KEY = -10,          /**< The key used to sign is empty */
            SIG_ERROR_CREATING_SIGNATURE = -1, /**< Error creating the signature */
            SIG_ERROR_CREATING_RSA_OBJECT = -2, /**< Error creating the RSA object */
            SIG_ERROR_READING_KEY = -3    /**< Error reading the key */
        };

        Result m_result; /**< Result */
        unsigned char * m_digest; /**< RAW Signature created by createSignature() */
        unsigned int m_digestSize; /**< RAW Signature size created by createSignature() */
    };

    /**
     * @brief Constructor for JWT object
     *
     * @param algorithm Algorithm to use for JWT
     */
    JWT(const Algorithm& algorithm) : m_algorithm(algorithm)
    {
    }

    /**
     * @brief Check if a given algorithm is supported
     *
     * @param algorithm Algorithm to check
     * @return true if algorithm is supported
     * @return false if algorithm is not supported
     */
    static bool isAlgorithmSupported(const std::string & algorithm);

    /**
     * @brief Create a JWT token from a JSON payload
     *
     * @param payload JSON payload to sign
     * @return std::string JWT token
     */
    std::string sign(const Json::Value& payload);

    /**
     * @brief signFromToken Sign from a constructed token
     * @param token input token
     * @param updateDefaultTimeValues the constructed token will be updated on the IAT, NBF and EXP claims...
     * @return JWT signed string (including the header, token and signature)
     */
    std::string signFromToken(Token& token, bool updateDefaultTimeValues = true);

    /**
     * @brief Verify the signature of a JWT token
     *
     * @param token JWT token to verify
     * @return true if signature is valid
     * @return false if signature is invalid
     */
    bool verify(const std::string& fullSignedToken, Token *tokenPayloadOutput = nullptr);

    /**
     * @brief verifyAndDecodeTokenPayload Return an object with the verified token (or empty token if not verified)
     * @param fullSignedToken
     * @return verified token (or empty object)
     */
    Token verifyAndDecodeTokenPayload(const std::string& fullSignedToken);

    /**
     * @brief Set the shared secret for HMAC algorithms
     *
     * @param newSharedSecret Shared secret to set
     */
    void setSharedSecret(const std::string &newSharedSecret);

    /**
     * @brief Set the public key for RSA algorithms
     *
     * @param newPublicSecret Public key to set
     */
    void setPublicSecret(const std::string &newPublicSecret);

    /**
     * @brief Set the private key for RSA algorithms
     *
     * @param newPrivateSecret Private key to set
     */
    void setPrivateSecret(const std::string &newPrivateSecret);

    /**
     * @brief Sets the maximum default expiration time for new tokens items in seconds
     *
     * @param newMaxExpirationTimeInSeconds The new maximum expiration time in seconds
     */
    void setDefaultMaxExpirationTimeInSeconds(std::time_t newMaxExpirationTimeInSeconds);

    /**
     * @brief Sets the maximum default time before a new token item will be become invalid
     *
     * @param newMaxTimeBeforeInSeconds The new maximum time before a cached item should be refreshed in seconds
     */
    void setDefaultMaxTimeBeforeInSeconds(std::time_t newMaxTimeBeforeInSeconds);

    /**
     * @brief Returns the maximum default expiration time for new tokens items in seconds
     *
     * @return The maximum expiration time in seconds
     */
    std::time_t defaultExpirationTimeInSeconds() const;

    /**
     * @brief Returns the maximum default time before a token item will be become invalid
     *
     * @return The maximum time before a cached item should be refreshed in seconds
     */
    std::time_t defaultMaxTimeBeforeInSeconds() const;


    Cache m_cache;
    Revocation m_revocation;

private:

    /**
     * @brief Check if a given algorithm is an HMAC algorithm
     *
     * @param algorithm Algorithm to check
     * @return true if algorithm is an HMAC algorithm
     * @return false if algorithm is not an HMAC algorithm
     */
    bool isHMACAlgorithm(const std::string & algorithm);

    /**
     * @brief Check if a given algorithm is an RSA algorithm
     *
     * @param algorithm Algorithm to check
     * @return true if algorithm is an RSA algorithm
     * @return false if algorithm is not an RSA algorithm
     */
    bool isRSAAlgorithm(const std::string & algorithm);
    /**
     * @brief Creates a header string.
     *
     * @return std::string The generated header string.
     */
    std::string createHeader();

    /**
     * @brief Creates a signature using the specified algorithm.
     *
     * @param data The input data to be signed.
     * @return RAWSignature A RAWSignature object containing the generated signature.
     */
    std::shared_ptr<RAWSignature> createSignature(const std::string& data);

    /**
     * @brief Generates an HMAC signature using the specified hash algorithm.
     *
     * @param hashType The integer representation of the hash algorithm to use.
     * @param data The input data to be signed.
     * @param digestOut A pointer to the output buffer to store the generated signature.
     * @param digestOutLength A pointer to the length of the generated signature.
     * @return RAWSignature::Result The result of the signature generation operation.
     */
    std::shared_ptr<RAWSignature> createHMACSignature(int hashType, const std::string& data);

    /**
     * @brief Generates an RSA signature using the specified hash algorithm.
     *
     * @param hashType The integer representation of the hash algorithm to use.
     * @param data The input data to be signed.
     * @param digestOut A pointer to the output buffer to store the generated signature.
     * @param digestOutLength A pointer to the length of the generated signature.
     * @return RAWSignature::Result The result of the signature generation operation.
     */
    std::shared_ptr<RAWSignature> createRSASignature(int hashType, const std::string& data);

    /**
     * @brief Validates an RSA signature using the specified hash algorithm.
     *
     * @param hashType The integer representation of the hash algorithm to use.
     * @param data The input data that was signed.
     * @param signature A pointer to the signature buffer to be validated.
     * @param signatureLength The length of the signature buffer.
     * @return int 0 if the signature is valid, -1 otherwise.
     */
    int validateRSASignature(int hashType, const std::string& data, const char* signature, unsigned int signatureLength);

    /**
     * @brief Returns the integer representation of the currently used hash algorithm.
     *
     * @return int The integer representation of the hash algorithm.
     */
    int getHashTypeNumber();

    /**
     * @brief The cryptographic algorithm used for signature generation.
     *
     */
    Algorithm m_algorithm;

    /**
     * @brief The shared secret used for cryptographic operations.
     *
     */
    std::string m_sharedSecret;

    /**
     * @brief The public secret used for cryptographic operations.
     *
     */
    std::string m_publicSecret;

    /**
     * @brief The private secret used for cryptographic operations.
     *
     */
    std::string m_privateSecret;

    /**
     * @brief Max age of the token
     */
    std::time_t m_defaultExpirationTimeInSeconds = 300;

    /**
     * @brief Max Time In Seconds in the past for non-synchronized servers/clients validating the token
     */
    std::time_t m_defaultMaxTimeBeforeInSeconds = 60;

};

}}

