#pragma once

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <memory>

namespace Mantids30::Program {
namespace Config {

class JWT
{
public:
    JWT() = default;

    // ==================== MAIN FACTORY FUNCTIONS ====================

    /**
     * @brief Creates a JWT signer instance
     * @param log Application logger
     * @param ptr Configuration property tree
     * @param configClassName Configuration class name prefix
     * @return JWT signer instance or nullptr on failure
     */
    static std::shared_ptr<DataFormat::JWT> createJWTSigner(Logs::AppLog *log, const boost::property_tree::ptree &ptr, const std::string &configClassName,
                                                            const std::map<std::string, std::string> &vars = {});

    /**
     * @brief Creates a JWT validator instance from configuration
     * @param log Application logger
     * @param ptr Configuration property tree
     * @param configClassName Configuration class name prefix
     * @return JWT validator instance or nullptr on failure
     */
    static std::shared_ptr<DataFormat::JWT> createJWTValidator(Logs::AppLog *log, const boost::property_tree::ptree &ptr, const std::string &configClassName,
                                                               const std::map<std::string, std::string> &vars = {});

    /**
     * @brief Creates a JWT validator instance from provided key
     * @param log Application logger
     * @param algorithm Algorithm name
     * @param key Secret key (HMAC) or public key (RSA)
     * @return JWT validator instance or nullptr on failure
     */
    static std::shared_ptr<DataFormat::JWT> createJWTValidator(Logs::AppLog *log, const std::string &algorithm, const std::string &key);

private:
    static bool createHMACSecret(Program::Logs::AppLog *log, const std::string &filePath);
    static bool createRSASecret(Program::Logs::AppLog *log, const std::string &keyPath, const std::string &crtPath, uint16_t keySize = 4096);

    // ==================== HMAC FUNCTIONS ====================

    /**
     * @brief Loads HMAC secret from file
     * @param log Application logger
     * @param hmacFilePath Path to HMAC secret file
     * @param createIfNotPresent Whether to create the file if it doesn't exist
     * @return HMAC secret string, empty if failed
     */
    static std::string loadHMACSecret(Logs::AppLog *log, const std::string &hmacFilePath, bool createIfNotPresent);

    /**
     * @brief Creates JWT object with HMAC algorithm
     * @param log Application logger
     * @param algorithmDetails Algorithm details
     * @param hmacSecret HMAC secret key
     * @param purpose "signing" or "validation" for logging
     * @return Configured JWT object or nullptr on failure
     */
    static std::shared_ptr<DataFormat::JWT> createHMACJWT(Logs::AppLog *log, const DataFormat::JWT::AlgorithmDetails &algorithmDetails, const std::string &hmacSecret, const std::string &purpose);

    // ==================== RSA FUNCTIONS ====================

    /**
     * @brief Loads RSA private key from file
     * @param log Application logger
     * @param privateKeyFilePath Path to private key file
     * @param publicKeyFilePath Path to public key file (for creation)
     * @param createIfNotPresent Whether to create keys if they don't exist
     * @param createRSASize RSA key size for creation
     * @return Private key PEM string, empty if failed
     */
    static std::string loadRSAPrivateKey(Logs::AppLog *log, const std::string &privateKeyFilePath, const std::string &publicKeyFilePath, bool createIfNotPresent, uint16_t createRSASize);

    /**
     * @brief Loads RSA public key from file
     * @param log Application logger
     * @param publicKeyFilePath Path to public key file
     * @param privateKeyFilePath Path to private key file (for creation)
     * @param createIfNotPresent Whether to create keys if they don't exist
     * @param createRSASize RSA key size for creation
     * @return Public key PEM string, empty if failed
     */
    static std::string loadRSAPublicKey(Logs::AppLog *log, const std::string &publicKeyFilePath, const std::string &privateKeyFilePath, bool createIfNotPresent, uint16_t createRSASize);

    // ==================== HELPER FUNCTIONS ====================

    /**
     * @brief Validates JWT algorithm support
     * @param log Application logger
     * @param algorithmName Name of the algorithm to validate
     * @return true if algorithm is supported, false otherwise
     */
    static bool validateAlgorithm(Logs::AppLog *log, const std::string &algorithmName);

    /**
     * @brief Removes trailing newline character from a string
     * @param str String to process
     */
    static void removeTrailingNewline(std::string &str);

    /**
     * @brief Validates file security permissions
     * @param log Application logger
     * @param filePath Path to the file to check
     * @param fileType Type of file for logging purposes
     * @return true if file permissions are secure, false otherwise
     */
    static bool validateFilePermissions(Logs::AppLog *log, const std::string &filePath, const std::string &fileType);

    /**
     * @brief Opens a file with optional creation
     * @param filePath Path to the file
     * @param createIfNotPresent Whether to create the file if it doesn't exist
     * @param createFunc Function to call for file creation
     * @return File stream if successful
     */
    static std::ifstream openFileWithCreation(const std::string &filePath, bool createIfNotPresent, std::function<bool()> createFunc);
};

} // namespace Config
} // namespace Mantids30::Program
