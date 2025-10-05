#include "jwt.h"
#include "Mantids30/Memory/b_chunks.h"
#include "Mantids30/Memory/streamablejson.h"
#include "Mantids30/Protocol_HTTP/httpv1_base.h"

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/file.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Program_Logs/loglevels.h>

#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Protocol_HTTP/httpv1_client.h>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <fcntl.h>
#include <ostream>
#include <sys/stat.h>

#include <fstream>
#include <memory>

using namespace Mantids30;
using namespace Mantids30::Program;
using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Program::Config;

// ==================== MAIN FACTORY FUNCTIONS ====================

struct APISyncParameters
{
    void loadFromInfoTree(const boost::property_tree::ptree &ptr)
    {
        apiSyncHost = ptr.get<std::string>("APISyncHost", "");

        if (apiSyncHost.empty())
        {
            return;
        }

        useTLS = ptr.get<bool>("UseTLS", true);
        apiSyncPort = ptr.get<uint16_t>("APISyncPort", 7081);

        if (auto tlsHeaders = ptr.get_child_optional("TLS"))
        {
            checkTLSPeer = tlsHeaders->get<bool>("CheckTLSPeer", true);
            usePrivateCA = tlsHeaders->get<bool>("UsePrivateCA", false);
            privateCAPath = ptr.get<std::string>("PrivateCAPath", "");
        }
    }

    std::string apiSyncHost;
    uint16_t apiSyncPort = 7081;
    bool checkTLSPeer = true;
    bool usePrivateCA = false;
    bool useTLS = true;
    std::string privateCAPath = "";
};
json loadAPI(Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &functionName, const std::string &appName, const std::string &apiKey)
{
    std::shared_ptr<Socket_Stream> connection;

    log->log0(__func__, Logs::LEVEL_INFO, "Requesting API Synchronization using '%s' for app '%s'.", functionName.c_str(), appName.c_str());

    if (proxyParameters->useTLS)
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Using TLS connection.");

        auto socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "Enabling certificate validation.");
            socket->setCertValidation(Socket_TLS::CERT_X509_VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                log->log0(__func__, Logs::LEVEL_DEBUG, "Using private CA from path: '%s'.", proxyParameters->privateCAPath.c_str());
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "Skipping certificate validation.");
            socket->setCertValidation(Socket_TLS::CERT_X509_NOVALIDATE);
        }

        connection = socket;
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_WARN, "Using plain TCP connection.");
        auto socket = std::make_shared<Socket_TCP>();
        connection = socket;
    }

    // Make the connection
    if (connection->connectTo(proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort))
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Connected to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);

        HTTP::HTTPv1_Client client(connection);

        auto strJSONRequest = std::make_shared<Memory::Streams::StreamableJSON>();

        (*strJSONRequest->getValue())["APIKEY"] = apiKey;
        // Set POST as request method
        client.clientRequest.requestLine.setRequestMethod("POST");
        client.clientRequest.requestLine.setRequestURI("/api/v1/" + functionName);
        client.clientRequest.getVars(HTTP::VARS_GET)->addVar("APP", std::make_shared<Memory::Containers::B_Chunks>(appName));
        client.clientRequest.content.setStreamableObj(strJSONRequest);
        client.clientRequest.headers.add("Content-Type","application/json");

        auto strJSONResponse = std::make_shared<Memory::Streams::StreamableJSON>();
        client.serverResponse.setDataStreamer(strJSONResponse);

        // Make the petition...
        Mantids30::Memory::Streams::Parser::ErrorMSG msg;
        client.parseObject(&msg);

        if (msg == Mantids30::Memory::Streams::Parser::PARSING_SUCCEED)
        {
            if (client.serverResponse.status.getCode() != HTTP::Status::S_200_OK)
            {
                log->log0(__func__, Logs::LEVEL_ERR, "Failed to retrieve Response. Error code: %d. = %s", static_cast<int>(client.serverResponse.status.getCode()), strJSONResponse->getValue()->toStyledString().c_str());
            }
            else
            {
                log->log0(__func__, Logs::LEVEL_DEBUG, "API request to %s successful.", functionName.c_str());
                return *strJSONResponse->getValue();
            }
        }
        else
        {
            log->log0(__func__, Logs::LEVEL_ERR, "Failed to parse API response. Error code: %d.", static_cast<int>(msg));
        }
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to connect to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);
    }

    return {};
}

std::optional<Json::Value> getApplicationJWTConfig(Logs::AppLog *log, APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const auto itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APP' variable in JWT configuration request.");
        return std::nullopt;
    }

    const auto itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APIKEY' variable in JWT configuration request.");
        return std::nullopt;
    }

    json response = loadAPI(log, proxyParameters, "getApplicationJWTConfig", itAppName->second, itApiKey->second);
    if (response.isNull())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to get JWT configuration from API.");
        return std::nullopt;
    }

    return response;
}

std::optional<std::string> getApplicationJWTSigningKey(Logs::AppLog *log, APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const auto itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APP' variable in JWT signing key request.");
        return std::nullopt;
    }

    const auto itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APIKEY' variable in JWT signing key request.");
        return std::nullopt;
    }

    json response = loadAPI(log, proxyParameters, "getApplicationJWTSigningKey", itAppName->second, itApiKey->second);
    if (response.isNull() || !response.isString())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to get JWT signing key from API.");
        return std::nullopt;
    }

    return response.asString();
}

std::optional<std::string> getApplicationJWTValidationKey(Logs::AppLog *log, APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const auto itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APP' variable in JWT validation key request.");
        return std::nullopt;
    }

    const auto itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Missing 'APIKEY' variable in JWT validation key request.");
        return std::nullopt;
    }

    json response = loadAPI(log, proxyParameters, "getApplicationJWTValidationKey", itAppName->second, itApiKey->second);
    if (response.isNull() || !response.isString())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to get JWT validation key from API.");
        return std::nullopt;
    }

    return response.asString();
}

std::shared_ptr<DataFormat::JWT> JWT::createJWTSigner(Logs::AppLog *log, const boost::property_tree::ptree &ptr, const std::string &configClassName, const std::map<std::string, std::string> &vars)
{
    APISyncParameters apiSyncParameters;
    if (auto x = ptr.get_child_optional(configClassName))
    {
        apiSyncParameters.loadFromInfoTree(*x);
    }

    std::string algorithmName;
    bool createIfNotPresent = false;

    if (apiSyncParameters.apiSyncHost.empty())
    {
        // Read configuration
        algorithmName = ptr.get<std::string>(configClassName + ".Algorithm", "HS256");
        createIfNotPresent = ptr.get<bool>(configClassName + ".CreateIfNotPresent", true);
    }
    else
    {
        auto jwtconfig = getApplicationJWTConfig(log, &apiSyncParameters, vars);
        if (jwtconfig)
        {
            algorithmName = JSON_ASSTRING((*jwtconfig), "tokenType", "");
        }
        else
            return nullptr;
    }

    // Validate algorithm
    if (!validateAlgorithm(log, algorithmName))
    {
        return nullptr;
    }

    DataFormat::JWT::AlgorithmDetails algorithmDetails(algorithmName.c_str());

    if (algorithmDetails.isUsingHMAC)
    {
        // HMAC-based signing
        std::string hmacFilePath;
        std::string hmacSecret;

        if (apiSyncParameters.apiSyncHost.empty())
        {
            hmacFilePath = ptr.get<std::string>(configClassName + ".HMACSecretFile", "jwt/jwt_secret.key");
            hmacSecret = loadHMACSecret(log, hmacFilePath, createIfNotPresent);
        }
        else
        {
            auto key = getApplicationJWTSigningKey(log, &apiSyncParameters, vars);
            if (key)
            {
                hmacSecret = (*key);
            }
            else
                return nullptr;
        }

        return createHMACJWT(log, algorithmDetails, hmacSecret, "signing");
    }
    else
    {
        // RSA-based signing
        std::string privateKeyFilePath;
        std::string publicKeyFilePath;
        uint16_t createRSASize;
        std::string privateKey;

        if (apiSyncParameters.apiSyncHost.empty())
        {
            privateKeyFilePath = ptr.get<std::string>(configClassName + ".PrivateKeyFile", "jwt.key");
            publicKeyFilePath = ptr.get<std::string>(configClassName + ".PublicKeyFile", "jwt.pub");
            createRSASize = ptr.get<uint16_t>(configClassName + ".CreateRSASize", 4096);
            privateKey = loadRSAPrivateKey(log, privateKeyFilePath, publicKeyFilePath, createIfNotPresent, createRSASize);
        }
        else
        {
            auto key = getApplicationJWTSigningKey(log, &apiSyncParameters, vars);
            if (key)
            {
                privateKey = (*key);
            }
            else
                return nullptr;
        }

        if (privateKey.empty())
        {
            return nullptr;
        }

        auto jwtSigner = std::make_shared<DataFormat::JWT>(algorithmDetails.algorithm);
        jwtSigner->setPrivateSecret(privateKey);
        return jwtSigner;
    }
}

std::shared_ptr<DataFormat::JWT> Mantids30::Program::Config::JWT::JWT::createJWTValidator(Logs::AppLog *log, const boost::property_tree::ptree &ptr, const std::string &configClassName,
                                                                                          const std::map<std::string, std::string> &vars)
{
    APISyncParameters apiSyncParameters;
    if (auto x = ptr.get_child_optional(configClassName))
    {
        apiSyncParameters.loadFromInfoTree(*x);
    }

    std::string algorithmName;
    bool createIfNotPresent = false;

    if (apiSyncParameters.apiSyncHost.empty())
    {
        // Read configuration
        algorithmName = ptr.get<std::string>(configClassName + ".Algorithm", "HS256");
        createIfNotPresent = ptr.get<bool>(configClassName + ".CreateIfNotPresent", true);
    }
    else
    {
        auto jwtconfig = getApplicationJWTConfig(log, &apiSyncParameters, vars);
        if (jwtconfig)
        {
            algorithmName = JSON_ASSTRING((*jwtconfig), "tokenType", "");
        }
        else
            return nullptr;
    }

    // Validate algorithm
    if (!validateAlgorithm(log, algorithmName))
    {
        return nullptr;
    }

    DataFormat::JWT::AlgorithmDetails algorithmDetails(algorithmName.c_str());

    if (algorithmDetails.isUsingHMAC)
    {
        // HMAC-based validation
        std::string hmacFilePath;
        std::string hmacSecret;
        if (apiSyncParameters.apiSyncHost.empty())
        {
            hmacFilePath = ptr.get<std::string>(configClassName + ".HMACSecretFile", "jwt/jwt_secret.key");
            hmacSecret = loadHMACSecret(log, hmacFilePath, createIfNotPresent);
        }
        else
        {
            auto key = getApplicationJWTValidationKey(log, &apiSyncParameters, vars);
            if (key)
            {
                hmacSecret = (*key);
            }
            else
                return nullptr;
        }
        return createHMACJWT(log, algorithmDetails, hmacSecret, "validation");
    }
    else
    {
        // RSA-based validation
        std::string privateKeyFilePath;
        std::string publicKeyFilePath;
        uint16_t createRSASize;
        std::string publicKey;

        if (apiSyncParameters.apiSyncHost.empty())
        {
            privateKeyFilePath = ptr.get<std::string>(configClassName + ".PrivateKeyFile", "jwt.key");
            publicKeyFilePath = ptr.get<std::string>(configClassName + ".PublicKeyFile", "jwt.pub");
            createRSASize = ptr.get<uint16_t>(configClassName + ".CreateRSASize", 4096);
            publicKey = loadRSAPublicKey(log, publicKeyFilePath, privateKeyFilePath, createIfNotPresent, createRSASize);
        }
        else
        {
            auto key = getApplicationJWTValidationKey(log, &apiSyncParameters, vars);
            if (key)
            {
                publicKey = (*key);
            }
            else
                return nullptr;
        }

        if (publicKey.empty())
        {
            return nullptr;
        }

        auto jwtValidator = std::make_shared<DataFormat::JWT>(algorithmDetails.algorithm);
        jwtValidator->setPublicSecret(publicKey);
        return jwtValidator;
    }
}

std::shared_ptr<DataFormat::JWT> Mantids30::Program::Config::JWT::JWT::createJWTValidator(Logs::AppLog *log, const std::string &algorithm, const std::string &key)
{
    // Validate algorithm
    if (!validateAlgorithm(log, algorithm))
    {
        return nullptr;
    }

    DataFormat::JWT::AlgorithmDetails algorithmDetails(algorithm.c_str());

    if (algorithmDetails.isUsingHMAC)
    {
        // HMAC-based validation
        std::string hmacSecret = key;
        removeTrailingNewline(hmacSecret);
        return createHMACJWT(log, algorithmDetails, hmacSecret, "validation");
    }
    else
    {
        // RSA-based validation
        if (key.empty())
        {
            log->log0(__func__, Logs::LEVEL_CRITICAL, "No JWT Public Validation Key Configured.");
            return nullptr;
        }

        auto jwtValidator = std::make_shared<DataFormat::JWT>(algorithmDetails.algorithm);
        jwtValidator->setPublicSecret(key);
        log->log0(__func__, Logs::LEVEL_INFO, "JWT Public Validation Key Loaded.");
        return jwtValidator;
    }
}

// ==================== HELPER FUNCTIONS ====================

bool JWT::validateAlgorithm(Logs::AppLog *log, const std::string &algorithmName)
{
    if (!DataFormat::JWT::isAlgorithmSupported(algorithmName))
    {
        log->log0(__func__, Logs::LEVEL_ERR, "JWT algorithm '%s' not supported.", algorithmName.c_str());
        return false;
    }
    return true;
}

void JWT::removeTrailingNewline(std::string &str)
{
    if (!str.empty() && str.back() == '\n')
    {
        str.pop_back();
    }
}

bool JWT::validateFilePermissions(Logs::AppLog *log, const std::string &filePath, const std::string &fileType)
{
    bool insecureFile;

    if (!Helpers::File::isSensitiveConfigPermissionInsecure(filePath, &insecureFile))
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to check permissions on %s file.", fileType.c_str());
        return false;
    }

    if (insecureFile)
    {
        log->log0(__func__, Logs::LEVEL_SECURITY_ALERT,
                  "Insecure %s file permissions detected. For security reasons, "
                  "it is crucial to change this key immediately and reboot the service.",
                  fileType.c_str());
        return false;
    }

    return true;
}

std::ifstream JWT::openFileWithCreation(const std::string &filePath, bool createIfNotPresent, std::function<bool()> createFunc)
{
    std::ifstream file(filePath.c_str());

    if (!file.is_open() && createIfNotPresent && createFunc())
    {
        file.open(filePath.c_str());
    }

    return file;
}

// ==================== HMAC FUNCTIONS ====================

std::string JWT::loadHMACSecret(Logs::AppLog *log, const std::string &hmacFilePath, bool createIfNotPresent)
{
    // Open file with optional creation
    auto hmacFile = openFileWithCreation(hmacFilePath, createIfNotPresent, [&]() { return createHMACSecret(log, hmacFilePath); });

    if (!hmacFile.is_open())
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to open HMAC secret file.");
        return "";
    }

    // Validate file permissions
    if (!validateFilePermissions(log, hmacFilePath, "HMAC secret"))
    {
        return "";
    }

    // Read secret
    std::string hmacSecret;
    std::getline(hmacFile, hmacSecret);
    removeTrailingNewline(hmacSecret);

    if (hmacSecret.empty())
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Empty JWT HMAC Key.");
        return "";
    }

    return hmacSecret;
}

std::shared_ptr<DataFormat::JWT> JWT::createHMACJWT(Logs::AppLog *log, const DataFormat::JWT::AlgorithmDetails &algorithmDetails, const std::string &hmacSecret, const std::string &purpose)
{
    if (hmacSecret.empty())
    {
        return nullptr;
    }

    auto jwt = std::make_shared<DataFormat::JWT>(algorithmDetails.algorithm);
    jwt->setSharedSecret(hmacSecret);

    log->log0(__func__, Logs::LEVEL_INFO, "JWT HMAC %s key successfully loaded with algorithm: '%s'.", purpose.c_str(), algorithmDetails.algorithm);

    return jwt;
}

bool JWT::createHMACSecret(Logs::AppLog *log, const std::string &filePath)
{
    bool r = false;
    int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // 0600 permissions
    if (fd != -1)
    {
        FILE *pkeyFile = fdopen(fd, "w");
        if (pkeyFile)
        {
            std::string rndstr = Helpers::Random::createRandomString(32);
            r = fwrite(rndstr.c_str(), 32, 1, pkeyFile) == 1;

            if (r)
            {
                log->log0(__func__, Logs::LEVEL_WARN, "Created JWT HMAC Secret File: %s", filePath.c_str());
            }
            else
            {
                log->log0(__func__, Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (3): %s", filePath.c_str());
            }

            fclose(pkeyFile);
        }
        else
        {
            log->log0(__func__, Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (2): %s", filePath.c_str());
            close(fd);
        }
    }
    else
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (1): %s", filePath.c_str());

    return r;
}

// ==================== RSA FUNCTIONS ====================

std::string JWT::loadRSAPrivateKey(Logs::AppLog *log, const std::string &privateKeyFilePath, const std::string &publicKeyFilePath, bool createIfNotPresent, uint16_t createRSASize)
{
    if (privateKeyFilePath.empty())
    {
        log->log0(__func__, Logs::LEVEL_INFO, "No JWT RSA Signing Key Configured.");
        return "";
    }

    // Try to open file with optional creation
    FILE *privateKeyFP = fopen(privateKeyFilePath.c_str(), "r");
    if (privateKeyFP == nullptr && createIfNotPresent && createRSASecret(log, privateKeyFilePath, publicKeyFilePath, createRSASize))
    {
        privateKeyFP = fopen(privateKeyFilePath.c_str(), "r");
    }

    if (privateKeyFP == nullptr)
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to read the JWT Private Key from file '%s'.", privateKeyFilePath.c_str());
        return "";
    }

    // Validate file permissions
    if (!validateFilePermissions(log, privateKeyFilePath, "JWT RSA Signing Key secret"))
    {
        fclose(privateKeyFP);
        return "";
    }

    // Load and convert key
    std::string fileContent;
    EVP_PKEY *evpPrivateKey = PEM_read_PrivateKey(privateKeyFP, nullptr, nullptr, nullptr);

    if (evpPrivateKey)
    {
        BIO *privateKeyBio = BIO_new(BIO_s_mem());
        PEM_write_bio_PrivateKey(privateKeyBio, evpPrivateKey, nullptr, nullptr, 0, nullptr, nullptr);

        char *privateKeyStr;
        long privateKeyLen = BIO_get_mem_data(privateKeyBio, &privateKeyStr);
        fileContent.assign(privateKeyStr, privateKeyLen);

        BIO_free(privateKeyBio);
        log->log0(__func__, Logs::LEVEL_INFO, "JWT RSA Signing Key Loaded.");
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to load the JWT Private Key from file '%s'.", privateKeyFilePath.c_str());
    }

    EVP_PKEY_free(evpPrivateKey);
    fclose(privateKeyFP);

    return fileContent;
}

std::string JWT::loadRSAPublicKey(Logs::AppLog *log, const std::string &publicKeyFilePath, const std::string &privateKeyFilePath, bool createIfNotPresent, uint16_t createRSASize)
{
    if (publicKeyFilePath.empty())
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "No JWT RSA Validation Key Configured.");
        return "";
    }

    // Try to open file with optional creation
    FILE *publicKeyFP = fopen(publicKeyFilePath.c_str(), "r");
    if (publicKeyFP == nullptr && createIfNotPresent && createRSASecret(log, privateKeyFilePath, publicKeyFilePath, createRSASize))
    {
        publicKeyFP = fopen(publicKeyFilePath.c_str(), "r");
    }

    if (publicKeyFP == nullptr)
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Failed to read the JWT Public Key from file '%s'.", publicKeyFilePath.c_str());
        return "";
    }

    // Load and convert key
    std::string fileContent;
    EVP_PKEY *evpPublicKey = PEM_read_PUBKEY(publicKeyFP, nullptr, nullptr, nullptr);

    if (evpPublicKey)
    {
        BIO *publicKeyBio = BIO_new(BIO_s_mem());
        PEM_write_bio_PUBKEY(publicKeyBio, evpPublicKey);

        char *publicKeyStr;
        long publicKeyLen = BIO_get_mem_data(publicKeyBio, &publicKeyStr);
        fileContent.assign(publicKeyStr, publicKeyLen);

        BIO_free(publicKeyBio);
        log->log0(__func__, Logs::LEVEL_INFO, "JWT RSA Validation Key Loaded.");
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Failed to load the JWT Public Key from file '%s'.", publicKeyFilePath.c_str());
    }

    EVP_PKEY_free(evpPublicKey);
    fclose(publicKeyFP);

    return fileContent;
}

bool JWT::createRSASecret(Logs::AppLog *log, const std::string &keyPath, const std::string &crtPath, uint16_t keySize)
{
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    bool success = false;

    if (access(keyPath.c_str(), F_OK) && access(crtPath.c_str(), F_OK))
    {
        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        if (ctx && EVP_PKEY_keygen_init(ctx) > 0 && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, keySize) > 0 && EVP_PKEY_keygen(ctx, &pkey) > 0)
        {
            // Save private key
            int fd = open(keyPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // 0600 permissions
            if (fd != -1)
            {
                FILE *pkeyFile = fdopen(fd, "w");
                if (pkeyFile)
                {
                    if (PEM_write_PrivateKey(pkeyFile, pkey, NULL, NULL, 0, NULL, NULL))
                    {
                        log->log0(__func__, Logs::LEVEL_WARN, "Created JWT X.509 RSA Private Key: %s", keyPath.c_str());

                        // Save public key
                        FILE *pubkeyFile = fopen(crtPath.c_str(), "wb");
                        if (pubkeyFile)
                        {
                            success = PEM_write_PUBKEY(pubkeyFile, pkey) > 0;
                            if (success)
                            {
                                log->log0(__func__, Logs::LEVEL_WARN, "Created JWT X.509 RSA Public Key: %s", crtPath.c_str());
                            }
                            else
                            {
                                log->log0(__func__, Logs::LEVEL_ERR, "Failed to write X.509 RSA Public Key (2): %s", crtPath.c_str());
                            }
                            fclose(pubkeyFile);
                        }
                        else
                        {
                            log->log0(__func__, Logs::LEVEL_ERR, "Failed to write X.509 RSA Public Key (1): %s", crtPath.c_str());
                        }
                    }
                    else
                    {
                        log->log0(__func__, Logs::LEVEL_ERR, "Failed to write X.509 RSA Private Key (2): %s", keyPath.c_str());
                    }
                    fclose(pkeyFile);
                }
                else
                {
                    log->log0(__func__, Logs::LEVEL_ERR, "Failed to write X.509 RSA Private Key (1): %s", keyPath.c_str());
                    close(fd);
                }
            }
            else
                log->log0(__func__, Logs::LEVEL_ERR, "Failed to generate X.509 RSA Keys (2)");
        }
        else
            log->log0(__func__, Logs::LEVEL_ERR, "Failed to generate X.509 RSA Keys (1)");
    }

    if (pkey)
        EVP_PKEY_free(pkey);
    if (ctx)
        EVP_PKEY_CTX_free(ctx);

    return success;
}
