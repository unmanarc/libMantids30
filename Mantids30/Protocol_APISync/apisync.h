#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <stdint.h>
#include <string>

namespace Mantids30::Network::Protocols {
namespace APISync {

/**
 * @brief Structure to hold API synchronization parameters.
 *
 * This structure encapsulates configuration settings required for synchronizing
 * with an external API server, including host, port, TLS options, and certificate
 * validation settings.
 */
struct APISyncParameters
{
    /**
     * @brief Loads API synchronization parameters from a property tree.
     *
     * Parses the provided `boost::property_tree::ptree` object to extract
     * configuration values for API synchronization. If any required fields are
     * missing, default values are used.
     *
     * @param ptr The property tree containing configuration data.
     */
    void loadFromInfoTree(const boost::property_tree::ptree &ptr);

    std::string apiSyncHost;        ///< Hostname or IP address of the API sync server.
    uint16_t apiSyncPort = 7081;    ///< Port number for the API sync service (default: 7081).
    bool checkTLSPeer = true;       ///< Whether to validate TLS peer certificate.
    bool usePrivateCA = false;      ///< Whether to use a custom CA certificate.
    bool useTLS = true;             ///< Whether to use TLS encryption.
    std::string privateCAPath = ""; ///< Path to the private CA certificate file (if applicable).
};

/**
 * @brief Performs an API synchronization request.
 *
 * Sends a JSON-based request to the configured API endpoint for synchronization
 * purposes.
 *
 * @param _log Application log instance for logging messages.
 * @param proxyParameters Configuration parameters for API sync.
 * @param functionName Name of the function to call on the remote API.
 * @param jsonRequest JSON payload to send in the request.
 * @param appName Name of the application making the request.
 * @param apiKey Authentication key used for authorization.
 * @return Response from the API as a JSON object.
 */
json performAPISynchronizationRequest(Mantids30::Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &functionName, const json &jsonRequest, const std::string &appName,
                                      const std::string &apiKey);

/**
 * @brief Retrieves the JWT validation key for an application.
 *
 * Fetches the public key used to verify JWT tokens issued by the API for
 * the specified application.
 *
 * @param *log Application log instance for logging messages.
 * @param proxyParameters Configuration parameters for API sync.
 * @param appName Name of the application.
 * @param apiKey Authentication key used for authorization.
 * @return JSON object containing the JWT validation key.
 */
json getApplicationJWTValidationKey(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey);

/**
 * @brief Retrieves the JWT signing key for an application.
 *
 * Fetches the private key used to sign JWT tokens for the specified application.
 *
 * @param *log Application log instance for logging messages.
 * @param proxyParameters Configuration parameters for API sync.
 * @param appName Name of the application.
 * @param apiKey Authentication key used for authorization.
 * @return JSON object containing the JWT signing key.
 */
//json getApplicationJWTSigningKey(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey);

/**
 * @brief Retrieves the JWT configuration for an application.
 *
 * Fetches the JWT-related configuration settings (e.g., issuer, audience, etc.)
 * for the specified application.
 *
 * @param *log Application log instance for logging messages.
 * @param proxyParameters Configuration parameters for API sync.
 * @param appName Name of the application.
 * @param apiKey Authentication key used for authorization.
 * @return JSON object containing the JWT configuration.
 */
json getApplicationJWTConfig(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey);

/**
 * @brief Updates or sets Scopes, Roles and Activities for an application.
 *
 * Sends a request to update the scopes, roles, and activities associated with
 * the given application.
 *
 * @param *log Application log instance for logging messages.
 * @param proxyParameters Configuration parameters for API sync.
 * @param appName Name of the application.
 * @param apiKey Authentication key used for authorization.
 * @param scopes JSON array of scopes used by the application.
 * @param roles JSON array of roles used by the application.
 * @param activities JSON array of activities that can be performed.
 * @return Response from the API after updating SRA settings.
 */
json updateAccessControlContext(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey, const json &scopes, const json &roles,
                                const json &activities);

} // namespace APISync
} // namespace Mantids30::Network::Protocols
