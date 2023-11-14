#include "jwt.h"

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/file.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <fstream>
#include <memory>

using namespace Mantids30;

std::shared_ptr<DataFormat::JWT> ConfigBuilder::JWT::createJWTSigner(Program::Logs::AppLog *log,
                                                              boost::property_tree::ptree *ptr,
                                                              const std::string &configClassName)
{
    bool insecureFile;
    std::shared_ptr<DataFormat::JWT> jwtNull;

    std::string algorithmName = ptr->get<std::string>(configClassName + ".Algorithm", "HS256");
    bool createIfNotPresent = ptr->get<bool>(configClassName + ".CreateIfNotPresent", true);

    if (!DataFormat::JWT::isAlgorithmSupported(algorithmName))
    {
        log->log0(__func__, Program::Logs::LEVEL_ERR, "JWT algorithm '%s' not supported.", algorithmName.c_str());
        return jwtNull;
    }

    DataFormat::JWT::AlgorithmDetails algorithmDetails(algorithmName.c_str());

    auto jwtSigner = std::make_shared<DataFormat::JWT>(algorithmDetails.m_algorithm);

    if (algorithmDetails.m_usingHMAC)
    {
        auto hmacFilePath = ptr->get<std::string>(configClassName + ".HMACSecret", "jwt_secret.key");

        // HMACSecret is a file, read the hmacSecret variable from file to file and
        // report error if failed to read or if permissions are not secure.
        std::ifstream hmacFile(hmacFilePath.c_str());

        if (!hmacFile.is_open() && createIfNotPresent && createHMACSecret(log,hmacFilePath))
        {
            hmacFile.open(hmacFilePath.c_str());
        }

        if (!hmacFile.is_open())
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to open HMAC secret file.");
            return jwtNull;
        }

        if (!Helpers::File::isSensitiveConfigPermissionInsecure(hmacFilePath, &insecureFile))
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to open permissions on HMAC secret file.");
            return jwtNull;
        }

        if (insecureFile)
        {
            log->log0(__func__,
                      Program::Logs::LEVEL_SECURITY_ALERT,
                      "Insecure HMAC secret file permissions detected. For security reasons, it is crucial to change this key immediately and reboot "
                      "the service.");
            return jwtNull;
        }

        std::string hmacSecret;

        std::getline(hmacFile, hmacSecret);

        // Remove the newline character if it exists
        if (!hmacSecret.empty() && hmacSecret.back() == '\n')
        {
            hmacSecret.pop_back();
        }

        if (hmacSecret.empty())
        {
            log->log0(__func__, Program::Logs::LEVEL_DEBUG, "Empty JWT HMAC Signing Key.");
            return jwtNull;
        }

        // Set HMAC secret key on jwtSigner object
        jwtSigner->setSharedSecret(hmacSecret);
        log->log0(__func__, Program::Logs::LEVEL_INFO, "JWT HMAC Signing Key Loaded.");

        return jwtSigner;
    }
    else
    {
        std::string privateKeyFilePath = ptr->get<std::string>(configClassName + ".PrivateKeyFile", "jwt.key");
        std::string publicKeyFilePath = ptr->get<std::string>(configClassName + ".PublicKeyFile", "jwt.pub");
        uint16_t createRSASize = ptr->get<bool>(configClassName + ".CreateRSASize", 4096);

        if (privateKeyFilePath.empty())
        {
            log->log0(__func__, Program::Logs::LEVEL_INFO, "No JWT RSA Signing Key Configured.");
            return jwtNull;
        }

        bool loaded = false;
        std::string fileContent;

        FILE *privateKeyFP = fopen(privateKeyFilePath.c_str(), "r");

        if (privateKeyFP == nullptr && createIfNotPresent && createRSASecret(log,privateKeyFilePath, publicKeyFilePath, createRSASize))
        {
            privateKeyFP = fopen(privateKeyFilePath.c_str(), "r");
        }

        if (privateKeyFP != nullptr)
        {
            if (!Helpers::File::isSensitiveConfigPermissionInsecure(privateKeyFilePath, &insecureFile))
            {
                log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to open permissions on JWT RSA Signing Key secret file.");
                fclose(privateKeyFP);
                return jwtNull;
            }

            if (insecureFile)
            {
                log->log0(__func__,
                          Program::Logs::LEVEL_SECURITY_ALERT,
                          "Insecure JWT RSA Signing Key secret file permissions detected. For security reasons, it is crucial to change this key "
                          "immediately and reboot the service.");
                fclose(privateKeyFP);
                return jwtNull;
            }

            EVP_PKEY *evpPrivateKey = PEM_read_PrivateKey(privateKeyFP, nullptr, nullptr, nullptr);
            if (evpPrivateKey)
            {
                BIO *privateKeyBio = BIO_new(BIO_s_mem());
                PEM_write_bio_PrivateKey(privateKeyBio, evpPrivateKey, nullptr, nullptr, 0, nullptr, nullptr);

                char *privateKeyStr;
                long privateKeyLen = BIO_get_mem_data(privateKeyBio, &privateKeyStr);

                fileContent.assign(privateKeyStr, privateKeyLen);

                BIO_free(privateKeyBio);

                jwtSigner->setPrivateSecret(fileContent);
                log->log0(__func__, Program::Logs::LEVEL_INFO, "JWT RSA Signing Key Loaded.");
                loaded = true;
            }
            else
            {
                log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to load the JWT Private Key from file '%s'.", privateKeyFilePath.c_str());
            }

            EVP_PKEY_free(evpPrivateKey);
            fclose(privateKeyFP);
        }
        else
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to read the JWT Private Key from file '%s'.", privateKeyFilePath.c_str());
        }

        if (loaded)
            return jwtSigner;
        else
            return jwtNull;
    }
}

std::shared_ptr<DataFormat::JWT> ConfigBuilder::JWT::createJWTValidator(Program::Logs::AppLog *log,
                                                                 boost::property_tree::ptree *ptr,
                                                                 const std::string &configClassName)
{
    bool insecureFile;
    std::shared_ptr<DataFormat::JWT> jwtNull;
    std::string algorithmName = ptr->get<std::string>(configClassName + ".Algorithm", "HS256");
    bool createIfNotPresent = ptr->get<bool>(configClassName + ".CreateIfNotPresent", true);

    if (!DataFormat::JWT::isAlgorithmSupported(algorithmName))
    {
        log->log0(__func__, Program::Logs::LEVEL_ERR, "JWT algorithm '%s' not supported.", algorithmName.c_str());
        return jwtNull;
    }

    DataFormat::JWT::AlgorithmDetails algorithmDetails(algorithmName.c_str());

    auto jwtValidator = std::make_shared<DataFormat::JWT>(algorithmDetails.m_algorithm);

    if (algorithmDetails.m_usingHMAC)
    {
        // HMACSecret is a file, read the hmacSecret variable from file to file and
        // report error if failed to read or if permissions are not secure.

        auto hmacFilePath = ptr->get<std::string>(configClassName + ".HMACSecret", "jwt_secret.key");

        std::ifstream hmacFile(hmacFilePath.c_str());

        if (!hmacFile.is_open() && createIfNotPresent && createHMACSecret(log,hmacFilePath))
        {
            hmacFile.open(hmacFilePath.c_str());
        }

        if (!hmacFile.is_open())
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to open HMAC secret file.");
            return jwtNull;
        }

        if (!Helpers::File::isSensitiveConfigPermissionInsecure(hmacFilePath, &insecureFile))
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to open permissions on HMAC secret file.");
            return jwtNull;
        }

        if (insecureFile)
        {
            log->log0(__func__,
                      Program::Logs::LEVEL_SECURITY_ALERT,
                      "Insecure HMAC secret file permissions detected. For security reasons, it is crucial to change this key immediately and reboot "
                      "the service.");
            return jwtNull;
        }

        std::string hmacSecret;
        std::getline(hmacFile, hmacSecret);

        // Remove the newline character if it exists
        if (!hmacSecret.empty() && hmacSecret.back() == '\n')
        {
            hmacSecret.pop_back();
        }

        if (hmacSecret.empty())
        {
            log->log0(__func__, Program::Logs::LEVEL_DEBUG, "Empty JWT HMAC Validation Key.");
            return jwtNull;
        }

        // Set HMAC secret key on jwtSigner object
        jwtValidator->setSharedSecret(hmacSecret);
        log->log0(__func__, Program::Logs::LEVEL_INFO, "JWT HMAC Validation Key Loaded.");

        return jwtValidator;
    }
    else
    {
        uint16_t createRSASize = ptr->get<bool>(configClassName + ".CreateRSASize", 4096);
        std::string privateKeyFilePath = ptr->get<std::string>(configClassName + ".PrivateKeyFile", "jwt.key");
        std::string publicKeyFilePath = ptr->get<std::string>(configClassName + ".PublicKeyFile", "jwt.pub");

        if (publicKeyFilePath.empty())
        {
            log->log0(__func__, Program::Logs::LEVEL_CRITICAL, "No JWT RSA Validation Key Configured.");
            return jwtNull;
        }

        bool loaded = false;
        std::string fileContent;

        FILE *publicKeyFP = fopen(publicKeyFilePath.c_str(), "r");

        if (publicKeyFP == nullptr && createIfNotPresent && createRSASecret(log,privateKeyFilePath, publicKeyFilePath, createRSASize))
        {
            publicKeyFP = fopen(publicKeyFilePath.c_str(), "r");
        }

        if (publicKeyFP != nullptr)
        {
            EVP_PKEY *evpPublicKey = PEM_read_PUBKEY(publicKeyFP, nullptr, nullptr, nullptr);
            if (evpPublicKey)
            {
                BIO *publicKeyBio = BIO_new(BIO_s_mem());
                PEM_write_bio_PUBKEY(publicKeyBio, evpPublicKey);

                char *publicKeyStr;
                long publicKeyLen = BIO_get_mem_data(publicKeyBio, &publicKeyStr);

                fileContent.assign(publicKeyStr, publicKeyLen);

                BIO_free(publicKeyBio);

                jwtValidator->setPublicSecret(fileContent);
                log->log0(__func__, Program::Logs::LEVEL_INFO, "JWT RSA Validation Key Loaded.");
                loaded = true;
            }
            else
            {
                log->log0(__func__, Program::Logs::LEVEL_CRITICAL, "Failed to load the JWT Public Key from file '%s'.", publicKeyFilePath.c_str());
            }

            EVP_PKEY_free(evpPublicKey);
            fclose(publicKeyFP);
        }
        else
        {
            log->log0(__func__, Program::Logs::LEVEL_CRITICAL, "Failed to read the JWT Public Key from file '%s'.", publicKeyFilePath.c_str());
        }

        if (loaded)
            return jwtValidator;
        else
            return jwtNull;
    }
}

bool ConfigBuilder::JWT::createHMACSecret(Program::Logs::AppLog *log, const std::string &filePath)
{
    bool r = false;
    int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // 0600 permissions
    if (fd != -1)
    {
        FILE *pkeyFile = fdopen(fd, "w");
        if (pkeyFile)
        {
            auto rndstr = Helpers::Random::createRandomString(32);
            r = fwrite(rndstr.c_str(), 32, 1, pkeyFile) == 1;

            if (r)
            {
                log->log0(__func__, Program::Logs::LEVEL_WARN, "Created JWT HMAC Secret File: %s", filePath.c_str());
            }
            else
            {
                log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (3): %s", filePath.c_str());
            }

            fclose(pkeyFile);
        }
        else
        {
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (2): %s", filePath.c_str());
            close(fd);
        }
    }
    else
        log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to create JWT HMAC Secret File (1): %s", filePath.c_str());

    return r;
}

bool ConfigBuilder::JWT::createRSASecret(Program::Logs::AppLog *log, const std::string &keyPath, const std::string &crtPath, uint16_t keySize)
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
                        log->log0(__func__, Program::Logs::LEVEL_WARN, "Created JWT X.509 RSA Private Key: %s", keyPath.c_str());

                        // Save public key
                        FILE *pubkeyFile = fopen(crtPath.c_str(), "wb");
                        if (pubkeyFile)
                        {
                            success = PEM_write_PUBKEY(pubkeyFile, pkey) > 0;
                            if (success)
                            {
                                log->log0(__func__, Program::Logs::LEVEL_WARN, "Created JWT X.509 RSA Public Key: %s", crtPath.c_str());
                            }
                            else
                            {
                                log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to write X.509 RSA Public Key (2): %s", crtPath.c_str());
                            }
                            fclose(pubkeyFile);
                        }
                        else
                        {
                            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to write X.509 RSA Public Key (1): %s", crtPath.c_str());
                        }
                    }
                    else
                    {
                        log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to write X.509 RSA Private Key (2): %s", keyPath.c_str());
                    }
                    fclose(pkeyFile);
                }
                else
                {
                    log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to write X.509 RSA Private Key (1): %s", keyPath.c_str());
                    close(fd);
                }
            }
            else
                log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to generate X.509 RSA Keys (2)");
        }
        else
            log->log0(__func__, Program::Logs::LEVEL_ERR, "Failed to generate X.509 RSA Keys (1)");
    }

    if (pkey)
        EVP_PKEY_free(pkey);
    if (ctx)
        EVP_PKEY_CTX_free(ctx);

    return success;
}
